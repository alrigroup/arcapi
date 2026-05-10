#include "server.h"
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


#define WEB_BASE_PATH "web"

#include <stdbool.h>
bool global_tty_print_enabled = false;

void alri_print(const char *format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s", buffer);

    if (global_tty_print_enabled) {
        FILE *tty = fopen("/dev/tty1", "w");
        if (tty) {
            fprintf(tty, "%s", buffer);
            fclose(tty);
        }
    }
}

void alri_print_force(const char *format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("%s", buffer);
    FILE *tty = fopen("/dev/tty1", "w");
    if (tty) {
        fprintf(tty, "%s", buffer);
        fclose(tty);
    }
}

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

typedef struct AllowedPath {
    char path[512];
    struct AllowedPath *next;
} AllowedPath;

static AllowedPath *allowed_paths = NULL;

// ------------------------------------------------------------------
// Persistent JSON Database (Users & Sessions)
// ------------------------------------------------------------------
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;
static cJSON *global_db = NULL; // Cache Global em RAM

static void load_db_ram() {
    if (global_db) { cJSON_Delete(global_db); global_db = NULL; }
    FILE *f = fopen("db.json", "rb");
    if (f) {
        fseek(f, 0, SEEK_END); long fsize = ftell(f); fseek(f, 0, SEEK_SET);
        char *content = malloc(fsize + 1);
        if (content) {
            fread(content, 1, fsize, f); content[fsize] = '\0';
            global_db = cJSON_Parse(content); free(content);
        }
        fclose(f);
    }
}

static void sync_db_disk() {
    if (!global_db) return;
    char *content = cJSON_PrintUnformatted(global_db);
    if (content) {
        FILE *f = fopen("db.json", "wb");
        if (f) {
            fwrite(content, 1, strlen(content), f);
            fclose(f);
        }
        cJSON_free(content); // [FIX cJSON Mem Leak]
    }
}

// ------------------------------------------------------------------
// Auth & Security Middlewares
// ------------------------------------------------------------------
static void init_db_json() {
    pthread_mutex_lock(&db_mutex);
    load_db_ram();
    if (!global_db) {
        global_db = cJSON_CreateObject();
        cJSON *users = cJSON_CreateArray();
        cJSON *sessions = cJSON_CreateArray();
        cJSON *config = cJSON_CreateObject();
        cJSON_AddItemToObject(global_db, "users", users);
        cJSON_AddItemToObject(global_db, "sessions", sessions);
        cJSON_AddItemToObject(global_db, "config", config);
        
        cJSON_AddBoolToObject(config, "tty_print", false);

        cJSON *root_user = cJSON_CreateObject();
        // Username em texto claro, senha "admin" pré-hasheada em SHA-256
        cJSON_AddStringToObject(root_user, "user", "admin");
        cJSON_AddNumberToObject(root_user, "role", 0); // 0 = ROOT, 1 = ADMIN, 2 = SUP
        cJSON_AddStringToObject(root_user, "pass", "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918");
        cJSON_AddItemToArray(users, root_user);
        
        sync_db_disk();
        alri_print(CYAN"[ARC-INFO]"RESET" db.json created with default 'admin/admin' credentials (Hashed).\n");
    } else {
        cJSON *config = cJSON_GetObjectItem(global_db, "config");
        if (!config) {
            config = cJSON_CreateObject();
            cJSON_AddBoolToObject(config, "tty_print", false);
            cJSON_AddItemToObject(global_db, "config", config);
            sync_db_disk();
        }
        cJSON *tty_print = cJSON_GetObjectItem(config, "tty_print");
        if (tty_print && cJSON_IsBool(tty_print)) {
            global_tty_print_enabled = cJSON_IsTrue(tty_print);
        }
    }
    pthread_mutex_unlock(&db_mutex);
}

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
    else if (strcmp(level, "MAP") == 0) alri_print(PURPLE"[ARC-MAP]"RESET" %s\n", buffer);
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
    if (!ip || !path || strncmp(path, "/manager/", 9) == 0) return; // Ignora logs do painel
    pthread_mutex_lock(&access_mutex);
    strncpy(access_logs[access_head].ip, ip, 63);
    access_logs[access_head].ts = time(NULL);
    strncpy(access_logs[access_head].path, path, 255);
    access_logs[access_head].status = status;
    if (anon_id) strncpy(access_logs[access_head].anon_id, anon_id, 64);
    else access_logs[access_head].anon_id[0] = '\0';
    
    access_head = (access_head + 1) % MAX_ACCESS_LOGS;
    if (access_count < MAX_ACCESS_LOGS) access_count++;
    pthread_mutex_unlock(&access_mutex);
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
    // Prevenção robusta: bloqueia null bytes diretos, ".." e evita
    // bypass de double URL encoding bloqueando o '%' após o decode.
    if (strstr(path, "..") != NULL || strchr(path, '%') != NULL || strstr(path, "%00") != NULL) {
        return 0;
    }

    AllowedPath *curr = allowed_paths;
    while (curr) {
        if (strcmp(curr->path, path) == 0) return 1;
        curr = curr->next;
    }
    
    if (strncmp(path, WEB_BASE_PATH, strlen(WEB_BASE_PATH)) == 0) {
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
// I/O Abstraction (Write)
// ------------------------------------------------------------------
static int conn_write(ClientConnection *conn, const void *buf, int num) {
    if (conn->mode == MODE_SECURE && conn->ssl != NULL) {
        return SSL_write(conn->ssl, buf, num);
    } else {
        return write(conn->socket_fd, buf, num);
    }
}

static int conn_read(ClientConnection *conn, void *buf, int num) {
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
             
    conn_write(conn, headers, strlen(headers));
    if (body) {
        conn_write(conn, body, body_len);
    }
    track_access(conn->client_ip, conn->current_path, status, conn->anon_id);
}

void server_redirect(ClientConnection *conn, const char *url) {
    if (!conn || !url) return;
    char headers[1024];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 302 Found\r\n"
             "Location: %s\r\n"
             "Content-Length: 0\r\n"
             "%s"
             "Connection: close\r\n\r\n",
             url, conn->response_headers);
    conn_write(conn, headers, strlen(headers));
}

