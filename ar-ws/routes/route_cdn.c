#include "../routes.h"
#include "../core/database.h"
#include "../core/logs.h"
#include "../core/utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

static const char* get_mime_type(const char *ext) {
    if (!ext || !*ext) return "application/octet-stream";
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "js") == 0) return "application/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcasecmp(ext, "ico") == 0) return "image/x-icon";
    if (strcasecmp(ext, "webp") == 0) return "image/webp";
    if (strcasecmp(ext, "avif") == 0) return "image/avif";
    if (strcasecmp(ext, "mp4") == 0) return "video/mp4";
    if (strcasecmp(ext, "webm") == 0) return "video/webm";
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "woff") == 0) return "font/woff";
    if (strcasecmp(ext, "woff2") == 0) return "font/woff2";
    if (strcasecmp(ext, "ttf") == 0) return "font/ttf";
    if (strcasecmp(ext, "otf") == 0) return "font/otf";
    if (strcasecmp(ext, "xml") == 0) return "application/xml";
    if (strcasecmp(ext, "zip") == 0) return "application/zip";
    if (strcasecmp(ext, "gz") == 0) return "application/gzip";
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    if (strcasecmp(ext, "csv") == 0) return "text/csv";
    return "application/octet-stream";
}

static void send_cdn_file(ClientConnection *conn, const char *filepath, const char *mime_type) {
    struct stat st;
    if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) {
        server_send_response(conn, 404, "application/json", "{\"error\":\"Not found\"}");
        return;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        server_send_response(conn, 500, "application/json", "{\"error\":\"Internal error\"}");
        return;
    }

    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: public, max-age=31536000\r\n"
        "Accept-Ranges: bytes\r\n"
        "Connection: keep-alive\r\n"
        "\r\n",
        mime_type, (long)st.st_size);

    server_conn_write(conn, header, hlen);

    char buf[32768];
    int n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        server_conn_write(conn, buf, n);
    }
    fclose(f);
}

void cdn_handler(ClientConnection *conn, HttpRequest *req) {
    server_add_header(conn, "Access-Control-Allow-Origin: *\r\n");
    const char *ip = server_get_client_ip(conn);

    // req->path já vem URL-decodado pelo framework (handle_client em server.c)
    // Usar req->path diretamente para evitar double URL decoding bypass
    const char *decoded = req->path;

    if (strstr(decoded, "..") || strchr(decoded, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid path\"}");
        return;
    }

    if (strcmp(decoded, "/") == 0 || strcmp(decoded, "") == 0) {
        server_send_response(conn, 200, "application/json",
            "{\"message\":\"ALRI CDN\",\"status\":\"active\"}");
        return;
    }

    cJSON *file = db_cdn_lookup(decoded);
    if (!file) {
        server_send_response(conn, 404, "application/json", "{\"error\":\"Not found\"}");
        return;
    }

    cJSON *file_path_json = cJSON_GetObjectItem(file, "file_path");
    cJSON *id_json = cJSON_GetObjectItem(file, "id");
    cJSON *mime_json = cJSON_GetObjectItem(file, "mime_type");

    const char *file_path = file_path_json ? file_path_json->valuestring : NULL;
    const char *mime = mime_json ? mime_json->valuestring : "application/octet-stream";
    int id = id_json ? id_json->valueint : 0;

    if (!file_path) {
        cJSON_Delete(file);
        server_send_response(conn, 500, "application/json", "{\"error\":\"Internal error\"}");
        return;
    }

    send_cdn_file(conn, file_path, mime);

    if (id > 0) {
        db_cdn_log_download(id, ip);
    }

    cJSON_Delete(file);
}
