#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define ROUTE_HASH_SIZE 128
static Route *route_hash[ROUTE_HASH_SIZE] = {0};

// Função hash rápida (djb2)
static unsigned int hash_route(const char *path, const char *method) {
    unsigned int hash = 5381;
    int c;
    while ((c = *path++)) hash = ((hash << 5) + hash) + c;
    while ((c = *method++)) hash = ((hash << 5) + hash) + c;
    return hash % ROUTE_HASH_SIZE;
}

// ------------------------------------------------------------------
// Route Registration System
// ------------------------------------------------------------------
void add_route(const char *path, const char *method, RouteHandler handler) {
    Route *new_route = (Route *)malloc(sizeof(Route));
    strncpy(new_route->path, path, sizeof(new_route->path) - 1);
    strncpy(new_route->method, method, sizeof(new_route->method) - 1);
    new_route->handler = handler;
    
    unsigned int idx = hash_route(path, method);
    new_route->next = route_hash[idx];
    route_hash[idx] = new_route;
}

// ------------------------------------------------------------------
// Page Handling (Replaces old send_page)
// ------------------------------------------------------------------
void send_page(ClientConnection *conn, const char *folder_name, const char *request_path) {
    char full_path[512];
    
    if (strchr(request_path, '.') == NULL) {
        snprintf(full_path, sizeof(full_path), "web/%s/%s.html", folder_name, folder_name);
        server_serve_file(conn, full_path, "text/html");
    } else if (strstr(request_path, ".js")) {
        const char *filename = strrchr(request_path, '/');
        filename = filename ? filename + 1 : request_path;
        snprintf(full_path, sizeof(full_path), "web/%s/%s", folder_name, filename);
        server_serve_file(conn, full_path, "application/javascript");
    } else if (strstr(request_path, ".css")) {
        const char *filename = strrchr(request_path, '/');
        filename = filename ? filename + 1 : request_path;
        snprintf(full_path, sizeof(full_path), "web/%s/%s", folder_name, filename);
        server_serve_file(conn, full_path, "text/css");
    }
}

// ------------------------------------------------------------------
// Static Files and 404
// ------------------------------------------------------------------
static int serve_static_file(ClientConnection *conn, const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;
    
    char filepath[512];
    
    if (strncmp(path, "/home/assets/", 13) == 0) {
        snprintf(filepath, sizeof(filepath), "web/home/dist/assets/%s", path + 13);
    } else {
        const char *clean_path = (path[0] == '/') ? path + 1 : path;
        snprintf(filepath, sizeof(filepath), "web/%s", clean_path);
    }
    
    const char *content_type = "text/plain";
    if (strcmp(ext, ".html") == 0) content_type = "text/html";
    else if (strcmp(ext, ".css") == 0) content_type = "text/css";
    else if (strcmp(ext, ".js") == 0) content_type = "application/javascript";
    else if (strcmp(ext, ".png") == 0) content_type = "image/png";
    else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) content_type = "image/jpeg";
    
    return server_serve_file(conn, filepath, content_type);
}

static void route_404(ClientConnection *conn, HttpRequest *req) {
    server_send_response(conn, 404, "text/html", "<h1>404 - Page Not Found</h1>");
    alri_print(RED"[API]" RESET " Route not found: %s\n", req->path);
}

// ------------------------------------------------------------------
// Main API Dispatcher
// ------------------------------------------------------------------
void api_plugin_handler(ClientConnection *conn, HttpRequest *req) {
    // 1. Try to serve static file first
    if (serve_static_file(conn, req->path)) {
        return; 
    }
    
    // 2. Search in route hash table (Busca O(1))
    unsigned int idx = hash_route(req->path, req->method);
    Route *current = route_hash[idx];
    int found = 0;
    while (current != NULL) {
        if (strcmp(current->path, req->path) == 0 && strcmp(current->method, req->method) == 0) {
            current->handler(conn, req);
            found = 1;
            break;
        }
        current = current->next;
    }
    
    // 3. Fallback to 404 if not found
    if (!found) {
        route_404(conn, req);
    }
}

// ------------------------------------------------------------------
// Example Application Handlers
// ------------------------------------------------------------------
static void sendpage(ClientConnection *conn, const char *folder_name) {
    char full_path[512];
    
    // Try index.html directly in folder
    snprintf(full_path, sizeof(full_path), "web/%s/index.html", folder_name);
    if (access(full_path, F_OK) != 0) {
        // If not found, try main.html
        snprintf(full_path, sizeof(full_path), "web/%s/main.html", folder_name);
        if (access(full_path, F_OK) != 0) {
            // Fallback for SPA projects like Vite that use dist/ folder
            snprintf(full_path, sizeof(full_path), "web/%s/dist/index.html", folder_name);
        }
    }
    
    server_serve_file(conn, full_path, "text/html");
}

static void home_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "home");
}

static void api_data_handler(ClientConnection *conn, HttpRequest *req) {
    const char *json = "{\"status\": \"success\", \"message\": \"New C API running!\"}";
    server_send_response(conn, 200, "application/json", json);
}