int server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type) {
    if (!is_path_allowed(filepath)) {
        arc_log("ERROR", "Access denied (Path Traversal): %s", filepath);
        track_access(conn->client_ip, conn->current_path, 403, conn->anon_id);
        return 0; 
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) return 0;
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char headers[1024];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "%s"
             "Connection: close\r\n\r\n",
             content_type, fsize, conn->response_headers);
             
    conn_write(conn, headers, strlen(headers));
    
    char chunk[8192];
    size_t bytes_read;
    while ((bytes_read = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (conn_write(conn, chunk, bytes_read) <= 0) {
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

static int constant_time_compare(const char *a, const char *b) {
    if (!a || !b) return 0;
    int len_a = strlen(a);
    int len_b = strlen(b);
    if (len_a != len_b) return 0;
    int result = 0;
    for (int i = 0; i < len_a; i++) result |= (a[i] ^ b[i]);
    return result == 0;
}

int server_validate_admin_login(const char *username, const char *pass_hash, const char *ip, int *out_role) {
    if (!username || !pass_hash || !ip) return 0;

    pthread_mutex_lock(&brute_force_mutex);
    time_t now = time(NULL);
    int ip_idx = -1;
    for (int i = 0; i < MAX_FAILED_IPS; i++) {
        if (failed_logins[i].ip[0] != '\0' && strcmp(failed_logins[i].ip, ip) == 0) {
            ip_idx = i;
            if (failed_logins[i].lockout_until > now) {
                pthread_mutex_unlock(&brute_force_mutex);
                arc_log("WARN", "Brute force blocked for IP: %s", ip);
                return 0; // Lockout de Segurança Ativo
            }
            break;
        }
    }
    pthread_mutex_unlock(&brute_force_mutex);

    int is_valid = 0;
    pthread_mutex_lock(&db_mutex);
    if (global_db) {
        cJSON *users = cJSON_GetObjectItem(global_db, "users");
        cJSON *u;
        cJSON_ArrayForEach(u, users) {
            cJSON *u_name = cJSON_GetObjectItem(u, "user");
            cJSON *u_pass = cJSON_GetObjectItem(u, "pass");
            if (u_name && u_pass && strcmp(u_name->valuestring, username) == 0 && constant_time_compare(u_pass->valuestring, pass_hash)) {
                is_valid = 1;
                cJSON *u_role = cJSON_GetObjectItem(u, "role");
                if (u_role && out_role) *out_role = u_role->valueint;
                else if (out_role) *out_role = 2; // Fallback para SUP
                break;
            }
        }
    }
    pthread_mutex_unlock(&db_mutex);

    // Gravação da Tentativa de Acesso
    pthread_mutex_lock(&brute_force_mutex);
    if (!is_valid) {
        if (ip_idx == -1) {
            for (int i = 0; i < MAX_FAILED_IPS; i++) {
                if (failed_logins[i].ip[0] == '\0' || failed_logins[i].lockout_until < now - 3600) {
                    ip_idx = i;
                    strncpy(failed_logins[i].ip, ip, 63);
                    failed_logins[i].count = 0;
                    failed_logins[i].lockout_until = 0;
                    break;
                }
            }
        }
        if (ip_idx == -1) {
            // Array de Brute Force cheio: Substitui o registro mais antigo para impedir Bypass
            int oldest_idx = 0;
            for (int i = 1; i < MAX_FAILED_IPS; i++) {
                if (failed_logins[i].lockout_until < failed_logins[oldest_idx].lockout_until) oldest_idx = i;
            }
            ip_idx = oldest_idx;
            strncpy(failed_logins[ip_idx].ip, ip, 63);
            failed_logins[ip_idx].count = 0;
            failed_logins[ip_idx].lockout_until = 0;
        }
        if (ip_idx != -1) {
            failed_logins[ip_idx].count++;
            if (failed_logins[ip_idx].count >= 5) {
                failed_logins[ip_idx].lockout_until = now + 300; // Bloqueio de 5 Minutos
                arc_log("ERROR", "IP %s locked out for 5 minutes due to multiple failed logins.", ip);
            }
        }
    } else if (ip_idx != -1) {
        failed_logins[ip_idx].count = 0; // Sucesso reseta as tentativas
        failed_logins[ip_idx].lockout_until = 0;
    }
    pthread_mutex_unlock(&brute_force_mutex);

    return is_valid;
}

// Validação Zero-Trust para Sudo-Mode
static int verify_sudo(const char *username, const char *confirm_pass) {
    if (!username || !confirm_pass || username[0] == '\0') return 0;
    int valid = 0;
    pthread_mutex_lock(&db_mutex);
    if (global_db) {
        cJSON *users = cJSON_GetObjectItem(global_db, "users");
        cJSON *u;
        cJSON_ArrayForEach(u, users) {
            cJSON *uname = cJSON_GetObjectItem(u, "user");
            if (uname && strcmp(uname->valuestring, username) == 0) {
                cJSON *upass = cJSON_GetObjectItem(u, "pass");
                if (upass && constant_time_compare(upass->valuestring, confirm_pass)) valid = 1;
                break;
            }
        }
    }
    pthread_mutex_unlock(&db_mutex);
    return valid;
}

const char* server_create_admin_session(const char *username, const char *ip) {
    if (!ip) return NULL;
    unsigned char rand_bytes[32];
    // Generate cryptographically strong pseudo-random bytes
    if (RAND_bytes(rand_bytes, sizeof(rand_bytes)) != 1) return NULL;
    
    static __thread char token_str[65]; // Thread-local to avoid buffer overwrite
    for (int i = 0; i < 32; i++) snprintf(token_str + (i * 2), 3, "%02x", rand_bytes[i]);
    
    time_t now = time(NULL);
    pthread_mutex_lock(&session_mutex);
    for (int i = 0; i < MAX_ADMIN_SESSIONS; i++) {
        if (!admin_sessions[i].active || admin_sessions[i].expires_at < now) {
            strncpy(admin_sessions[i].token, token_str, 64);
            strncpy(admin_sessions[i].ip, ip, 63);
            strncpy(admin_sessions[i].username, username, 63);
            admin_sessions[i].expires_at = now + 3600; // 1 hour validity
            admin_sessions[i].active = 1;
            pthread_mutex_unlock(&session_mutex);
            return admin_sessions[i].token;
        }
    }
    pthread_mutex_unlock(&session_mutex);
    return NULL;
}

static int is_valid_admin_session(const char *token, const char *ip, int *out_role) {
    if (!token || !ip) return 0;
    time_t now = time(NULL);
    int valid = 0;
    pthread_mutex_lock(&session_mutex);
    for (int i = 0; i < MAX_ADMIN_SESSIONS; i++) {
        if (admin_sessions[i].active && admin_sessions[i].expires_at >= now) {
            // Comparação blindada contra vazamento de tempo e validação de IP
            if (constant_time_compare(admin_sessions[i].token, token)) { 
                if (strcmp(admin_sessions[i].ip, ip) == 0) {
                    valid = 1;
                    if (out_role) {
                        pthread_mutex_lock(&db_mutex);
                        cJSON *users = cJSON_GetObjectItem(global_db, "users");
                        cJSON *u;
                        *out_role = 2; // Default restrito
                        cJSON_ArrayForEach(u, users) {
                            cJSON *uname = cJSON_GetObjectItem(u, "user");
                            if (uname && strcmp(uname->valuestring, admin_sessions[i].username) == 0) {
                                cJSON *urole = cJSON_GetObjectItem(u, "role");
                                if (urole) *out_role = urole->valueint;
                                break;
                            }
                        }
                        pthread_mutex_unlock(&db_mutex);
                    }
                } else {
                    valid = -1; // Clonagem de IP detectada (Segurança Crítica)
                }
                break; 
            }
        } else if (admin_sessions[i].active && admin_sessions[i].expires_at < now) {
            // Garbage Collection (Limpa token morto ativamente)
            admin_sessions[i].active = 0;
            memset(admin_sessions[i].token, 0, sizeof(admin_sessions[i].token));
        }
    }
    pthread_mutex_unlock(&session_mutex);
    return valid;
}

// ------------------------------------------------------------------
// HTTP -> HTTPS Redirector (Threaded)
// ------------------------------------------------------------------
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
        
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        
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

            // Obtém o Path requisitado (se possível) para manter a rota ao redirecionar
            char response[1024];
            snprintf(response, sizeof(response),
                     "HTTP/1.1 301 Moved Permanently\r\n"
                     "Location: https://%s/\r\n"
                     "Content-Length: 0\r\n"
                     "Connection: close\r\n\r\n", 
                     host);

            write(client_socket, response, strlen(response));
        }
        close(client_socket);
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
        int bytes_read = conn_read(conn, buffer + total_read, 16383 - total_read);
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
    conn->response_headers[0] = '\0';
    conn->anon_id[0] = '\0';
    
    // Extrai o corpo (body) da requisição separando por duplo CRLF
    *body_start = '\0'; // Quebra a string para isolar os headers
    req.body = body_start + 4;
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
    
    // [FIX 1.2] Evita Evasão do IP-Binding em Servidores via Reverse Proxy
    const char *xff = get_header(&req, "X-Forwarded-For");
    if (!xff) xff = get_header(&req, "X-Real-IP");
    if (xff) {
        char *comma = strchr(xff, ',');
        if (comma) {
            int len = comma - xff;
            if (len > 63) len = 63;
            strncpy(conn->client_ip, xff, len);
            conn->client_ip[len] = '\0';
        } else {
            strncpy(conn->client_ip, xff, 63);
            conn->client_ip[63] = '\0';
        }
    }

    // Rastreamento Anonimo de Visitantes & Busca de JWT Admin via Cookie
    char admin_token[65] = {0};
    const char *cookie_header = get_header(&req, "Cookie");
    if (cookie_header) {
        char *anon_ptr = strstr(cookie_header, "ARC_ANON_ID=");
        if (anon_ptr) {
            strncpy(conn->anon_id, anon_ptr + 12, 64);
            char *semi = strchr(conn->anon_id, ';');
            if (semi) *semi = '\0';
        }
        char *adm_ptr = strstr(cookie_header, "arc_admin_token=");
        if (adm_ptr) {
            strncpy(admin_token, adm_ptr + 16, 64);
            char *semi = strchr(admin_token, ';');
            if (semi) *semi = '\0';
        }
    }
    if (conn->anon_id[0] == '\0') {
        unsigned char rand_bytes[32];
        RAND_bytes(rand_bytes, 32);
        for (int i = 0; i < 32; i++) snprintf(conn->anon_id + (i * 2), 3, "%02x", rand_bytes[i]);
        snprintf(conn->response_headers + strlen(conn->response_headers), 1024 - strlen(conn->response_headers),
                 "Set-Cookie: ARC_ANON_ID=%s; Path=/; HttpOnly; SameSite=Lax\r\n", conn->anon_id);
    }

    // Proteção em Nível de Servidor: Bloqueia o HTML/CSS/JS do Dashboard
    // Se o usuário não tiver o Cookie do admin válido, o servidor nunca entregará o arquivo.
    if (strncmp(req.path, "/manager/dashboard", 18) == 0) {
        char *final_token = admin_token[0] != '\0' ? admin_token : NULL;
        int role_trash = 2;
        int auth_status = is_valid_admin_session(final_token, conn->client_ip, &role_trash);
        if (auth_status != 1) {
            snprintf(conn->response_headers + strlen(conn->response_headers), 1024 - strlen(conn->response_headers),
                     "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
            server_redirect(conn, "/manager/login");
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer);
            return; // Aborta completamente o envio da página
        }
        
        // Trava Diretórios Estáticos para Evitar Bypass (Obriga uso do Component Loader)
        if (strncmp(req.path, "/manager/dashboard/tabs", 23) == 0) {
            server_send_response(conn, 403, "text/plain", "Direct access to component files is strictly forbidden.");
            free(buffer); return;
        }
    }

    // Middleware de Segurança (Sessão JWT / Em Memória) para proteger rotas do Manager
    if (strncmp(req.path, "/manager/api/", 13) == 0) {

        // Intercepta a Rota NATIVA de Login (Não Exige Token)
        if (strcmp(req.path, "/manager/api/login") == 0 && strcmp(req.method, "POST") == 0) {
            cJSON *json = parse_json_body(&req);
            if (!json) {
                server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON payload\"}");
            } else {
                cJSON *u = cJSON_GetObjectItem(json, "user");
                cJSON *p = cJSON_GetObjectItem(json, "pass");
                if (!u || !p || !cJSON_IsString(u) || !cJSON_IsString(p)) {
                    server_send_response(conn, 400, "application/json", "{\"error\": \"Missing username or password hash\"}");
                } else {
                    int logged_role = 2;
                    if (server_validate_admin_login(u->valuestring, p->valuestring, conn->client_ip, &logged_role)) {
                        const char *token = server_create_admin_session(u->valuestring, conn->client_ip);
                        if (token) {
                            snprintf(conn->response_headers + strlen(conn->response_headers), 1024 - strlen(conn->response_headers),
                                     "Set-Cookie: arc_admin_token=%s; Path=/; HttpOnly; SameSite=Lax\r\n", token);
                            cJSON *resp = cJSON_CreateObject();
                            cJSON_AddStringToObject(resp, "message", "Authenticated");
                            cJSON_AddNumberToObject(resp, "role", logged_role);
                            server_send_json(conn, 200, resp);
                        } else {
                            server_send_response(conn, 500, "application/json", "{\"error\": \"Session database error\"}");
                        }
                    } else {
                        server_send_response(conn, 401, "application/json", "{\"error\": \"Invalid credentials or IP blocked\"}");
                    }
                }
            }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }
        
        if (strcmp(req.path, "/manager/api/logout") == 0) {
            snprintf(conn->response_headers + strlen(conn->response_headers), 1024 - strlen(conn->response_headers),
                     "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
            server_send_response(conn, 200, "application/json", "{\"message\": \"Logged out successfully\"}");
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }

        // --- VALIDAÇÃO DE SESSÃO OBRIGATÓRIA PARA O RESTANTE DA API ---
        const char *auth_header = get_header(&req, "Authorization");
        char *final_token = NULL;
        if (auth_header && strncmp(auth_header, "Bearer ", 7) == 0) final_token = (char*)auth_header + 7;
        else if (admin_token[0] != '\0') final_token = admin_token;
        
        int logged_in_role = 2;
        char logged_in_user[64] = {0};
        int auth_status = is_valid_admin_session(final_token, conn->client_ip, &logged_in_role);
        
        if (auth_status == 1) {
            pthread_mutex_lock(&session_mutex);
            for(int i=0; i<MAX_ADMIN_SESSIONS; i++) {
                if (admin_sessions[i].active && strcmp(admin_sessions[i].token, final_token) == 0) {
                    strncpy(logged_in_user, admin_sessions[i].username, 63);
                    break;
                }
            }
            pthread_mutex_unlock(&session_mutex);
        }

        if (auth_status != 1) {
            snprintf(conn->response_headers + strlen(conn->response_headers), 1024 - strlen(conn->response_headers),
                     "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
            if (auth_status == -1) server_send_response(conn, 401, "application/json", "{\"error\": \"Sessão invalidada: IP mismatch.\"}");
            else server_send_response(conn, 401, "application/json", "{\"error\": \"Unauthorized\", \"message\": \"Token invalid or missing.\"}");
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }

        // Server-Side Component Rendering (Com logged_in_role já validado)
        if (strncmp(req.path, "/manager/api/component/", 23) == 0 && strcmp(req.method, "GET") == 0) {
            const char *component = req.path + 23;
            if (strstr(component, "..") || strchr(component, '/')) {
                server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid component\"}");
                free(buffer); return;
            }

            if (strcmp(component, "tab-tty") == 0 || strcmp(component, "tab-update") == 0) {
                if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden\"}"); free(buffer); return; }
            } else if (strcmp(component, "tab-users") == 0) {
                if (logged_in_role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden\"}"); free(buffer); return; }
            }
            
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "web/manager/dashboard/tabs/%s.html", component);
            if (!server_serve_file(conn, filepath, "text/html")) {
                server_send_response(conn, 404, "application/json", "{\"error\": \"Component not found\"}");
            }
            free(buffer); return;
        }

        // Configurações Globais (TTY Print)
        if (strcmp(req.path, "/manager/api/config") == 0 && strcmp(req.method, "GET") == 0) {
            if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); free(buffer); return; }
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp, "tty_print", global_tty_print_enabled);
            server_send_json(conn, 200, resp);
            free(buffer); return;
        }

        if (strcmp(req.path, "/manager/api/config/tty") == 0 && strcmp(req.method, "POST") == 0) {
            if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); free(buffer); return; }
            
            const char *confirm_pass = get_header(&req, "X-Confirm-Pass");
            if (!confirm_pass || !verify_sudo(logged_in_user, confirm_pass)) {
                arc_log("WARN", "Failed sudo auth for TTY config by user '%s'", logged_in_user);
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
                free(buffer); return;
            }

            cJSON *json = parse_json_body(&req);
            if (json) {
                cJSON *tty_print = cJSON_GetObjectItem(json, "tty_print");
                if (tty_print && (tty_print->type == cJSON_True || tty_print->type == cJSON_False)) {
                    pthread_mutex_lock(&db_mutex);
                    if (global_db) {
                        cJSON *config = cJSON_GetObjectItem(global_db, "config");
                        if (!config) { config = cJSON_CreateObject(); cJSON_AddItemToObject(global_db, "config", config); }
                        int is_true = (tty_print->type == cJSON_True);
                        if (cJSON_GetObjectItem(config, "tty_print")) {
                            cJSON_ReplaceItemInObject(config, "tty_print", cJSON_CreateBool(is_true));
                        } else {
                            cJSON_AddBoolToObject(config, "tty_print", is_true);
                        }
                        global_tty_print_enabled = is_true;
                        sync_db_disk();
                    }
                    pthread_mutex_unlock(&db_mutex);
                    server_send_response(conn, 200, "application/json", "{\"message\": \"Configuration updated.\"}");
                } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}"); }
            } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON.\"}"); }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }
        
        // System Manual Restart
        if (strcmp(req.path, "/manager/api/system/restart") == 0 && strcmp(req.method, "POST") == 0) {
            if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); free(buffer); return; }
            cJSON *json = parse_json_body(&req);
            if (json) {
                cJSON *mode = cJSON_GetObjectItem(json, "mode");
                cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
                if (mode && cp && cJSON_IsString(mode) && cJSON_IsString(cp)) {
                    int sudo_ok = 0;
                    pthread_mutex_lock(&db_mutex);
                    if (global_db) {
                        cJSON *users = cJSON_GetObjectItem(global_db, "users");
                        cJSON *curr;
                        cJSON_ArrayForEach(curr, users) {
                            cJSON *uname = cJSON_GetObjectItem(curr, "user");
                            if (uname && strcmp(uname->valuestring, logged_in_user) == 0) {
                                cJSON *upass = cJSON_GetObjectItem(curr, "pass");
                                if (upass && constant_time_compare(upass->valuestring, cp->valuestring)) sudo_ok = 1;
                                break;
                            }
                        }
                    }
                    pthread_mutex_unlock(&db_mutex);

                    if (!sudo_ok) {
                        arc_log("WARN", "Failed sudo auth for system restart by user '%s'", logged_in_user);
                        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta.\"}");
                    } else {
                        arc_log("WARN", "User '%s' triggered system restart (%s)", logged_in_user, mode->valuestring);
                        if (strcmp(mode->valuestring, "api") == 0) {
                            server_send_response(conn, 200, "application/json", "{\"message\": \"Restarting API...\"}");
                            kill(getppid(), SIGUSR1);
                        } else if (strcmp(mode->valuestring, "core") == 0) {
                            server_send_response(conn, 200, "application/json", "{\"message\": \"Restarting Core...\"}");
                            kill(getppid(), SIGUSR2);
                        } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid mode.\"}"); }
                    }
                } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}"); }
            } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON.\"}"); }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }
        
        // Global RBAC Enforcer (Bloqueia ROOT/ADMIN paths para quem não tem cargo)
        if ((strncmp(req.path, "/manager/api/tty", 16) == 0 || 
             strncmp(req.path, "/manager/api/upload", 19) == 0 || 
             strncmp(req.path, "/manager/api/delete", 19) == 0 || 
             strncmp(req.path, "/manager/api/hashes", 20) == 0) && logged_in_role > 0) {
            server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: Requires ROOT access.\"}");
            free(buffer); return;
        }

        // Middlewares Interceptadores Nativos (Endpoints de Analytics In-Memory)
        if (strcmp(req.path, "/manager/api/metrics") == 0) {
            cJSON *resp = cJSON_CreateObject();
            cJSON *arr = cJSON_AddArrayToObject(resp, "metrics");
            pthread_mutex_lock(&stats_mutex);
            for(int i=0; i < route_stat_count; i++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "path", route_stats[i].path);
                cJSON_AddNumberToObject(item, "count", route_stats[i].count);
                cJSON_AddItemToArray(arr, item);
            }
            pthread_mutex_unlock(&stats_mutex);
            server_send_json(conn, 200, resp); free(buffer); return;
        }
        if (strcmp(req.path, "/manager/api/ips") == 0) {
            cJSON *resp = cJSON_CreateObject();
            cJSON *arr = cJSON_AddArrayToObject(resp, "ips");
            pthread_mutex_lock(&access_mutex);
            int start = access_count < MAX_ACCESS_LOGS ? 0 : access_head;
            for(int i=0; i < access_count; i++) {
                int idx = (start + i) % MAX_ACCESS_LOGS;
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "ip", access_logs[idx].ip);
                cJSON_AddNumberToObject(item, "timestamp", (double)access_logs[idx].ts);
                cJSON_AddStringToObject(item, "path", access_logs[idx].path);
                cJSON_AddNumberToObject(item, "status", access_logs[idx].status);
                cJSON_AddStringToObject(item, "anon_id", access_logs[idx].anon_id);
                cJSON_AddItemToArray(arr, item);
            }
            pthread_mutex_unlock(&access_mutex);
            server_send_json(conn, 200, resp); free(buffer); return;
        }
        if (strcmp(req.path, "/manager/api/logs") == 0) {
            cJSON *resp = cJSON_CreateObject();
            cJSON *arr = cJSON_AddArrayToObject(resp, "logs");
            pthread_mutex_lock(&log_mutex);
            int start = log_count < MAX_LOG_ENTRIES ? 0 : log_head;
            for(int i=0; i < log_count; i++) {
                int idx = (start + i) % MAX_LOG_ENTRIES;
                cJSON *item = cJSON_CreateObject();
                cJSON_AddNumberToObject(item, "timestamp", (double)server_logs[idx].ts);
                cJSON_AddStringToObject(item, "level", server_logs[idx].level);
                cJSON_AddStringToObject(item, "message", server_logs[idx].message);
                cJSON_AddItemToArray(arr, item);
            }
            pthread_mutex_unlock(&log_mutex);
            server_send_json(conn, 200, resp); free(buffer); return;
        }
        if (strcmp(req.path, "/manager/api/hashes") == 0) {
            cJSON *resp = cJSON_CreateObject();
            cJSON *files_obj = cJSON_AddObjectToObject(resp, "files");
            scan_files_to_hashes("source", files_obj);
            scan_files_to_hashes("web", files_obj);
            server_send_json(conn, 200, resp); free(buffer); return;
        }
        if (strcmp(req.path, "/manager/api/upload") == 0 && strcmp(req.method, "POST") == 0) {
            const char *target_path = get_header(&req, "X-Target-Path");
            const char *restart_mode = get_header(&req, "X-Restart-Mode");
            const char *confirm_pass = get_header(&req, "X-Confirm-Pass");
            
            if (!target_path || !restart_mode || strstr(target_path, "..") || strchr(target_path, '%')) {
                server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid parameters or Path Traversal attempt.\"}");
                free(buffer); return;
            }

            // Validação de Segurança Sudo
            if (!confirm_pass || !verify_sudo(logged_in_user, confirm_pass)) {
                arc_log("WARN", "Failed sudo auth for file upload by user '%s'", logged_in_user);
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
                free(buffer); return;
            }
            
            if (strncmp(target_path, "source/", 7) != 0 && strncmp(target_path, "web/", 4) != 0) {
                server_send_response(conn, 403, "application/json", "{\"error\": \"Target path outside allowed directories.\"}");
                free(buffer); return;
            }
            
            create_directories(target_path); // Permite deploy de novas estruturas/assets

            FILE *out = fopen(target_path, "wb");
            if (!out) {
                server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to open target file for writing.\"}");
                free(buffer); return;
            }

            int body_in_buffer = total_read - (req.body - buffer);
            if (body_in_buffer > 0) {
                int to_write = body_in_buffer > content_length ? content_length : body_in_buffer;
                fwrite(req.body, 1, to_write, out);
            }
            int remaining = content_length - body_in_buffer;
            char chunk[8192];
            while (remaining > 0) {
                int to_read = remaining > (int)sizeof(chunk) ? (int)sizeof(chunk) : remaining;
                int r = conn_read(conn, chunk, to_read);
                if (r <= 0) break;
                fwrite(chunk, 1, r, out);
                remaining -= r;
            }
            fclose(out);
            
            if (strcmp(restart_mode, "api") == 0) {
                kill(getppid(), SIGUSR1);
            } else if (strcmp(restart_mode, "core") == 0) {
                kill(getppid(), SIGUSR2);
            }
            
            arc_log("INFO", "User '%s' uploaded and synced file: %s", logged_in_user, target_path);
            server_send_response(conn, 200, "application/json", "{\"message\": \"Upload successful!\"}");
            free(buffer); return;
        }
        
        if (strcmp(req.path, "/manager/api/delete") == 0 && strcmp(req.method, "POST") == 0) {
            const char *target_path = get_header(&req, "X-Target-Path");
            const char *restart_mode = get_header(&req, "X-Restart-Mode");
            const char *confirm_pass = get_header(&req, "X-Confirm-Pass");
            
            if (!target_path || !restart_mode || strstr(target_path, "..") || strchr(target_path, '%')) {
                server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid parameters or Path Traversal attempt.\"}");
                free(buffer); return;
            }

            // Validação de Segurança Sudo
            if (!confirm_pass || !verify_sudo(logged_in_user, confirm_pass)) {
                arc_log("WARN", "Failed sudo auth for file delete by user '%s'", logged_in_user);
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
                free(buffer); return;
            }
            
            if (strncmp(target_path, "source/", 7) != 0 && strncmp(target_path, "web/", 4) != 0) {
                server_send_response(conn, 403, "application/json", "{\"error\": \"Target path outside allowed directories.\"}");
                free(buffer); return;
            }
            
            if (remove(target_path) == 0) {
                if (strcmp(restart_mode, "api") == 0) kill(getppid(), SIGUSR1);
                else if (strcmp(restart_mode, "core") == 0) kill(getppid(), SIGUSR2);
                arc_log("INFO", "User '%s' deleted file: %s", logged_in_user, target_path);
                server_send_response(conn, 200, "application/json", "{\"message\": \"File deleted successfully!\"}");
            } else {
                server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to delete file or file not found.\"}");
            }
            free(buffer); return;
        }
        
        if (strcmp(req.path, "/manager/api/admin/list") == 0 && strcmp(req.method, "GET") == 0) {
            if (logged_in_role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); free(buffer); return; }
            cJSON *resp = cJSON_CreateObject();
            cJSON *arr = cJSON_AddArrayToObject(resp, "admins");
            pthread_mutex_lock(&db_mutex);
            if (global_db) {
                cJSON *users = cJSON_GetObjectItem(global_db, "users");
                cJSON *u;
                cJSON_ArrayForEach(u, users) {
                    cJSON *uname = cJSON_GetObjectItem(u, "user");
                    cJSON *urole = cJSON_GetObjectItem(u, "role");
                    if (uname && cJSON_IsString(uname)) {
                        cJSON *item = cJSON_CreateObject();
                        cJSON_AddStringToObject(item, "user", uname->valuestring);
                        cJSON_AddNumberToObject(item, "role", urole ? urole->valueint : 2);
                        cJSON_AddItemToArray(arr, item);
                    }
                }
            }
            pthread_mutex_unlock(&db_mutex);
            server_send_json(conn, 200, resp); free(buffer); return;
        }
        
        if (strcmp(req.path, "/manager/api/admin/create") == 0 && strcmp(req.method, "POST") == 0) {
            if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); free(buffer); return; }
            cJSON *json = parse_json_body(&req);
            if (json) {
                cJSON *u = cJSON_GetObjectItem(json, "user");
                cJSON *p = cJSON_GetObjectItem(json, "pass");
                cJSON *r = cJSON_GetObjectItem(json, "role");
                cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");

                if (u && p && r && cp && cJSON_IsString(u) && cJSON_IsString(p) && cJSON_IsNumber(r) && cJSON_IsString(cp)) {
                    int sudo_ok = 0;
                    pthread_mutex_lock(&db_mutex);
                    cJSON *users = cJSON_GetObjectItem(global_db, "users");
                    cJSON *curr;
                    cJSON_ArrayForEach(curr, users) {
                        cJSON *uname = cJSON_GetObjectItem(curr, "user");
                        if (uname && strcmp(uname->valuestring, logged_in_user) == 0) {
                            cJSON *upass = cJSON_GetObjectItem(curr, "pass");
                            if (upass && constant_time_compare(upass->valuestring, cp->valuestring)) sudo_ok = 1;
                            break;
                        }
                    }

                    if (!sudo_ok) {
                        pthread_mutex_unlock(&db_mutex);
                        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha de confirmação inválida (Sudo Mode).\"}");
                    } else {
                        int exists = 0;
                        cJSON_ArrayForEach(curr, users) {
                            cJSON *uname = cJSON_GetObjectItem(curr, "user");
                            if (uname && strcmp(uname->valuestring, u->valuestring) == 0) { exists = 1; break; }
                        }
                        if (!exists) {
                            cJSON *new_user = cJSON_CreateObject();
                            cJSON_AddStringToObject(new_user, "user", u->valuestring);
                            cJSON_AddStringToObject(new_user, "pass", p->valuestring);
                            cJSON_AddNumberToObject(new_user, "role", r->valueint);
                            cJSON_AddItemToArray(users, new_user);
                            sync_db_disk();
                        }
                        pthread_mutex_unlock(&db_mutex);
                        if (exists) server_send_response(conn, 400, "application/json", "{\"error\": \"Username already exists.\"}");
                        else server_send_response(conn, 200, "application/json", "{\"message\": \"Admin created successfully!\"}");
                    }
                } else server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}");
            }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }
        
        if (strcmp(req.path, "/manager/api/admin/delete") == 0 && strcmp(req.method, "POST") == 0) {
            if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); free(buffer); return; }
            cJSON *json = parse_json_body(&req);
            if (json) {
                cJSON *u = cJSON_GetObjectItem(json, "user");
                cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
                if (u && cp && cJSON_IsString(u) && cJSON_IsString(cp)) {
                    int sudo_ok = 0;
                    pthread_mutex_lock(&db_mutex);
                    cJSON *users = cJSON_GetObjectItem(global_db, "users");
                    cJSON *curr;
                    cJSON_ArrayForEach(curr, users) {
                        cJSON *uname = cJSON_GetObjectItem(curr, "user");
                        if (uname && strcmp(uname->valuestring, logged_in_user) == 0) {
                            cJSON *upass = cJSON_GetObjectItem(curr, "pass");
                            if (upass && constant_time_compare(upass->valuestring, cp->valuestring)) sudo_ok = 1;
                            break;
                        }
                    }

                    if (!sudo_ok) {
                        pthread_mutex_unlock(&db_mutex);
                        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta.\"}");
                    } else if (strcmp(u->valuestring, "admin") == 0) {
                        pthread_mutex_unlock(&db_mutex);
                        server_send_response(conn, 403, "application/json", "{\"error\": \"Cannot delete master root admin.\"}");
                    } else {
                        int deleted = 0; int i = 0;
                        cJSON_ArrayForEach(curr, users) {
                            cJSON *uname = cJSON_GetObjectItem(curr, "user");
                            if (uname && strcmp(uname->valuestring, u->valuestring) == 0) {
                                cJSON_DeleteItemFromArray(users, i);
                                sync_db_disk(); deleted = 1; break;
                            }
                            i++;
                        }
                        pthread_mutex_unlock(&db_mutex);
                        if (deleted) server_send_response(conn, 200, "application/json", "{\"message\": \"Admin deleted successfully!\"}");
                        else server_send_response(conn, 404, "application/json", "{\"error\": \"Admin not found.\"}");
                    }
                } else server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}");
            }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }
        
        if (strcmp(req.path, "/manager/api/admin/role") == 0 && strcmp(req.method, "POST") == 0) {
            if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); free(buffer); return; }
            cJSON *json = parse_json_body(&req);
            if (json) {
                cJSON *u = cJSON_GetObjectItem(json, "user");
                cJSON *r = cJSON_GetObjectItem(json, "role");
                cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
                
                if (!u || !cJSON_IsString(u)) { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload: user missing or not a string.\"}"); }
                else if (!r || (!cJSON_IsNumber(r) && !cJSON_IsString(r))) { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload: role missing or not a number/string.\"}"); }
                else if (!cp || !cJSON_IsString(cp)) { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload: confirm_pass missing or not a string.\"}"); }
                else {
                    int target_role = cJSON_IsNumber(r) ? r->valueint : atoi(r->valuestring);
                    int sudo_ok = 0;
                    pthread_mutex_lock(&db_mutex);
                    cJSON *users = cJSON_GetObjectItem(global_db, "users");
                    cJSON *curr;
                    cJSON_ArrayForEach(curr, users) {
                        cJSON *uname = cJSON_GetObjectItem(curr, "user");
                        if (uname && strcmp(uname->valuestring, logged_in_user) == 0) {
                            cJSON *upass = cJSON_GetObjectItem(curr, "pass");
                            if (upass && constant_time_compare(upass->valuestring, cp->valuestring)) sudo_ok = 1;
                            break;
                        }
                    }

                    if (!sudo_ok) {
                        pthread_mutex_unlock(&db_mutex);
                        server_send_response(conn, 401, "application/json", "{\"error\": \"Sudo confirmation failed.\"}");
                    } else if (strcmp(u->valuestring, "admin") == 0) {
                        pthread_mutex_unlock(&db_mutex);
                        server_send_response(conn, 403, "application/json", "{\"error\": \"Cannot change role of master root admin.\"}");
                    } else {
                        int updated = 0;
                        cJSON_ArrayForEach(curr, users) {
                            cJSON *uname = cJSON_GetObjectItem(curr, "user");
                            if (uname && strcmp(uname->valuestring, u->valuestring) == 0) {
                                cJSON_ReplaceItemInObject(curr, "role", cJSON_CreateNumber(target_role));
                                sync_db_disk(); updated = 1; break;
                            }
                        }
                        pthread_mutex_unlock(&db_mutex);
                        if (updated) server_send_response(conn, 200, "application/json", "{\"message\": \"Admin role updated!\"}");
                        else server_send_response(conn, 404, "application/json", "{\"error\": \"Admin not found.\"}");
                    }
                }
            } else {
                server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON payload.\"}");
            }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }

        if (strcmp(req.path, "/manager/api/admin/update") == 0 && strcmp(req.method, "POST") == 0) {
            if (logged_in_role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); free(buffer); return; }
            cJSON *json = parse_json_body(&req);
            if (json) {
                cJSON *u = cJSON_GetObjectItem(json, "user");
                cJSON *p = cJSON_GetObjectItem(json, "pass");
                if (u && p && cJSON_IsString(u) && cJSON_IsString(p)) {
                    int updated = 0;
                    pthread_mutex_lock(&db_mutex);
                    if (global_db) {
                        cJSON *users = cJSON_GetObjectItem(global_db, "users");
                        cJSON *curr;
                        cJSON_ArrayForEach(curr, users) {
                            cJSON *uname = cJSON_GetObjectItem(curr, "user");
                            if (uname && strcmp(uname->valuestring, u->valuestring) == 0) {
                                cJSON *urole = cJSON_GetObjectItem(curr, "role");
                                int target_role = urole ? urole->valueint : 2;
                                if (logged_in_role == 1 && target_role <= 1) {
                                    updated = -1; // Sem permissão para resetar ADMIN/ROOT
                                    break;
                                }
                                cJSON_ReplaceItemInObject(curr, "pass", cJSON_CreateString(p->valuestring));
                                sync_db_disk();
                                updated = 1;
                                break;
                            }
                        }
                    }
                    pthread_mutex_unlock(&db_mutex);
                    if (updated == 1) server_send_response(conn, 200, "application/json", "{\"message\": \"Admin password updated!\"}");
                    else if (updated == -1) server_send_response(conn, 403, "application/json", "{\"error\": \"Permission denied to reset this user's password.\"}");
                    else server_send_response(conn, 404, "application/json", "{\"error\": \"Admin not found.\"}");
                } else {
                    server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid hashes payload.\"}");
                }
            }
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }
    }

        if (global_api_handler) {
            global_api_handler(conn, &req);
            if (req.json_doc) cJSON_Delete(req.json_doc);
            free(buffer); return;
        }

    server_send_response(conn, 404, "text/plain", "Not Found");
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

// ------------------------------------------------------------------
// Standalone Server Entry Point
// ------------------------------------------------------------------
extern void api_plugin_init();
extern void api_plugin_handler(ClientConnection *conn, HttpRequest *req);

int main() {
    init_db_json(); // Carrega config global ANTES de qualquer print
    alri_print_force(GREEN"[ARC-CORE]"RESET" Starting standalone core server...\n");
    api_plugin_init();
    server_start(443, MODE_SECURE, api_plugin_handler); // Subirá porta 443 e 80 (redirect)
    return 0;
}
