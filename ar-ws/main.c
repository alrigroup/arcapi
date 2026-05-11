#include "../ar-bemf/server.h"
#include "../ar-bemf/router.h"
#include "endpoints.h"
#include "core/database.h"
#include "core/logs.h"
#include "core/user_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void app_request_handler(ClientConnection *conn, HttpRequest *req) {
    track_route(req->path);
    
    // --- MIDDLEWARE: AUTHENTICATION & ZERO-TRUST ---
    req->admin_role = 2; // Guest/SUP fallback (Proteção contra Broken Access)
    req->admin_user[0] = '\0';

    const char *cookie_header = get_header(req, "Cookie");
    char admin_token[65] = {0};
    if (cookie_header) {
        char *adm_ptr = strstr(cookie_header, "arc_admin_token=");
        if (adm_ptr) {
            strncpy(admin_token, adm_ptr + 16, 64);
            admin_token[64] = '\0';
            char *semi = strchr(admin_token, ';');
            if (semi) *semi = '\0';
        }
    }
    
    const char *auth_header = get_header(req, "Authorization");
    char *final_token = NULL;
    if (auth_header && strncmp(auth_header, "Bearer ", 7) == 0) final_token = (char*)auth_header + 7;
    else if (admin_token[0] != '\0') final_token = admin_token;

    if (final_token) {
        int role = 2;
        int status = is_valid_admin_session(final_token, server_get_client_ip(conn), &role);
        if (status == 1) {
            req->admin_role = role;
            server_get_session_user(final_token, req->admin_user);
        } else if (status == -1) {
            if (strncmp(req->path, "/manager/api/", 13) == 0 && strcmp(req->path, "/manager/api/login") != 0) {
                server_send_response(conn, 503, "application/json", "{\"error\": \"Database offline.\"}");
                return;
            }
        }
    }

    router_dispatch(conn, req);
}

int main() {
    // 1. Initialize Core Systems
    init_db_json();
    
    arc_log("INFO", "ALRI CWB Ecosystem Starting...");
    
    // 2. Register all endpoints (Business Logic)
    register_all_endpoints();
    
    // 3. Configure Agnostic Framework
    server_set_logger(track_access);
    
    // 4. Start the Server
    server_start(443, MODE_SECURE, app_request_handler);
    
    return 0;
}
