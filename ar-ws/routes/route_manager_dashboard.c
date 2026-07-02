#include "../routes.h"
#include "../core/user_manager.h"
#include <string.h>

void manager_dashboard_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) {
        server_add_header(conn, "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Secure\r\n");
        server_redirect(conn, "/manager/login");
        return;
    }
    
    sendpage(conn, "manager/dashboard");
}
