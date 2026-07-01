#include "../routes.h"
#include "../core/user_manager.h"
#include "../core/database.h"
#include "../core/logs.h"
#include "../core/update.h"
#include "../core/sync_engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static int check_admin_auth(ClientConnection *conn, HttpRequest *req, int *out_role, char *out_user) {
    const char *cookie_header = get_header(req, "Cookie");
    char admin_token[65] = {0};
    if (cookie_header) {
        char *adm_ptr = strstr(cookie_header, "arc_admin_token=");
        if (adm_ptr) {
            strncpy(admin_token, adm_ptr + 16, 64);
            char *semi = strchr(admin_token, ';');
            if (semi) *semi = '\0';
        }
    }
    
    const char *auth_header = get_header(req, "Authorization");
    char *final_token = NULL;
    if (auth_header && strncmp(auth_header, "Bearer ", 7) == 0) final_token = (char*)auth_header + 7;
    else if (admin_token[0] != '\0') final_token = admin_token;
    
    int logged_in_role = 2;
    int auth_status = is_valid_admin_session(final_token, server_get_client_ip(conn), &logged_in_role);
    
    if (auth_status != 1) {
        server_add_header(conn, "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
        if (auth_status == -1) server_send_response(conn, 401, "application/json", "{\"error\": \"Sessão invalidada: IP mismatch.\"}");
        else server_send_response(conn, 401, "application/json", "{\"error\": \"Unauthorized\", \"message\": \"Token invalid or missing.\"}");
        return 0;
    }
    
    if (out_role) *out_role = logged_in_role;
    if (out_user) {
        server_get_session_user(final_token, out_user);
    }
    return 1;
}

void api_config_get_handler(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role;
    if (!check_admin_auth(conn, req, &logged_in_role, NULL)) return;
    
    if (logged_in_role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *resp = cJSON_CreateObject();
    server_send_json(conn, 200, resp);
}

void api_system_restart_handler(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role;
    char logged_in_user[64] = {0};
    if (!check_admin_auth(conn, req, &logged_in_role, logged_in_user)) return;
    
    if (logged_in_role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *mode = cJSON_GetObjectItem(json, "mode");
        cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
        if (mode && cp && cJSON_IsString(mode) && cJSON_IsString(cp)) {
            if (!verify_sudo(logged_in_user, cp->valuestring)) {
                arc_log("WARN", "Failed sudo auth for system restart by user '%s'", logged_in_user);
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta.\"}");
            } else {
                arc_log("WARN", "User '%s' triggered system restart (%s)", logged_in_user, mode->valuestring);
                if (strcmp(mode->valuestring, "api") == 0) {
                    server_send_response(conn, 200, "application/json", "{\"message\": \"Restarting API...\"}");
                    kill(getppid(), SIGUSR1);
                } else if (strcmp(mode->valuestring, "core") == 0) {
                    server_send_response(conn, 200, "application/json", "{\"message\": \"Restarting Core...\"}");
                    kill(getppid(), SIGUSR2);
                } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid mode.\"}"); }
            }
        } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}"); }
    } else { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON.\"}"); }
}

