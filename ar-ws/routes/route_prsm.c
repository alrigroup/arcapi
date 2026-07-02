#include "../routes.h"

void prsm_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "prsm");
}
