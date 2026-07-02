#include "../routes.h"
#include "../core/database.h"
#include "../core/logs.h"
#include "../core/user_manager.h"
#include "../core/utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>



static const char* get_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

static int is_forbidden_ext(const char *path) {
    const char *ext = get_ext(path);
    if (!ext || ext[0] == '\0') return 0;
    const char *forbidden[] = {
        "php", "phtml", "php3", "php4", "php5", "php7", "phps",
        "jsp", "jspx", "asp", "aspx", "asa", "cer",
        "exe", "com", "bat", "cmd", "scr",
        "sh", "bash", "zsh",
        "py", "pl", "pm", "cgi", "rb",
        "htaccess", "htpasswd",
        "war", "jar",
        NULL
    };
    for (int i = 0; forbidden[i]; i++) {
        if (strcasecmp(ext, forbidden[i]) == 0) return 1;
    }
    return 0;
}

static void mkdir_recursive(const char *path) {
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static const char* detect_mime(const char *path) {
    const char *ext = get_ext(path);
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
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    if (strcasecmp(ext, "xml") == 0) return "application/xml";
    if (strcasecmp(ext, "zip") == 0) return "application/zip";
    return "application/octet-stream";
}

void cdn_admin_list_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    const char *dir = get_query_param(req, "dir");
    if (!dir) dir = "/";

    cJSON *list = db_cdn_list(dir);
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "dir", dir);
    if (list) {
        cJSON_AddItemToObject(resp, "files", list);
    } else {
        cJSON_AddItemToObject(resp, "files", cJSON_CreateArray());
    }
    server_send_json(conn, 200, resp);
}

void cdn_admin_upload_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    const char *target_path = get_header(req, "X-Target-Path");
    const char *confirm_pass = get_header(req, "X-Confirm-Pass");

    if (!target_path || strstr(target_path, "..") || strchr(target_path, '%') || target_path[0] != '/') {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid path\"}");
        return;
    }

    if (!confirm_pass || !verify_sudo(auth.user, confirm_pass)) {
        server_send_response(conn, 401, "application/json", "{\"error\":\"Sudo password required\"}");
        return;
    }

    char decoded_target[512];
    url_decode(decoded_target, target_path);

    if (is_forbidden_ext(decoded_target)) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Extension not allowed\"}");
        return;
    }

    cJSON *existing = db_cdn_lookup(decoded_target);
    if (existing) {
        const char *overwrite = get_header(req, "X-Overwrite");
        if (!overwrite || strcmp(overwrite, "true") != 0) {
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddStringToObject(resp, "error", "File already exists");
            cJSON_AddNumberToObject(resp, "existing_id", cJSON_GetObjectItem(existing, "id")->valueint);
            cJSON_AddNumberToObject(resp, "existing_size", cJSON_GetObjectItem(existing, "file_size")->valuedouble);
            cJSON_AddStringToObject(resp, "existing_mime", cJSON_GetObjectItem(existing, "mime_type")->valuestring);
            cJSON_AddNumberToObject(resp, "existing_downloads", cJSON_GetObjectItem(existing, "download_count")->valuedouble);
            server_send_json(conn, 409, resp);
            cJSON_Delete(existing);
            return;
        }
        cJSON *fp = cJSON_GetObjectItem(existing, "file_path");
        if (fp && fp->valuestring) remove(fp->valuestring);
        db_cdn_delete_by_path(decoded_target);
        cJSON_Delete(existing);
    }

    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "storage/cdn%s", decoded_target);

    char dir_copy[1024];
    strncpy(dir_copy, file_path, sizeof(dir_copy) - 1);
    char *dir = dirname(dir_copy);
    if (access(dir, F_OK) != 0) {
        mkdir_recursive(dir);
    }

    FILE *out = fopen(file_path, "wb");
    if (!out) {
        server_send_response(conn, 500, "application/json", "{\"error\":\"Failed to write file\"}");
        return;
    }

    const char *cl_str = get_header(req, "Content-Length");
    int content_length = cl_str ? atoi(cl_str) : 0;

    if (content_length > 100 * 1024 * 1024) {
        fclose(out);
        remove(file_path);
        server_send_response(conn, 413, "application/json", "{\"error\":\"File too large (max 100MB)\"}");
        return;
    }

    int body_in_buffer = req->body_length_in_buffer;

    if (body_in_buffer > 0) {
        int to_write = body_in_buffer > content_length ? content_length : body_in_buffer;
        fwrite(req->body, 1, to_write, out);
    }

    int remaining = content_length - body_in_buffer;
    char chunk[8192];
    while (remaining > 0) {
        int to_read = remaining > (int)sizeof(chunk) ? (int)sizeof(chunk) : remaining;
        int r = server_conn_read(conn, chunk, to_read);
        if (r <= 0) break;
        fwrite(chunk, 1, r, out);
        remaining -= r;
    }
    fclose(out);

    struct stat st;
    long fsize = 0;
    if (stat(file_path, &st) == 0) fsize = st.st_size;

    const char *mime = detect_mime(decoded_target);

    int id = db_cdn_register(decoded_target, file_path, fsize, mime, 0);
    if (id > 0) {
        db_log_event("CDN_UPLOAD", auth.user, decoded_target);
        arc_log("INFO", "CDN upload by '%s': %s -> %s", auth.user, decoded_target, file_path);
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "id", id);
        cJSON_AddStringToObject(resp, "path", decoded_target);
        cJSON_AddStringToObject(resp, "file_path", file_path);
        cJSON_AddNumberToObject(resp, "file_size", fsize);
        server_send_json(conn, 200, resp);
    } else {
        // DB failed, clean up file
        remove(file_path);
        server_send_response(conn, 500, "application/json", "{\"error\":\"Database registration failed\"}");
    }
}

