#include "router.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ROUTE_HASH_SIZE 128

typedef struct Route {
    char path[256];
    char method[16];
    RequestHandler handler;
    struct Route *next;
} Route;

static Route *route_hash[ROUTE_HASH_SIZE] = {0};

static unsigned int hash_route(const char *path, const char *method) {
    unsigned int hash = 5381;
    int c;
    while ((c = *path++)) hash = ((hash << 5) + hash) + c;
    while ((c = *method++)) hash = ((hash << 5) + hash) + c;
    return hash % ROUTE_HASH_SIZE;
}

void add_route(const char *path, const char *method, RequestHandler handler) {
    Route *new_route = (Route *)malloc(sizeof(Route));
    strncpy(new_route->path, path, sizeof(new_route->path) - 1);
    strncpy(new_route->method, method, sizeof(new_route->method) - 1);
    new_route->handler = handler;
    
    unsigned int idx = hash_route(path, method);
    new_route->next = route_hash[idx];
    route_hash[idx] = new_route;
}

void router_dispatch(ClientConnection *conn, HttpRequest *req) {
    unsigned int idx = hash_route(req->path, req->method);
    Route *current = route_hash[idx];
    while (current != NULL) {
        if (strcmp(current->path, req->path) == 0 && strcmp(current->method, req->method) == 0) {
            current->handler(conn, req);
            return;
        }
        current = current->next;
    }
    
    // Check for wildcard routes
    for (int i = 0; i < ROUTE_HASH_SIZE; i++) {
        current = route_hash[i];
        while (current != NULL) {
            int len = strlen(current->path);
            if (len > 0 && current->path[len - 1] == '*') {
                if (strncmp(current->path, req->path, len - 1) == 0 && strcmp(current->method, req->method) == 0) {
                    current->handler(conn, req);
                    return;
                }
            }
            current = current->next;
        }
    }
    
    server_send_response(conn, 404, "text/html", "<h1>404 - Page Not Found</h1>");
}