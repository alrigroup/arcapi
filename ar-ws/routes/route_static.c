#include "../routes.h"
#include "../core/logs.h"
#include <string.h>
#include <stdio.h>

void static_handler(ClientConnection *conn, HttpRequest *req) {
    char filepath[512];
    char clean_path[512];
    
    // Remove query string (?v=2 etc) do path
    strncpy(clean_path, req->path, sizeof(clean_path) - 1);
    char *q = strchr(clean_path, '?');
    if (q) *q = '\0';
    
    // Constrói o caminho relativo à pasta web
    snprintf(filepath, sizeof(filepath), "ar-ws/web%s", clean_path);
    
    // Determina o Content-Type baseado na extensão
    const char *ext = strrchr(clean_path, '.');
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

    // Tenta servir o arquivo. Se não existir, retorna 404 com página estilizada.
    if (!server_serve_file(conn, filepath, content_type)) {
        const char *host = req->host[0] ? req->host : "(sem host)";
        arc_log("ARC-DENIED", "Acesso negado | IP=%s URL:%s | Path=%s", server_get_client_ip(conn), host, clean_path);
        server_send_404(conn);
    }
}
