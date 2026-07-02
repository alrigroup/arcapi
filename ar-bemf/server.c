#include "server.h" // AR-BEMF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <strings.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <time.h>
#include <stdarg.h>

// Forward declarations
static void arc_log(const char *level, const char *format, ...);


#define WEB_BASE_PATH "ar-ws/web"

// Portable path canonicalization: resolves ".." and "." components in-place
static void canonicalize_path(char *dst, size_t dst_size, const char *src) {
    char stack[512][256];
    int top = 0;
    char tmp[512];
    strncpy(tmp, src, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
#ifdef _WIN32
    for (char *p = tmp; *p; p++) if (*p == '\\') *p = '/';
#endif
    char *token = strtok(tmp, "/");
    while (token && top < 512) {
        if (strcmp(token, "..") == 0) {
            if (top > 0) top--;
        } else if (strcmp(token, ".") != 0 && strcmp(token, "") != 0) {
            strncpy(stack[top], token, sizeof(stack[top]) - 1);
            stack[top][sizeof(stack[top]) - 1] = '\0';
            top++;
        }
        token = strtok(NULL, "/");
    }
    dst[0] = '\0';
    size_t pos = 0;
    for (int i = 0; i < top && pos < dst_size - 2; i++) {
        if (i > 0 || src[0] == '/') {
            size_t remaining = dst_size - pos;
            if (remaining > 1) {
                dst[pos] = '/'; pos++;
            }
        }
        size_t remaining = dst_size - pos;
        if (remaining > 1 && pos + strlen(stack[i]) < dst_size - 1) {
            strncpy(dst + pos, stack[i], remaining - 1);
            pos += strlen(stack[i]);
        }
    }
    if (pos == 0 && dst_size > 1) {
        dst[0] = '.'; dst[1] = '\0';
    } else {
        dst[pos] = '\0';
    }
}

// ------------------------------------------------------------------
// Framework Print Utilities
// ------------------------------------------------------------------
// (Movidos para ar-ws core/tty.c para customização de projeto)

static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit((unsigned char)a) && isxdigit((unsigned char)b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// ------------------------------------------------------------------
// Opaque structure implementation
// ------------------------------------------------------------------
struct ClientConnection {
    int socket_fd;
    SSL *ssl;
    int mode; // MODE_SECURE or MODE_INSECURE
    char client_ip[64];
    char current_path[256];
    char response_headers[1024]; // Permite adicionar multiplos Set-Cookies dinamicamente
    char anon_id[65];            // Identificador de tracking do visitante
};

// ------------------------------------------------------------------
// Globals for the Server
// ------------------------------------------------------------------
static RequestHandler global_api_handler = NULL;
static LoggerCallback global_logger = NULL;

typedef struct AllowedPath {
    char path[512];
    struct AllowedPath *next;
} AllowedPath;

static AllowedPath *allowed_paths = NULL;

// ------------------------------------------------------------------
// URL Decoding Utility
// ------------------------------------------------------------------

static int calculate_file_sha256(const char *path, char *output_buffer) {
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int buf_size = 32768; // Lê o arquivo em chunks de 32KB
    unsigned char *buffer = malloc(buf_size);
    if (!buffer) { fclose(file); return 0; }
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, buf_size, file))) {
        SHA256_Update(&sha256, buffer, bytes_read);
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(output_buffer + (i * 2), 3, "%02x", hash[i]);
    }
    free(buffer);
    fclose(file);
    return 1;
}

static void scan_files_to_hashes(const char *dir_name, cJSON *object) {
    DIR *d = opendir(dir_name);
    if (!d) return;
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
        struct stat s;
        if (stat(path, &s) == 0) {
            if (S_ISDIR(s.st_mode)) {
                scan_files_to_hashes(path, object);
            } else if (S_ISREG(s.st_mode)) {
                char hash_hex[65];
                if (calculate_file_sha256(path, hash_hex)) {
                    cJSON_AddStringToObject(object, path, hash_hex);
                }
            }
        }
    }
    closedir(d);
}

static void create_directories(const char *path) {
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0'; // Garante que é C-String segura
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755); // Cria o nível atual de diretório
            *p = '/';
        }
    }
}

