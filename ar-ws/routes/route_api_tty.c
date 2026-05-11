#include "../routes.h"
#include "../core/utils.h"
#include "../core/tty.h"
#include "../core/user_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int is_admin(ClientConnection *conn, HttpRequest *req) {
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
    return 1;
}

void tty_text_handler(ClientConnection *conn, HttpRequest *req) {
    if (!is_admin(conn, req)) return;

    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *text_item = cJSON_GetObjectItem(json, "text");
        if (text_item && cJSON_IsString(text_item)) {
            char *decoded = malloc(strlen(text_item->valuestring) + 1);
            decode_b64(text_item->valuestring, decoded);
            
            for (int i = 0; decoded[i] != '\0'; i++) {
                if ((unsigned char)decoded[i] < 32 && decoded[i] != '\n') {
                    decoded[i] = ' '; 
                }
            }

            tty_direct_write("%s\n", decoded);
            server_send_response(conn, 200, "application/json", "{\"message\":\"Texto enviado para o log/TTY1!\"}");

            free(decoded);
        } else {
            server_send_response(conn, 400, "application/json", "{\"error\":\"Payload invalido.\"}");
        }
    } else {
        server_send_response(conn, 400, "application/json", "{\"error\":\"JSON invalido.\"}");
    }
}

void tty_clear_handler(ClientConnection *conn, HttpRequest *req) {
    if (!is_admin(conn, req)) return;
    clear_tty();
    server_send_response(conn, 200, "application/json", "{\"message\":\"Tela do servidor limpa.\"}");
}

void tty_logo_handler(ClientConnection *conn, HttpRequest *req) {
    if (!is_admin(conn, req)) return;
    system("echo 'ICQkJCQkJFwgICQkJCQkJCRcICAgJCQkJCQkXCAgICQkJCQkJFwgICQkJCQkJCRcICQkJCQkJFwgCiQkICBfXyQkXCAkJCAgX18kJFwgJCQgIF9fJCRcICQkICBfXyQkXCAkJCAgX18kJFxcXyQkICBffAokJCAvICAkJCB8JCQgfCAgJCQgfCQkIC8gIFxfX3wkJCAvICAkJCB8JCQgfCAgJCQgfCAkJCB8ICAKJCQkJCQkJCQgfCQkJCQkJCQgIHwkJCB8ICAgICAgJCQkJCQkJCQgfCQkJCQkJCQgIHwgJCQgfCAgCiQkICBfXyQkIHwkJCAgX18kJDwgJCQgfCAgICAgICQkICBfXyQkIHwkJCAgX19fXy8gICQkIHwgIAokJCB8ICAkJCB8JCQgfCAgJCQgfCQkIHwgICQkXCAkJCB8ICAkJCB8JCQgfCAgICAgICAkJCB8ICAKJCQgfCAgJCQgfCQkIHwgICQkIHxcJCQkJCQkICB8JCQgfCAgJCQgfCQkIHwgICAgICQkJCQkJFwgClxfX3wgIFxfX3xcX198ICBcX198IFxfX19fX18vIFxfX3wgIFxfX3xcX198ICAgICBcX19fX19ffAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIA==' | base64 -d | while read line; do echo -e \"\\e[1;31m$line\\e[0m\" > /dev/tty1; sleep 0.03; done &");
    server_send_response(conn, 200, "application/json", "{\"message\":\"Animacao de Logo acionada!\"}");
}
