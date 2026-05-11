#include "../routes.h"
#include <string.h>
#include <stdio.h>

void static_handler(ClientConnection *conn, HttpRequest *req) {
    char filepath[512];
    
    // Constrói o caminho relativo à pasta web
    // req->path começa com '/', então concatenamos ar-ws/web + req->path
    snprintf(filepath, sizeof(filepath), "ar-ws/web%s", req->path);
    
    // Determina o Content-Type baseado na extensão
    const char *ext = strrchr(req->path, '.');
    const char *content_type = "text/plain";
    
    if (ext) {
        if (strcasecmp(ext, ".html") == 0) content_type = "text/html";
        else if (strcasecmp(ext, ".css") == 0) content_type = "text/css";
        else if (strcasecmp(ext, ".js") == 0) content_type = "application/javascript";
        else if (strcasecmp(ext, ".png") == 0) content_type = "image/png";
        else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) content_type = "image/jpeg";
        else if (strcasecmp(ext, ".gif") == 0) content_type = "image/gif";
        else if (strcasecmp(ext, ".svg") == 0) content_type = "image/svg+xml";
        else if (strcasecmp(ext, ".ico") == 0) content_type = "image/x-icon";
        else if (strcasecmp(ext, ".json") == 0) content_type = "application/json";
    }

    // Tenta servir o arquivo. Se não existir, retorna 404.
    if (!server_serve_file(conn, filepath, content_type)) {
        // Se for uma requisição de diretório ou arquivo inexistente
        server_send_response(conn, 404, "text/plain", "Asset Not Found");
    }
}