// ------------------------------------------------------------------
// Analytics & Logs (In-Memory)
// ------------------------------------------------------------------
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_LOG_ENTRIES 500
typedef struct {
    time_t ts;
    char level[16];
    char message[256];
} LogEntry;
static LogEntry server_logs[MAX_LOG_ENTRIES];
static int log_head = 0;
static int log_count = 0;

static void arc_log(const char *level, const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (strcmp(level, "INFO") == 0) alri_print(CYAN"[ARC-INFO]"RESET" %s\n", buffer);
    else if (strcmp(level, "WARN") == 0) alri_print(YELLOW"[ARC-WARN]"RESET" %s\n", buffer);
    else if (strcmp(level, "ERROR") == 0) alri_print(RED"[ARC-ERROR]"RESET" %s\n", buffer);
    else if (strcmp(level, "MAP") == 0) { /* suppress indexed file spam */ }
    else alri_print(GREEN"[ARC]"RESET" %s\n", buffer);

    pthread_mutex_lock(&log_mutex);

    FILE *f = fopen("arc_server.log", "a");
    if (f) {
        time_t now = time(NULL);
        char time_buf[64];
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);
        fprintf(f, "[%s] [%s] %s\n", time_buf, level, buffer);
        fclose(f);
    }

    server_logs[log_head].ts = time(NULL);
    strncpy(server_logs[log_head].level, level, sizeof(server_logs[log_head].level) - 1);
    strncpy(server_logs[log_head].message, buffer, sizeof(server_logs[log_head].message) - 1);
    log_head = (log_head + 1) % MAX_LOG_ENTRIES;
    if(log_count < MAX_LOG_ENTRIES) log_count++;
    pthread_mutex_unlock(&log_mutex);
}

#define MAX_ROUTE_STATS 256
typedef struct { char path[256]; int count; } RouteStat;
static RouteStat route_stats[MAX_ROUTE_STATS];
static int route_stat_count = 0;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

static void track_route(const char *path) {
    if (!path || strncmp(path, "/manager/", 9) == 0) return; // Ignora rotas do painel

    pthread_mutex_lock(&stats_mutex);
    for (int i = 0; i < route_stat_count; i++) {
        if (strcmp(route_stats[i].path, path) == 0) {
            route_stats[i].count++;
            pthread_mutex_unlock(&stats_mutex);
            return;
        }
    }
    if (route_stat_count < MAX_ROUTE_STATS) {
        strncpy(route_stats[route_stat_count].path, path, 255);
        route_stats[route_stat_count].count = 1;
        route_stat_count++;
    }
    pthread_mutex_unlock(&stats_mutex);
}

#define MAX_ACCESS_LOGS 1000
typedef struct {
    char ip[64];
    time_t ts;
    char path[256];
    int status;
    char anon_id[65];
} AccessLog;
static AccessLog access_logs[MAX_ACCESS_LOGS];
static int access_head = 0;
static int access_count = 0;
static pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;

static void track_access(const char *ip, const char *path, int status, const char *anon_id) {
    if (global_logger) {
        global_logger(ip, path, status, anon_id);
    }
}

// ------------------------------------------------------------------
// Internal helper: Map allowed paths
// ------------------------------------------------------------------
static void add_allowed_path(const char *path) {
    AllowedPath *new_path = (AllowedPath *)malloc(sizeof(AllowedPath));
    if (!new_path) return;
    strncpy(new_path->path, path, sizeof(new_path->path) - 1);
    new_path->path[sizeof(new_path->path) - 1] = '\0';
    new_path->next = allowed_paths;
    allowed_paths = new_path;
    arc_log("MAP", "Indexed: [%s]", path);
}

