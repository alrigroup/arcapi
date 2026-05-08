#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define WEB_BASE_PATH "web"

// ------------------------------------------------------------------
// Opaque structure implementation
// ------------------------------------------------------------------
struct ClientConnection {
    int socket_fd;
    SSL *ssl;
    int mode; // MODE_SECURE or MODE_INSECURE
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
// Internal helper: Map allowed paths
// ------------------------------------------------------------------
static void add_allowed_path(const char *path) {
    AllowedPath *new_path = (AllowedPath *)malloc(sizeof(AllowedPath));
    if (!new_path) return;
    strncpy(new_path->path, path, sizeof(new_path->path) - 1);
    new_path->path[sizeof(new_path->path) - 1] = '\0';
    new_path->next = allowed_paths;
    allowed_paths = new_path;
    printf(PURPLE"[ARC-SERVER-MAP]"RESET" Indexed: [%s]\n", path);
}

static int is_path_allowed(const char *path) {
    if (strstr(path, "..") != NULL) {
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
        printf(RED"[ARC-SERVER-ERR]"RESET" Could not open directory: %s\n", dir_name);
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
             "Connection: close\r\n\r\n",
             status, status_text, content_type, body_len);
             
    conn_write(conn, headers, strlen(headers));
    if (body) {
        conn_write(conn, body, body_len);
    }
}

int server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type) {
    if (!is_path_allowed(filepath)) {
        printf(RED"[ARC-SERVER]"RESET" Access denied (Path Traversal): %s\n", filepath);
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
             "Connection: close\r\n\r\n",
             content_type, fsize);
             
    conn_write(conn, headers, strlen(headers));
    
    char chunk[8192];
    size_t bytes_read;
    while ((bytes_read = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        conn_write(conn, chunk, bytes_read);
    }
    
    fclose(f);
    return 1;
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
        perror("Failed redirector socket");
        return NULL;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(80);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error opening port 80");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 100) < 0) {
        perror("Error on port 80 Listen");
        close(server_fd);
        return NULL;
    }

    printf(CYAN"[ARC-REDIRECTOR]"RESET" Running on port 80, redirecting to HTTPS...\n");

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            continue;
        }
        
        read(client_socket, buffer, sizeof(buffer));

        char *response = "HTTP/1.1 301 Moved Permanently\r\n"
                         "Location: https://10.7.7.7/\r\n" 
                         "Content-Length: 0\r\n"
                         "Connection: close\r\n\r\n";

        write(client_socket, response, strlen(response));
        close(client_socket);
    }
    return NULL;
}

// ------------------------------------------------------------------
// Main Worker Thread
// ------------------------------------------------------------------
static void handle_client(ClientConnection *conn) {
    char buffer[4096] = {0};
    int bytes_read = conn_read(conn, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) return;
    
    HttpRequest req;
    memset(&req, 0, sizeof(HttpRequest));
    
    char *method = strtok(buffer, " \t\r\n");
    char *full_path = strtok(NULL, " \t\r\n");
    
    if (!method || !full_path) {
        server_send_response(conn, 400, "text/plain", "Bad Request");
        return;
    }
    
    req.method = method;
    
    char *query = strchr(full_path, '?');
    if (query) {
        *query = '\0'; 
        req.query_params = query + 1;
    }
    req.path = full_path;
    
    // Pass control to the API layer to process the route
    if (global_api_handler) {
        global_api_handler(conn, &req);
    } else {
        server_send_response(conn, 500, "text/plain", "API not properly initialized");
    }
}

typedef struct {
    int client_sock;
    SSL_CTX *ctx;
    int mode;
} ClientThreadArgs;

static void* client_thread(void *arg) {
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    
    ClientConnection conn;
    conn.socket_fd = args->client_sock;
    conn.mode = args->mode;
    conn.ssl = NULL;

    if (conn.mode == MODE_SECURE) {
        conn.ssl = SSL_new(args->ctx);
        SSL_set_fd(conn.ssl, conn.socket_fd);

        if (SSL_accept(conn.ssl) > 0) {
            handle_client(&conn);
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

    printf(PURPLE"[ARC-SERVER-MAP]"RESET" Scanning web directory...\n");
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
        perror("Error on main server bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 100) < 0) {
        perror("Error on main server listen");
        exit(EXIT_FAILURE);
    }

    if (mode == MODE_SECURE) {
        printf(GREEN"[ARC-SERVER]" RESET " HTTPS Server started on port %d!\n", port);
    } else {
        printf(GREEN"[ARC-SERVER]" RESET " HTTP Server started on port %d!\n", port);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock < 0) continue;

        ClientThreadArgs *args = (ClientThreadArgs *)malloc(sizeof(ClientThreadArgs));
        args->client_sock = client_sock;
        args->ctx = ctx;
        args->mode = mode;
        
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
