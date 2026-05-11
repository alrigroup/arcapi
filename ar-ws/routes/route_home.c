#include "../routes.h"

void home_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "home");
}
