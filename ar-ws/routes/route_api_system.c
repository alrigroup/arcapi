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

void api_config_get_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *resp = cJSON_CreateObject();
    server_send_json(conn, 200, resp);
}

void api_system_restart_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *mode = cJSON_GetObjectItem(json, "mode");
        cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
        if (mode && cp && cJSON_IsString(mode) && cJSON_IsString(cp)) {
            if (!verify_sudo(auth.user, cp->valuestring)) {
                arc_log("WARN", "Failed sudo auth for system restart by user '%s'", auth.user);
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta.\"}");
            } else {
                arc_log("WARN", "User '%s' triggered system restart (%s)", auth.user, mode->valuestring);
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
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
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
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }

    const char *target_path_orig = get_header(req, "X-Target-Path");
    const char *restart_mode = get_header(req, "X-Restart-Mode");
    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    char *target_path = NULL;
    
    if (!target_path_orig || !restart_mode || strstr(target_path_orig, "..") || strchr(target_path_orig, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid parameters or Path Traversal attempt.\"}");
        return;
    }

    if (!confirm_pass || !verify_sudo(auth.user, confirm_pass)) {
        arc_log("WARN", "Failed sudo auth for file upload by user '%s'", auth.user);
        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
        return;
    }
    
    // Allowed root directories for upload
    int allowed = 0;
    if (strncmp(target_path_orig, "ar-core/", 8) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "ar-bemf/", 8) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "shared/", 7) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "ar-ws/", 6) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "web/", 4) == 0) {
        char new_path[512];
        snprintf(new_path, sizeof(new_path), "ar-ws/%s", target_path_orig);
        target_path = strdup(new_path);
        allowed = 1;
    }

    if (!allowed) {
        server_send_response(conn, 403, "application/json", "{\"error\": \"Target path outside allowed directories.\"}");
        return;
    }
    
    create_directories(target_path); 

    FILE *out = fopen(target_path, "wb");
    if (!out) {
        free(target_path != target_path_orig ? target_path : NULL);
        server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to open target file for writing.\"}");
        return;
    }

    const char *cl_str = get_header(req, "Content-Length");
    long content_length = 0;
    if (cl_str) {
        char *endptr;
        content_length = strtol(cl_str, &endptr, 10);
        if (*endptr != '\0' || content_length < 0) {
            fclose(out);
            remove(target_path);
            server_send_response(conn, 400, "application/json", "{\"error\":\"Invalid Content-Length.\"}");
            free(target_path != target_path_orig ? target_path : NULL);
            return;
        }
    }

    int body_in_buffer = req->body_length_in_buffer;
    
    if (body_in_buffer > 0) {
        int to_write = body_in_buffer > content_length ? content_length : body_in_buffer;
        fwrite(req->body, 1, to_write, out);
    }
    
    long remaining = content_length - body_in_buffer;
    char chunk[8192];
    while (remaining > 0) {
        int to_read = (int)(remaining > (long)sizeof(chunk) ? (long)sizeof(chunk) : remaining);
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
    
    arc_log("INFO", "User '%s' uploaded and synced file: %s", auth.user, target_path);
    server_send_response(conn, 200, "application/json", "{\"message\": \"Upload successful!\"}");
    free(target_path != target_path_orig ? target_path : NULL);
}

void api_delete_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }

    const char *target_path_orig = get_header(req, "X-Target-Path");
    const char *restart_mode = get_header(req, "X-Restart-Mode");
    const char *confirm_pass = get_header(req, "X-Confirm-Pass");
    char *target_path = NULL;
    
    if (!target_path_orig || !restart_mode || strstr(target_path_orig, "..") || strchr(target_path_orig, '%')) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid parameters or Path Traversal attempt.\"}");
        return;
    }

    if (!confirm_pass || !verify_sudo(auth.user, confirm_pass)) {
        arc_log("WARN", "Failed sudo auth for file delete by user '%s'", auth.user);
        server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta ou ausente.\"}");
        return;
    }
    
    // Allowed root directories for delete
    int allowed = 0;
    if (strncmp(target_path_orig, "ar-core/", 8) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "ar-bemf/", 8) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "shared/", 7) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "ar-ws/", 6) == 0) { allowed = 1; target_path = (char*)target_path_orig; }
    else if (strncmp(target_path_orig, "web/", 4) == 0) {
        char new_path[512];
        snprintf(new_path, sizeof(new_path), "ar-ws/%s", target_path_orig);
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
        arc_log("INFO", "User '%s' deleted file: %s", auth.user, target_path);
        server_send_response(conn, 200, "application/json", "{\"message\": \"File deleted successfully!\"}");
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to delete file or file not found.\"}");
    }
    free(target_path != target_path_orig ? target_path : NULL);
}

void api_sync_batch_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }

    long content_length = 0;
    const char *cl = get_header(req, "Content-Length");
    if (cl) {
        char *endptr;
        content_length = strtol(cl, &endptr, 10);
        if (*endptr != '\0' || content_length < 0) content_length = 0;
    }

    int status = process_batch_sync(conn, req, content_length);
    
    if (status == 200) {
        server_send_response(conn, 200, "application/json", "{\"status\": \"Batch sync completed successfully.\"}");
    } else if (status == 401) {
        server_send_response(conn, 401, "application/json", "{\"error\": \"Sudo auth failed.\"}");
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\": \"Internal server error during sync.\"}");
    }
}
