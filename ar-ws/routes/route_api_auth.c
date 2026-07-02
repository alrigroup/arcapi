#include "../routes.h"
#include "../core/user_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void api_login_handler(ClientConnection *conn, HttpRequest *req) {
    if (strcmp(req->method, "POST") != 0) {
        server_send_response(conn, 405, "application/json", "{\"error\": \"Method not allowed\"}");
        return;
    }

    cJSON *json = parse_json_body(req);
    if (!json) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON payload\"}");
        return;
    }
    
    cJSON *u = cJSON_GetObjectItem(json, "user");
    cJSON *p = cJSON_GetObjectItem(json, "pass");
    if (!u || !p || !cJSON_IsString(u) || !cJSON_IsString(p)) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Missing username or password hash\"}");
        return;
    }

    if (strlen(u->valuestring) > 64 || strlen(p->valuestring) > 128) {
        server_send_response(conn, 400, "application/json", "{\"error\":\"Field too long.\"}");
        return;
    }
    
    int logged_role = 2;
    if (server_validate_admin_login(u->valuestring, p->valuestring, server_get_client_ip(conn), &logged_role)) {
        const char *token = server_create_admin_session(u->valuestring, server_get_client_ip(conn));
        if (token) {
            char cookie_buf[1024];
            snprintf(cookie_buf, sizeof(cookie_buf), "Set-Cookie: arc_admin_token=%s; Path=/; HttpOnly; SameSite=Lax; Secure\r\n", token);
            server_add_header(conn, cookie_buf);
            
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddStringToObject(resp, "message", "Authenticated");
            cJSON_AddNumberToObject(resp, "role", logged_role);
            server_send_json(conn, 200, resp);
        } else {
            server_send_response(conn, 500, "application/json", "{\"error\": \"Session database error\"}");
        }
    } else {
        server_send_response(conn, 401, "application/json", "{\"error\": \"Invalid credentials or IP blocked\"}");
    }
}

void api_logout_handler(ClientConnection *conn, HttpRequest *req) {
    const char *cookie_header = get_header(req, "Cookie");
    if (cookie_header) {
        char *adm_ptr = strstr(cookie_header, "arc_admin_token=");
        if (adm_ptr) {
            char admin_token[65] = {0};
            strncpy(admin_token, adm_ptr + 16, 64);
            char *semi = strchr(admin_token, ';');
            if (semi) *semi = '\0';
            
            // Invalidação ativa em memória
            server_invalidate_admin_session(admin_token);
        }
    }
    
    server_add_header(conn, "Set-Cookie: arc_admin_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Secure\r\n");
    server_send_response(conn, 200, "application/json", "{\"message\": \"Logged out successfully\"}");
}
