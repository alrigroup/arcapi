#include "router.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#define ROUTE_HASH_SIZE 128

typedef struct Route {
    char path[256];
    char method[16];
    char domain[128];          // "" = todos os domínios
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

void add_route(const char *path, const char *method, const char *domain, RequestHandler handler) {
    Route *new_route = (Route *)malloc(sizeof(Route));
    strncpy(new_route->path, path, sizeof(new_route->path) - 1);
    strncpy(new_route->method, method, sizeof(new_route->method) - 1);
    new_route->domain[0] = '\0';
    if (domain && domain[0] != '\0') {
        strncpy(new_route->domain, domain, sizeof(new_route->domain) - 1);
    }
    new_route->handler = handler;
    
    unsigned int idx = hash_route(path, method);
    new_route->next = route_hash[idx];
    route_hash[idx] = new_route;
}

static int domain_matches(const char *route_domain, const char *req_host) {
    if (route_domain[0] == '\0') return 1;
    if (req_host[0] == '\0') return 0;
    // Exact match
    if (strcasecmp(route_domain, req_host) == 0) return 1;
    // www.alrigroup.com → alrigroup.com
    if (strncasecmp(req_host, "www.", 4) == 0 && strcasecmp(route_domain, req_host + 4) == 0) return 1;
    if (strncasecmp(route_domain, "www.", 4) == 0 && strcasecmp(route_domain + 4, req_host) == 0) return 1;
    // Reject any other subdomain (strict whitelist)
    return 0;
}

void router_dispatch(ClientConnection *conn, HttpRequest *req) {
    unsigned int idx = hash_route(req->path, req->method);
    Route *current = route_hash[idx];
    while (current != NULL) {
        if (strcmp(current->path, req->path) == 0 && strcmp(current->method, req->method) == 0) {
            if (domain_matches(current->domain, req->host)) {
                current->handler(conn, req);
                return;
            }
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
                    if (domain_matches(current->domain, req->host)) {
                        current->handler(conn, req);
                        return;
                    }
                }
            }
            current = current->next;
        }
    }
    
    server_send_404(conn);
}