void cdn_admin_link_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    cJSON *json = parse_json_body(req);
    if (!json) { server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid JSON\"}"); return; }

    cJSON *path_json = cJSON_GetObjectItem(json, "path");
    cJSON *file_path_json = cJSON_GetObjectItem(json, "file_path");

    if (!path_json || !file_path_json || !cJSON_IsString(path_json) || !cJSON_IsString(file_path_json) ||
        path_json->valuestring[0] != '/') {
        server_send_response(conn, 400, "application/json", "{\"error\":\"path and file_path required\"}");
        return;
    }

    const char *public_path = path_json->valuestring;
    const char *real_path = file_path_json->valuestring;

    if (strstr(public_path, "..") || strchr(public_path, '%') ||
        strstr(real_path, "..") || strchr(real_path, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid path\"}");
        return;
    }

    struct stat st;
    if (stat(real_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"File not found or not a regular file\"}");
        return;
    }

    const char *mime = detect_mime(public_path);
    char real_path_abs[1024] = {0};
#ifdef _WIN32
    _fullpath(real_path_abs, real_path, sizeof(real_path_abs) - 1);
#else
    if (realpath(real_path, real_path_abs) == NULL) {
        strncpy(real_path_abs, real_path, sizeof(real_path_abs) - 1);
    }
#endif
    if (real_path_abs[0] == '\0') {
        strncpy(real_path_abs, real_path, sizeof(real_path_abs) - 1);
    }

    int id = db_cdn_register(public_path, real_path_abs, st.st_size, mime, 1);
    if (id > 0) {
        db_log_event("CDN_LINK", auth.user, public_path);
        arc_log("INFO", "CDN link by '%s': %s -> %s", auth.user, public_path, real_path_abs);
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "id", id);
        cJSON_AddStringToObject(resp, "path", public_path);
        cJSON_AddStringToObject(resp, "file_path", real_path_abs);
        cJSON_AddNumberToObject(resp, "file_size", st.st_size);
        server_send_json(conn, 200, resp);
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\":\"Database registration failed\"}");
    }
}

void cdn_admin_delete_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    if (!confirm_pass || !verify_sudo(auth.user, confirm_pass)) {
        server_send_response(conn, 401, "application/json", "{\"error\":\"Sudo password required\"}");
        return;
    }

    cJSON *json = parse_json_body(req);
    if (!json) { server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid JSON\"}"); return; }

    cJSON *id_json = cJSON_GetObjectItem(json, "id");
    cJSON *path_json = cJSON_GetObjectItem(json, "path");
    cJSON *remove_file_json = cJSON_GetObjectItem(json, "remove_file");

    int remove_from_disk = remove_file_json && cJSON_IsBool(remove_file_json) && cJSON_IsTrue(remove_file_json);
    int deleted = 0;

    if (id_json && cJSON_IsNumber(id_json)) {
        int fid = id_json->valueint;

        if (remove_from_disk) {
            cJSON *f = db_cdn_lookup_by_id(fid);
            if (f) {
                cJSON *fp = cJSON_GetObjectItem(f, "file_path");
                cJSON *linked = cJSON_GetObjectItem(f, "is_linked");
                if (fp && fp->valuestring && (!linked || linked->valueint == 0)) {
                    remove(fp->valuestring);
                }
                cJSON_Delete(f);
            }
        }

        deleted = db_cdn_delete(fid);
        if (deleted) db_log_event("CDN_DELETE", auth.user, "id");
    } else if (path_json && cJSON_IsString(path_json)) {
        const char *p = path_json->valuestring;

        if (remove_from_disk) {
            cJSON *f = db_cdn_lookup(p);
            if (f) {
                cJSON *fp = cJSON_GetObjectItem(f, "file_path");
                cJSON *linked = cJSON_GetObjectItem(f, "is_linked");
                if (fp && fp->valuestring && (!linked || linked->valueint == 0)) {
                    remove(fp->valuestring);
                }
                cJSON_Delete(f);
            }
        }

        deleted = db_cdn_delete_by_path(p);
        if (deleted) db_log_event("CDN_DELETE", auth.user, p);
    }

    if (deleted) {
        arc_log("INFO", "CDN delete by '%s'", auth.user);
        server_send_response(conn, 200, "application/json", "{\"message\":\"Deleted\"}");
    } else {
        server_send_response(conn, 404, "application/json", "{\"error\":\"Not found\"}");
    }
}