// ------------------------------------------------------------------
// TTY Control System (E2EE/Base64 Secured)
// ------------------------------------------------------------------
static int b64_int(unsigned char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+') return 62;
    if (ch == '/') return 63;
    return 0;
}

static void decode_b64(const char* in, char* out) {
    int in_len = strlen(in);
    int i = 0, j = 0;
    while (i < in_len) {
        uint32_t a = in[i] == '=' ? 0 : b64_int(in[i]); i++;
        uint32_t b = in[i] == '=' ? 0 : b64_int(in[i]); i++;
        uint32_t c = in[i] == '=' ? 0 : b64_int(in[i]); i++;
        uint32_t d = in[i] == '=' ? 0 : b64_int(in[i]); i++;
        
        uint32_t trip = (a << 18) | (b << 12) | (c << 6) | d;
        out[j++] = (trip >> 16) & 0xFF;
        if (in[i-2] != '=') out[j++] = (trip >> 8) & 0xFF;
        if (in[i-1] != '=') out[j++] = trip & 0xFF;
    }
    out[j] = '\0';
}

static void tty_text_handler(ClientConnection *conn, HttpRequest *req) {
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *text_item = cJSON_GetObjectItem(json, "text");
        if (text_item && cJSON_IsString(text_item)) {
            char *decoded = malloc(strlen(text_item->valuestring) + 1);
            decode_b64(text_item->valuestring, decoded);
            
            // [FIX 1.1] Sanitização de Controle ANSI (Terminal Injection)
            for (int i = 0; decoded[i] != '\0'; i++) {
                if ((unsigned char)decoded[i] < 32 && decoded[i] != '\n') {
                    decoded[i] = ' '; // Substitui bytes de controle (ex: ESC) por espaço vazio
                }
            }

            alri_print("%s\n", decoded);
            server_send_response(conn, 200, "application/json", "{\"message\":\"Texto enviado para o log/TTY1!\"}");

            free(decoded);
        } else {
            server_send_response(conn, 400, "application/json", "{\"error\":\"Payload invalido.\"}");
        }
    } else {
        server_send_response(conn, 400, "application/json", "{\"error\":\"JSON invalido.\"}");
    }
}

static void tty_clear_handler(ClientConnection *conn, HttpRequest *req) {
    alri_print("\033[H\033[J"); // ANSI Clear Screen & Home
    server_send_response(conn, 200, "application/json", "{\"message\":\"Tela do servidor limpa.\"}");
}

static void tty_logo_handler(ClientConnection *conn, HttpRequest *req) {
    // Aciona externamente via system, aproveitando o design blindado do server.c
    // Roda em background para não travar a resposta da web request.
    system("echo 'ICQkJCQkJFwgICQkJCQkJCRcICAgJCQkJCQkXCAgICQkJCQkJFwgICQkJCQkJCRcICQkJCQkJFwgCiQkICBfXyQkXCAkJCAgX18kJFwgJCQgIF9fJCRcICQkICBfXyQkXCAkJCAgX18kJFxcXyQkICBffAokJCAvICAkJCB8JCQgfCAgJCQgfCQkIC8gIFxfX3wkJCAvICAkJCB8JCQgfCAgJCQgfCAkJCB8ICAKJCQkJCQkJCQgfCQkJCQkJCQgIHwkJCB8ICAgICAgJCQkJCQkJCQgfCQkJCQkJCQgIHwgJCQgfCAgCiQkICBfXyQkIHwkJCAgX18kJDwgJCQgfCAgICAgICQkICBfXyQkIHwkJCAgX19fXy8gICQkIHwgIAokJCB8ICAkJCB8JCQgfCAgJCQgfCQkIHwgICQkXCAkJCB8ICAkJCB8JCQgfCAgICAgICAkJCB8ICAKJCQgfCAgJCQgfCQkIHwgICQkIHxcJCQkJCQkICB8JCQgfCAgJCQgfCQkIHwgICAgICQkJCQkJFwgClxfX3wgIFxfX3xcX198ICBcX198IFxfX19fX18vIFxfX3wgIFxfX3xcX198ICAgICBcX19fX19ffAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIA==' | base64 -d | while read line; do echo -e \"\\e[1;31m$line\\e[0m\" > /dev/tty1; sleep 0.03; done &");
    
    server_send_response(conn, 200, "application/json", "{\"message\":\"Animacao de Logo acionada!\"}");
}

static void manager_login_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "manager/login");
}

static void manager_dashboard_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "manager/dashboard");
}

// ------------------------------------------------------------------
// Initialization
// ------------------------------------------------------------------
void api_plugin_init() {
    alri_print_force(CYAN"[API]" RESET " Initializing routes...\n");

    add_route("/", "GET", home_handler);
    add_route("/home", "GET", home_handler);
    add_route("/manager/login", "GET", manager_login_handler);
    add_route("/manager/dashboard", "GET", manager_dashboard_handler);
    add_route("/api/data", "GET", api_data_handler);
    add_route("/manager/api/tty/text", "POST", tty_text_handler);
    add_route("/manager/api/tty/clear", "POST", tty_clear_handler);
    add_route("/manager/api/tty/logo", "POST", tty_logo_handler);
}
