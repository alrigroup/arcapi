#include "../routes.h"
#include "../core/user_manager.h"
#include <string.h>
#include <stdio.h>

void api_component_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }

    const char *component = req->path + 23;
    
    if (strstr(component, "..") || strchr(component, '/')) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid component\"}");
        return;
    }

    if (strcmp(component, "tab-tty") == 0 || strcmp(component, "tab-update") == 0 || strcmp(component, "tab-system") == 0) {
        if (auth.role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden\"}"); return; }
    } else if (strcmp(component, "tab-users") == 0) {
        if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden\"}"); return; }
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "ar-ws/web/manager/dashboard/tabs/%s.html", component);
    server_add_header(conn, "Cache-Control: no-cache, must-revalidate\r\n");
    if (!server_serve_file(conn, filepath, "text/html")) {
        server_send_response(conn, 404, "application/json", "{\"error\": \"Component not found\"}");
    }
}
