#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Route *route_list = NULL;

// ------------------------------------------------------------------
// Route Registration System
// ------------------------------------------------------------------
void add_route(const char *path, const char *method, RouteHandler handler) {
    Route *new_route = (Route *)malloc(sizeof(Route));
    strncpy(new_route->path, path, sizeof(new_route->path) - 1);
    strncpy(new_route->method, method, sizeof(new_route->method) - 1);
    new_route->handler = handler;
    new_route->next = route_list;
    route_list = new_route;
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
    printf(RED"[API]" RESET " Route not found: %s\n", req->path);
}

// ------------------------------------------------------------------
// Main API Dispatcher
// ------------------------------------------------------------------
static void api_request_handler(ClientConnection *conn, HttpRequest *req) {
    // 1. Try to serve static file first
    if (serve_static_file(conn, req->path)) {
        return; 
    }
    
    // 2. Search in route list
    Route *current = route_list;
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
// Initialization
// ------------------------------------------------------------------
void api_init() {
    printf(CYAN"[API]" RESET " Initializing routes...\n");

    add_route("/", "GET", home_handler);
    add_route("/home", "GET", home_handler);
    add_route("/api/data", "GET", api_data_handler);

    printf(CYAN"[API]" RESET " Starting Core Server...\n");
    server_start(SERVER_PORT, OPERATION_MODE, api_request_handler);
}

// ------------------------------------------------------------------
// Standard Entry Point (main)
// ------------------------------------------------------------------
int main() {
    api_init();
    return 0;
}