void cdn_admin_mkdir_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    if (!confirm_pass || !verify_sudo(auth.user, confirm_pass)) {
        server_send_response(conn, 401, "application/json", "{\"error\":\"Sudo password required\"}");
        return;
    }

    cJSON *json = parse_json_body(req);
    if (!json) { server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid JSON\"}"); return; }

    cJSON *path_json = cJSON_GetObjectItem(json, "path");
    if (!path_json || !cJSON_IsString(path_json) || path_json->valuestring[0] != '/') {
        server_send_response(conn, 400, "application/json", "{\"error\":\"path required (starting with /)\"}");
        return;
    }

    const char *p = path_json->valuestring;
    if (strstr(p, "..") || strchr(p, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid path\"}");
        return;
    }

    char disk_path[1024];
    snprintf(disk_path, sizeof(disk_path), "storage/cdn%s", p);

    mkdir_recursive(disk_path);

    db_log_event("CDN_MKDIR", auth.user, p);
    arc_log("INFO", "CDN mkdir by '%s': %s", auth.user, p);
    server_send_response(conn, 200, "application/json", "{\"message\":\"Directory created\"}");
}

void cdn_admin_rename_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    if (!confirm_pass || !verify_sudo(auth.user, confirm_pass)) {
        server_send_response(conn, 401, "application/json", "{\"error\":\"Sudo password required\"}");
        return;
    }

    cJSON *json = parse_json_body(req);
    if (!json) { server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid JSON\"}"); return; }

    cJSON *id_json = cJSON_GetObjectItem(json, "id");
    cJSON *new_path_json = cJSON_GetObjectItem(json, "new_path");

    if (!id_json || !cJSON_IsNumber(id_json) || !new_path_json || !cJSON_IsString(new_path_json) ||
        new_path_json->valuestring[0] != '/') {
        server_send_response(conn, 400, "application/json", "{\"error\":\"id and new_path required (new_path must start with /)\"}");
        return;
    }

    int fid = id_json->valueint;
    char new_path[1024];
    strncpy(new_path, new_path_json->valuestring, sizeof(new_path) - 1);

    if (strstr(new_path, "..") || strchr(new_path, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid path\"}");
        return;
    }

    cJSON *file = db_cdn_lookup_by_id(fid);
    if (!file) {
        server_send_response(conn, 404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }

    cJSON *old_path_json = cJSON_GetObjectItem(file, "path");
    cJSON *old_file_path_json = cJSON_GetObjectItem(file, "file_path");
    cJSON *linked_json = cJSON_GetObjectItem(file, "is_linked");

    char old_path_buf[512] = {0};
    char old_file_path_buf[1024] = {0};
    if (old_path_json) strncpy(old_path_buf, old_path_json->valuestring, sizeof(old_path_buf) - 1);
    if (old_file_path_json) strncpy(old_file_path_buf, old_file_path_json->valuestring, sizeof(old_file_path_buf) - 1);
    int is_linked = linked_json ? linked_json->valueint : 0;

    char new_file_path[1024] = {0};
    if (!is_linked && old_file_path_buf[0] != '\0') {
        snprintf(new_file_path, sizeof(new_file_path), "storage/cdn%s", new_path);
    }

    if (!is_linked && old_file_path_buf[0] != '\0' && access(old_file_path_buf, F_OK) == 0) {
        char dirc[1024];
        strncpy(dirc, new_file_path, sizeof(dirc) - 1);
        char *p = strrchr(dirc, '/');
        if (p) {
            *p = '\0';
            mkdir_recursive(dirc);
        }

        if (rename(old_file_path_buf, new_file_path) != 0) {
            cJSON_Delete(file);
            server_send_response(conn, 500, "application/json", "{\"error\":\"Failed to move file on disk\"}");
            return;
        }
    }

    int updated = db_cdn_rename(fid, new_path, new_file_path[0] ? new_file_path : NULL);
    cJSON_Delete(file);

    if (updated) {
        db_log_event("CDN_RENAME", auth.user, old_path_buf);
        arc_log("INFO", "CDN rename by '%s': %s -> %s", auth.user, old_path_buf, new_path);
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddStringToObject(resp, "message", "Renamed");
        cJSON_AddStringToObject(resp, "old_path", old_path_buf);
        cJSON_AddStringToObject(resp, "new_path", new_path);
        server_send_json(conn, 200, resp);
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\":\"Database update failed\"}");
    }
}

void cdn_admin_stats_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\":\"Forbidden\"}"); return; }

    const char *id_str = get_query_param(req, "id");
    if (!id_str) { server_send_response(conn, 400, "application/json", "{\"error\":\"id required\"}"); return; }

    int id = atoi(id_str);
    cJSON *stats = db_cdn_get_stats(id);
    if (stats) {
        server_send_json(conn, 200, stats);
    } else {
        server_send_response(conn, 404, "application/json", "{\"error\":\"Not found\"}");
    }
}
