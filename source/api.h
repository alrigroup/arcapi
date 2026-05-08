#ifndef API_H
#define API_H

#include "server.h"

// Defines how the server will run: MODE_SECURE (HTTPS on 443 + Redirect on 80) or MODE_INSECURE (HTTP)
#define OPERATION_MODE MODE_SECURE
#define SERVER_PORT (OPERATION_MODE == MODE_SECURE ? 443 : 8080)

// Application-specific route handler signature
typedef void (*RouteHandler)(ClientConnection *conn, HttpRequest *req);

// Route structure
typedef struct Route {
    char path[256];
    char method[16];
    RouteHandler handler;
    struct Route *next;
} Route;

/**
 * Adds a new route to the application.
 */
void add_route(const char *path, const char *method, RouteHandler handler);

/**
 * Application utility to send a page based on the folder name.
 */
void send_page(ClientConnection *conn, const char *folder_name, const char *request_path);

/**
 * Initializes the API, registers routes, and starts the server.
 */
void api_init();

#endif // API_H