static int is_path_allowed(const char *path) {
    // Bloqueia caracteres perigosos no path

#ifdef _WIN32
    if (strstr(path, "..") != NULL || strchr(path, '%') != NULL ||
        strchr(path, '\\') != NULL || strchr(path, ':') != NULL) {
        return 0;
    }
#else
    if (strstr(path, "..") != NULL || strchr(path, '%') != NULL) {
        return 0;
    }
#endif

    // Rejeita diretórios para evitar fopen em pastas (retorna Content-Length corrupto)
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 0;
    }

    // Whitelist de paths pré-escaneados (mais seguro)
    AllowedPath *curr = allowed_paths;
    while (curr) {
        if (strcmp(curr->path, path) == 0) return 1;
        curr = curr->next;
    }

    // Fallback: resolve canonicalmente e verifica prefixo
    char canon[512];
    char base_canon[512];
    canonicalize_path(canon, sizeof(canon), path);
    canonicalize_path(base_canon, sizeof(base_canon), WEB_BASE_PATH);

    size_t base_len = strlen(base_canon);
    if (strncmp(canon, base_canon, base_len) == 0 &&
        (canon[base_len] == '/' || canon[base_len] == '\0')) {
        return 1;
    }

    return 0;
}

static void scan_web_directory(const char *dir_name) {
    DIR *d = opendir(dir_name);
    if (!d) {
        arc_log("ERROR", "Could not open directory: %s", dir_name);
        return;
    }
    
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;
            
        char path[512];
        if (dir_name[strlen(dir_name)-1] == '/') {
            snprintf(path, sizeof(path), "%s%s", dir_name, dir->d_name);
        } else {
            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
        }
        
        struct stat s;
        if (stat(path, &s) == 0) {
            if (S_ISDIR(s.st_mode)) {
                scan_web_directory(path);
            } else if (S_ISREG(s.st_mode)) {
                add_allowed_path(path);
            }
        }
    }
    closedir(d);
}

// ------------------------------------------------------------------
// I/O Abstraction
// ------------------------------------------------------------------
int server_conn_write(ClientConnection *conn, const void *buf, int num) {
    if (conn->mode == MODE_SECURE && conn->ssl != NULL) {
        return SSL_write(conn->ssl, buf, num);
    } else {
        return write(conn->socket_fd, buf, num);
    }
}

int server_conn_read(ClientConnection *conn, void *buf, int num) {
    if (conn->mode == MODE_SECURE && conn->ssl != NULL) {
        return SSL_read(conn->ssl, buf, num);
    } else {
        return read(conn->socket_fd, buf, num);
    }
}

// ------------------------------------------------------------------
// API exposed implementations
// ------------------------------------------------------------------

void server_send_response(ClientConnection *conn, int status, const char *content_type, const char *body) {
    char headers[1024];
    int body_len = body ? strlen(body) : 0;
    
    const char *status_text = "OK";
    if (status == 404) status_text = "Not Found";
    else if (status == 500) status_text = "Internal Server Error";
    
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "%s"
             "Connection: close\r\n\r\n",
             status, status_text, content_type, body_len, conn->response_headers);
             
    server_conn_write(conn, headers, strlen(headers));
    if (body) {
        server_conn_write(conn, body, body_len);
    }
    track_access(conn->client_ip, conn->current_path, status, conn->anon_id);
}

void server_add_header(ClientConnection *conn, const char *header_line) {
    if (!conn || !header_line) return;
    // Sanitiza CRLF para prevenir HTTP Response Splitting
    char sanitized[1024];
    int si = 0;
    for (int i = 0; header_line[i] != '\0' && si < (int)sizeof(sanitized) - 2; i++) {
        if (header_line[i] != '\r' && header_line[i] != '\n') {
            sanitized[si++] = header_line[i];
        }
    }
    sanitized[si] = '\0';

    int current_len = strlen(conn->response_headers);
    int line_len = strlen(sanitized);
    
    if (current_len + line_len + 2 < sizeof(conn->response_headers) - 1) {
        strcat(conn->response_headers, sanitized);
        strcat(conn->response_headers, "\r\n");
    }
}

void server_redirect(ClientConnection *conn, const char *url) {
    if (!conn || !url) return;
    // Sanitiza CRLF no URL para prevenir HTTP Response Splitting
    char safe_url[1024];
    int si = 0;
    for (int i = 0; url[i] != '\0' && si < (int)sizeof(safe_url) - 1; i++) {
        if (url[i] != '\r' && url[i] != '\n') {
            safe_url[si++] = url[i];
        }
    }
    safe_url[si] = '\0';

    char headers[1024];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 302 Found\r\n"
             "Location: %s\r\n"
             "Content-Length: 0\r\n"
             "%s"
             "Connection: close\r\n\r\n",
             safe_url, conn->response_headers);
    server_conn_write(conn, headers, strlen(headers));
}

