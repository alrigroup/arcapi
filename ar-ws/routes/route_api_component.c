#include "../routes.h"
#include "../core/user_manager.h"
#include <string.h>
#include <stdio.h>

void api_component_handler(ClientConnection *conn, HttpRequest *req) {
    const char *component = req->path + 23; // "/manager/api/component/" is 23 bytes long
    
    if (strstr(component, "..") || strchr(component, '/')) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid component\"}");
        return;
    }

    const char *cookie_header = get_header(req, "Cookie");
    char admin_token[65] = {0};
    if (cookie_header) {
        char *adm_ptr = strstr(cookie_header, "arc_admin_token=");
        if (adm_ptr) {
            strncpy(admin_token, adm_ptr + 16, 64);
            char *semi = strchr(admin_token, ';');
            if (semi) *semi = '\0';
        }
    }
    
    const char *auth_header = get_header(req, "Authorization");
    char *final_token = NULL;
    if (auth_header && strncmp(auth_header, "Bearer ", 7) == 0) final_token = (char*)auth_header + 7;
    else if (admin_token[0] != '\0') final_token = admin_token;
    
    int logged_in_role = 2;
    int auth_status = is_valid_admin_session(final_token, server_get_client_ip(conn), &logged_in_role);
    
    if (auth_status != 1) {
        server_add_header(conn, "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
        if (auth_status == -1) server_send_response(conn, 401, "application/json", "{\"error\": \"Sessão invalidada: IP mismatch.\"}");
        else server_send_response(conn, 401, "application/json", "{\"error\": \"Unauthorized\", \"message\": \"Token invalid or missing.\"}");
        return;
    }

    if (strcmp(component, "tab-tty") == 0 || strcmp(component, "tab-update") == 0 || strcmp(component, "tab-system") == 0) {
        if (logged_in_role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden\"}"); return; }
    } else if (strcmp(component, "tab-users") == 0) {
        if (logged_in_role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden\"}"); return; }
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "ar-ws/web/manager/dashboard/tabs/%s.html", component);
    if (!server_serve_file(conn, filepath, "text/html")) {
        server_send_response(conn, 404, "application/json", "{\"error\": \"Component not found\"}");
    }
}
