#ifndef API_H
#define API_H

#include "server.h"

// Define como o servidor vai rodar: MODE_SECURE (HTTPS na 443 + Redirect na 80) ou MODE_INSECURE (HTTP)
#define OPERATION_MODE MODE_SECURE
#define SERVER_PORT (OPERATION_MODE == MODE_SECURE ? 443 : 8080)

// Assinatura de um manipulador de rotas específico da aplicação
typedef void (*RouteHandler)(ClientConnection *conn, HttpRequest *req);

// Estrutura de uma rota
typedef struct Route {
    char path[256];
    char method[16];
    RouteHandler handler;
    struct Route *next;
} Route;

/**
 * Adiciona uma nova rota na aplicação.
 */
void add_route(const char *path, const char *method, RouteHandler handler);

/**
 * Utilitário da aplicação para enviar uma página baseada no nome da pasta.
 */
void send_page(ClientConnection *conn, const char *folder_name, const char *request_path);

/**
 * Inicializa a API, cadastra rotas e inicia o servidor.
 */
void api_init();

#endif // API_H