void server_send_404(ClientConnection *conn) {
    FILE *f = fopen("ar-ws/web/error-404/error-404.html", "rb");
    if (!f) {
        server_send_response(conn, 404, "text/html", "<h1>404</h1>");
        return;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *html = malloc(len + 1);
    if (!html) {
        fclose(f);
        server_send_response(conn, 404, "text/html", "<h1>404</h1>");
        return;
    }
    fread(html, 1, len, f);
    html[len] = '\0';
    fclose(f);
    server_send_response(conn, 404, "text/html", html);
    free(html);
}

int server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type) {
    if (!is_path_allowed(filepath)) {
        track_access(conn->client_ip, conn->current_path, 403, conn->anon_id);
        return 0; 
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) return 0;
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    server_add_header(conn, "Cache-Control: public, max-age=3600\r\n");
    
    char headers[1024];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "%s"
             "Connection: close\r\n\r\n",
             content_type, fsize, conn->response_headers);
             
    server_conn_write(conn, headers, strlen(headers));
    
    char chunk[8192];
    size_t bytes_read;
    while ((bytes_read = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (server_conn_write(conn, chunk, bytes_read) <= 0) {
            break; // Interrompe imediatamente se o cliente fechar a conexão
        }
    }
    
    fclose(f);
    track_access(conn->client_ip, conn->current_path, 200, conn->anon_id);
    return 1;
}

// ------------------------------------------------------------------
// Request Helpers
// ------------------------------------------------------------------
const char* get_header(HttpRequest *req, const char *header_name) {
    if (!req || !header_name) return NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, header_name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char* get_path_param(HttpRequest *req, const char *key) {
    if (!req || !key) return NULL;
    for (int i = 0; i < req->path_param_count; i++) {
        if (strcmp(req->path_params[i].key, key) == 0) {
            return req->path_params[i].value;
        }
    }
    return NULL;
}

const char* get_query_param(HttpRequest *req, const char *key) {
    // Busca thread-safe otimizada sem buffer sobrescrito
    if (!req || !key) return NULL;
    for (int i = 0; i < req->query_count; i++) {
        if (strcmp(req->parsed_query[i].key, key) == 0) {
            return req->parsed_query[i].value;
        }
    }
    return NULL;
}

// ------------------------------------------------------------------
// JSON Helpers
// ------------------------------------------------------------------
cJSON* parse_json_body(HttpRequest *req) {
    if (!req || !req->body) return NULL;
    // Cache do JSON para evitar vazamentos (Item 6 do ROADMAP)
    if (!req->json_doc) {
        req->json_doc = cJSON_Parse(req->body);
    }
    return req->json_doc;
}

void server_send_json(ClientConnection *conn, int status, cJSON *json_obj) {
    if (!conn || !json_obj) return;
    char *json_str = cJSON_PrintUnformatted(json_obj);
    if (json_str) {
        server_send_response(conn, status, "application/json", json_str);
        cJSON_free(json_str); // [FIX cJSON Mem Leak]
    }
    cJSON_Delete(json_obj); // Libera o recurso da memória perfeitamente
}

// ------------------------------------------------------------------
// Security: Constant Time Compare & Brute Force Protection
// ------------------------------------------------------------------
static pthread_mutex_t brute_force_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_FAILED_IPS 100
typedef struct {
    char ip[64];
    int count;
    time_t lockout_until;
} FailedLogin;
static FailedLogin failed_logins[MAX_FAILED_IPS] = {0};
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_ADMIN_SESSIONS 100
typedef struct {
    char token[65];
    char ip[64];
    char username[64];
    time_t expires_at;
    int active;
} AdminSession;
static AdminSession admin_sessions[MAX_ADMIN_SESSIONS] = {0};

// ------------------------------------------------------------------
// Infrastructure Helpers
// ------------------------------------------------------------------
const char* server_get_client_ip(ClientConnection *conn) {
    if (!conn) return "0.0.0.0";
    return conn->client_ip;
}

// ------------------------------------------------------------------
// API Handlers (Agnostic Core)
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// HTTP -> HTTPS Redirector (Threaded)
// ------------------------------------------------------------------
static void *http_worker_thread(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    
    char buffer[1024];
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Captura dinâmica do Host para redirecionamento correto
        char *host_start = strstr(buffer, "Host: ");
        if (!host_start) host_start = strstr(buffer, "host: ");
        
        char host[256] = "localhost"; // Fallback
        if (host_start) {
            host_start += 6;
            char *host_end = strstr(host_start, "\r\n");
            if (host_end && (host_end - host_start) < sizeof(host)) {
                strncpy(host, host_start, host_end - host_start);
                host[host_end - host_start] = '\0';
            }
        }

        const char *trusted_host = getenv("TRUSTED_DOMAIN");
        if (!trusted_host || trusted_host[0] == '\0') trusted_host = "localhost";
        // Valida o Host header contra domínio confiável para evitar Open Redirect
        if (strcmp(host, trusted_host) != 0) {
            strncpy(host, trusted_host, sizeof(host) - 1);
            host[sizeof(host) - 1] = '\0';
        }

        char response[1024];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 301 Moved Permanently\r\n"
                 "Location: https://%s/\r\n"
                 "Content-Length: 0\r\n"
                 "Connection: close\r\n\r\n", host);
        write(client_socket, response, strlen(response));
    }
    close(client_socket);
    return NULL;
}

