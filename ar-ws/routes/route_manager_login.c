#include "../routes.h"

void manager_login_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "manager/login");
}