void api_hashes_handler(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role;
    if (!check_admin_auth(conn, req, &logged_in_role, NULL)) return;
    
    if (logged_in_role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *resp = cJSON_CreateObject();
    cJSON *files_obj = cJSON_AddObjectToObject(resp, "files");
    
    // Scan ALL modular components
    scan_files_to_hashes("ar-core", files_obj);
    scan_files_to_hashes("ar-bemf", files_obj);
    scan_files_to_hashes("shared", files_obj);
    scan_files_to_hashes("ar-ws", files_obj);
    
    server_send_json(conn, 200, resp);
}

void api_upload_handler(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role;
    char logged_in_user[64] = {0};
    if (!check_admin_auth(conn, req, &logged_in_role, logged_in_user)) return;
    
    if (logged_in_role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }

    const char *target_path = get_header(req, "X-Target-Path");
    const char *restart_mode = get_header(req, "X-Restart-Mode");
    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    
    if (!target_path || !restart_mode || strstr(target_path, "..") || strchr(target_path, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid parameters or Path Traversal attempt.\"}");
        return;
    }

    if (!confirm_pass || !verify_sudo(logged_in_user, confirm_pass)) {
        arc_log("WARN", "Failed sudo auth for file upload by user '%s'", logged_in_user);
        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
        return;
    }
    
    // Allowed root directories for upload
    int allowed = 0;
    if (strncmp(target_path, "ar-core/", 8) == 0) allowed = 1;
    else if (strncmp(target_path, "ar-bemf/", 8) == 0) allowed = 1;
    else if (strncmp(target_path, "shared/", 7) == 0) allowed = 1;
    else if (strncmp(target_path, "ar-ws/", 6) == 0) allowed = 1;
    else if (strncmp(target_path, "web/", 4) == 0) {
        // Compatibilidade com dashboard antigo que manda "web/..."
        // Redirecionamos para ar-ws/web/...
        char new_path[512];
        snprintf(new_path, sizeof(new_path), "ar-ws/%s", target_path);
        target_path = strdup(new_path); // Note: Simple leak for brevity in this context, but safe for one-off restart
        allowed = 1;
    }

    if (!allowed) {
        server_send_response(conn, 403, "application/json", "{\"error\": \"Target path outside allowed directories.\"}");
        return;
    }
    
    create_directories(target_path); 

    FILE *out = fopen(target_path, "wb");
    if (!out) {
        server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to open target file for writing.\"}");
        return;
    }

    const char *cl_str = get_header(req, "Content-Length");
    int content_length = cl_str ? atoi(cl_str) : 0;

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
    
    if (strcmp(restart_mode, "api") == 0) {
        kill(getppid(), SIGUSR1);
    } else if (strcmp(restart_mode, "core") == 0) {
        kill(getppid(), SIGUSR2);
    }
    
    arc_log("INFO", "User '%s' uploaded and synced file: %s", logged_in_user, target_path);
    server_send_response(conn, 200, "application/json", "{\"message\": \"Upload successful!\"}");
}

void api_delete_handler(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role;
    char logged_in_user[64] = {0};
    if (!check_admin_auth(conn, req, &logged_in_role, logged_in_user)) return;
    
    if (logged_in_role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }

    const char *target_path = get_header(req, "X-Target-Path");
    const char *restart_mode = get_header(req, "X-Restart-Mode");
    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    
    if (!target_path || !restart_mode || strstr(target_path, "..") || strchr(target_path, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid parameters or Path Traversal attempt.\"}");
        return;
    }

    if (!confirm_pass || !verify_sudo(logged_in_user, confirm_pass)) {
        arc_log("WARN", "Failed sudo auth for file delete by user '%s'", logged_in_user);
        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
        return;
    }
    
    // Allowed root directories for delete
    int allowed = 0;
    if (strncmp(target_path, "ar-core/", 8) == 0) allowed = 1;
    else if (strncmp(target_path, "ar-bemf/", 8) == 0) allowed = 1;
    else if (strncmp(target_path, "shared/", 7) == 0) allowed = 1;
    else if (strncmp(target_path, "ar-ws/", 6) == 0) allowed = 1;
    else if (strncmp(target_path, "web/", 4) == 0) {
        char new_path[512];
        snprintf(new_path, sizeof(new_path), "ar-ws/%s", target_path);
        target_path = strdup(new_path);
        allowed = 1;
    }

    if (!allowed) {
        server_send_response(conn, 403, "application/json", "{\"error\": \"Target path outside allowed directories.\"}");
        return;
    }
    
    if (remove(target_path) == 0) {
        if (strcmp(restart_mode, "api") == 0) kill(getppid(), SIGUSR1);
        else if (strcmp(restart_mode, "core") == 0) kill(getppid(), SIGUSR2);
        arc_log("INFO", "User '%s' deleted file: %s", logged_in_user, target_path);
        server_send_response(conn, 200, "application/json", "{\"message\": \"File deleted successfully!\"}");
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to delete file or file not found.\"}");
    }
}

void api_sync_batch_handler(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role;
    if (!check_admin_auth(conn, req, &logged_in_role, NULL)) return;
    
    if (logged_in_role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }

    int content_length = 0;
    const char *cl = get_header(req, "Content-Length");
    if (cl) content_length = atoi(cl);

    int status = process_batch_sync(conn, req, content_length);
    
    if (status == 200) {
        server_send_response(conn, 200, "application/json", "{\"status\": \"Batch sync completed successfully.\"}");
    } else if (status == 401) {
        server_send_response(conn, 401, "application/json", "{\"error\": \"Sudo auth failed.\"}");
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\": \"Internal server error during sync.\"}");
    }
}