static void *redirector_thread(void *arg) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024];

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        alri_print("Failed redirector socket\n");
        return NULL;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(80);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        alri_print("Error opening port 80\n");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 100) < 0) {
        alri_print("Error on port 80 Listen\n");
        close(server_fd);
        return NULL;
    }

    arc_log("INFO", "Running on port 80, redirecting to HTTPS...");

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            continue;
        }
        
        int *sock_ptr = malloc(sizeof(int));
        if (sock_ptr) {
            *sock_ptr = client_socket;
            pthread_t worker_tid;
            if (pthread_create(&worker_tid, NULL, http_worker_thread, sock_ptr) == 0) {
                pthread_detach(worker_tid);
            } else {
                free(sock_ptr);
                close(client_socket);
            }
        }
    }
    return NULL;
}

// ------------------------------------------------------------------
// Main Worker Thread
// ------------------------------------------------------------------
static void handle_client(ClientConnection *conn) {
    char *buffer = calloc(1, 16384); // Buffer dinâmico e expandido
    if (!buffer) return;
    int total_read = 0;
    char *body_start = NULL;
    int content_length = 0;
    int headers_parsed = 0;
    
    // Loop de fragmentação TCP e Body Length
    while (total_read < 16383) {
        int bytes_read = server_conn_read(conn, buffer + total_read, 16383 - total_read);
        if (bytes_read <= 0) {
            break;
        }
        total_read += bytes_read;
        buffer[total_read] = '\0';
        
        if (!headers_parsed && (body_start = strstr(buffer, "\r\n\r\n")) != NULL) {
            headers_parsed = 1;
            char *cl_str = strstr(buffer, "Content-Length:");
            if (!cl_str) cl_str = strstr(buffer, "content-length:");
            if (cl_str && cl_str < body_start) {
                content_length = atoi(cl_str + 15);
            }
        }

        if (headers_parsed) {
            int header_len = (body_start + 4) - buffer;
            if (total_read - header_len >= content_length) {
                break; // Todas as partes TCP necessárias já chegaram
            }
        }
    }
    
    if (!body_start) { free(buffer); return; }
    
    HttpRequest req;
    memset(&req, 0, sizeof(HttpRequest));
    req.admin_role = 2; // Guest/SUP fallback (Proteção contra Broken Access)
    conn->response_headers[0] = '\0';
    conn->anon_id[0] = '\0';
    // Security headers (Etapa 4)
    server_add_header(conn, "Content-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline' https://cdnjs.cloudflare.com https://cdn.jsdelivr.net; style-src 'self' 'unsafe-inline' https://cdnjs.cloudflare.com https://fonts.googleapis.com https://fonts.gstatic.com; font-src 'self' https://fonts.gstatic.com https://cdnjs.cloudflare.com; img-src 'self' https://i.ibb.co https://cdn.alrigroup.com data:; manifest-src https://cdn.alrigroup.com; frame-ancestors 'none'\r\n");
    server_add_header(conn, "X-Content-Type-Options: nosniff\r\n");
    server_add_header(conn, "X-Frame-Options: DENY\r\n");
    server_add_header(conn, "Strict-Transport-Security: max-age=31536000; includeSubDomains\r\n");
    
    // Extrai o corpo (body) da requisição separando por duplo CRLF
    *body_start = '\0'; // Quebra a string para isolar os headers
    req.body = body_start + 4;
    req.body_length_in_buffer = total_read - (body_start + 4 - buffer);
    
    // Garante que o corpo seja tratado como uma string limpa terminando no content_length
    if (content_length > 0 && content_length < (16384 - (body_start + 4 - buffer))) {
        req.body[content_length] = '\0';
    }

    // Separa a linha principal de requisição do restante dos headers
    char *headers_start = strstr(buffer, "\r\n");
    if (headers_start) {
        *headers_start = '\0';
        headers_start += 2;
    }

    char *saveptr;
    char *method = strtok_r(buffer, " ", &saveptr);
    char *full_path = strtok_r(NULL, " ", &saveptr);
    
    if (!method || !full_path) {
        server_send_response(conn, 400, "text/plain", "Bad Request");
        free(buffer);
        return;
    }
    
    url_decode(full_path, full_path);
    
    req.method = method;
    req.path = full_path;
    strncpy(conn->current_path, full_path, sizeof(conn->current_path) - 1);
    track_route(full_path);
    
    char *query = strchr(full_path, '?');
    if (query) {
        *query = '\0'; 
        req.query_params = query + 1;
        
        // Parse pre-emptive das query strings de forma thread-safe
        req.query_count = 0;
        char *q = req.query_params;
        while (q && *q && req.query_count < 50) {
            char *amp = strchr(q, '&');
            if (amp) *amp = '\0';
            char *eq = strchr(q, '=');
            if (eq) {
                *eq = '\0';
                req.parsed_query[req.query_count].key = q;
                req.parsed_query[req.query_count].value = eq + 1;
            } else {
                req.parsed_query[req.query_count].key = q;
                req.parsed_query[req.query_count].value = "";
            }
            req.query_count++;
            if (amp) q = amp + 1;
            else break;
        }
    }
    
    // Parse seguro dos Headers e preenchimento na Struct
    req.header_count = 0;
    if (headers_start) {
        char *line = headers_start;
        char *next_line;
        while (line && *line != '\r' && *line != '\n' && *line != '\0' && req.header_count < 100) {
            next_line = strstr(line, "\r\n");
            if (next_line) {
                *next_line = '\0';
                next_line += 2;
            }
            
            char *colon = strchr(line, ':');
            if (colon) {
                *colon = '\0';
                req.headers[req.header_count].name = line;
                char *val = colon + 1;
                while (*val == ' ') val++; // Remove o espaço inicial do valor
                req.headers[req.header_count].value = val;
                req.header_count++;
            }
            line = next_line;
        }
    }
    
    // Extrai o Host header (domínio) sem a porta, trailing dot ou espaços
    req.host[0] = '\0';
    const char *host_header = get_header(&req, "Host");
    if (host_header) {
        const char *start = host_header;
        while (*start == ' ') start++;
        const char *end = start;
        while (*end && *end != ':') end++;
        int len = end - start;
        if (len > 255) len = 255;
        strncpy(req.host, start, len);
        req.host[len] = '\0';
        // Remove trailing dot (FQDN como "alrigroup.com.")
        while (len > 0 && req.host[len - 1] == '.') {
            req.host[--len] = '\0';
        }
    }

    // [FIX 1.2.1] Se atrás do Cloudflare, confia em CF-Connecting-IP
    const char *cf_ip = get_header(&req, "CF-Connecting-IP");
    if (cf_ip) {
        strncpy(conn->client_ip, cf_ip, sizeof(conn->client_ip) - 1);
        conn->client_ip[sizeof(conn->client_ip) - 1] = '\0';
    }

    // Rastreamento Anonimo de Visitantes
    const char *cookie_header = get_header(&req, "Cookie");
    if (cookie_header) {
        char *anon_ptr = strstr(cookie_header, "ARC_ANON_ID=");
        if (anon_ptr) {
            strncpy(conn->anon_id, anon_ptr + 12, 64);
            conn->anon_id[64] = '\0';
            char *semi = strchr(conn->anon_id, ';');
            if (semi) *semi = '\0';
        }
    }
    if (conn->anon_id[0] == '\0') {
        unsigned char rand_bytes[32];
        if (RAND_bytes(rand_bytes, 32) != 1) {
            arc_log("ERROR", "Failed to generate random bytes for anonymous ID");
            snprintf(conn->anon_id, sizeof(conn->anon_id), "%lx%x", (unsigned long)time(NULL), getpid());
        } else {
            for (int i = 0; i < 32; i++) snprintf(conn->anon_id + (i * 2), 3, "%02x", rand_bytes[i]);
        }
        snprintf(conn->response_headers + strlen(conn->response_headers), 1024 - strlen(conn->response_headers),
                 "Set-Cookie: ARC_ANON_ID=%s; Path=/; HttpOnly; SameSite=Lax; Secure\r\n", conn->anon_id);
    }

    // Repasse absoluto para a Camada de Aplicação (Cérebro / ar-ws)
    if (global_api_handler) {
        global_api_handler(conn, &req);
    } else {
        server_send_response(conn, 404, "text/plain", "Not Found");
    }

    if (req.json_doc) cJSON_Delete(req.json_doc);
    free(buffer);
}

