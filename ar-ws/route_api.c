#include "routes.h"

void api_data_handler(ClientConnection *conn, HttpRequest *req) {
    const char *json = "{\"status\": \"success\", \"message\": \"New C API running!\"}";
    server_send_response(conn, 200, "application/json", json);
}
