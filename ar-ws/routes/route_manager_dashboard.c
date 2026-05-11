#include "../routes.h"
#include "../core/user_manager.h"
#include <string.h>

void manager_dashboard_handler(ClientConnection *conn, HttpRequest *req) {
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
    char *final_token = admin_token[0] != '\0' ? admin_token : NULL;
    int role_trash = 2;
    int auth_status = is_valid_admin_session(final_token, server_get_client_ip(conn), &role_trash);
    
    if (auth_status != 1) {
        server_add_header(conn, "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
        server_redirect(conn, "/manager/login");
        return; 
    }
    
    sendpage(conn, "manager/dashboard");
}
