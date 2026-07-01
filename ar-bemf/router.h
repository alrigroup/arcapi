#ifndef ROUTER_H
#define ROUTER_H

#include "server.h"

void add_route(const char *path, const char *method, const char *domain, RequestHandler handler);
void router_dispatch(ClientConnection *conn, HttpRequest *req);

#endif // ROUTER_H