typedef struct {
    int client_sock;
    SSL_CTX *ctx;
    int mode;
    char client_ip[64];
} ClientThreadArgs;

static void* client_thread(void *arg) {
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    
    ClientConnection conn;
    conn.socket_fd = args->client_sock;
    conn.mode = args->mode;
    conn.ssl = NULL;
    strncpy(conn.client_ip, args->client_ip, sizeof(conn.client_ip) - 1);
    memset(conn.current_path, 0, sizeof(conn.current_path));

    if (conn.mode == MODE_SECURE) {
        conn.ssl = SSL_new(args->ctx);
        SSL_set_fd(conn.ssl, conn.socket_fd);

        if (SSL_accept(conn.ssl) > 0) {
            handle_client(&conn);
        } else {
            // Limpeza essencial da fila de erros se o cliente abortar o Handshake
            // Previne Memory Leak a longo prazo na libssl
            ERR_clear_error();
        }
        SSL_shutdown(conn.ssl);
        SSL_free(conn.ssl);
    } else {
        // Mode Insecure
        handle_client(&conn);
    }

    close(conn.socket_fd);
    free(args);
    return NULL;
}

// ------------------------------------------------------------------
// Server Core Initialization
// ------------------------------------------------------------------
void server_start(int port, int mode, RequestHandler handler) {
    signal(SIGPIPE, SIG_IGN);
    global_api_handler = handler;


    arc_log("MAP", "Scanning web directory...");
    scan_web_directory(WEB_BASE_PATH);

    SSL_CTX *ctx = NULL;

    if (mode == MODE_SECURE) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();

        const SSL_METHOD *method = TLS_server_method();
        ctx = SSL_CTX_new(method);
        
        if (!ctx) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }

        if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
        if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
        
        // Start HTTP -> HTTPS redirector on port 80 in parallel
        pthread_t redir_tid;
        pthread_create(&redir_tid, NULL, redirector_thread, NULL);
        pthread_detach(redir_tid);
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        alri_print("Error on main server bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 100) < 0) {
        alri_print("Error on main server listen\n");
        exit(EXIT_FAILURE);
    }

    if (mode == MODE_SECURE) {
        arc_log("INFO", "HTTPS Server started on port %d!", port);
    } else {
        arc_log("INFO", "HTTP Server started on port %d!", port);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock < 0) continue;

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

        ClientThreadArgs *args = (ClientThreadArgs *)malloc(sizeof(ClientThreadArgs));
        args->client_sock = client_sock;
        args->ctx = ctx;
        args->mode = mode;
        
        char ip_str[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
        strncpy(args->client_ip, ip_str, sizeof(args->client_ip) - 1);
        
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, args) == 0) {
            pthread_detach(tid);
        } else {
            close(client_sock);
            free(args);
        }
    }

    if (ctx) SSL_CTX_free(ctx);
}

void server_set_logger(LoggerCallback callback) {
    global_logger = callback;
}
