#include "../routes.h"
#include "../core/utils.h"
#include "../core/tty.h"
#include "../core/user_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void tty_text_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }

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
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    clear_tty();
    server_send_response(conn, 200, "application/json", "{\"message\":\"Tela do servidor limpa.\"}");
}

void tty_logo_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }

    const char *b64_logo = "ICQkJCQkJFwgICQkJCQkJCRcICAgJCQkJCQkXCAgICQkJCQkJFwgICQkJCQkJCRcICQkJCQkJFwgCiQkICBfXyQkXCAkJCAgX18kJFwgJCQgIF9fJCRcICQkICBfXyQkXCAkJCAgX18kJFxcXyQkICBffAokJCAvICAkJCB8JCQgfCAgJCQgfCQkIC8gIFxfX3wkJCAvICAkJCB8JCQgfCAgJCQgfCAkJCB8ICAKJCQkJCQkJCQgfCQkJCQkJCQgIHwkJCB8ICAgICAgJCQkJCQkJCQgfCQkJCQkJCQgIHwgJCQgfCAgCiQkICBfXyQkIHwkJCAgX18kJDwgJCQgfCAgICAgICQkICBfXyQkIHwkJCAgX19fXy8gICQkIHwgIAokJCB8ICAkJCB8JCQgfCAgJCQgfCQkIHwgICQkXCAkJCB8ICAkJCB8JCQgfCAgICAgICAkJCB8ICAKJCQgfCAgJCQgfCQkIHwgICQkIHxcJCQkJCQkICB8JCQgfCAgJCQgfCQkIHwgICAgICQkJCQkJFwgClxfX3wgIFxfX3xcX198ICBcX198IFxfX19fX18vIFxfX3wgIFxfX3xcX198ICAgICBcX19fX19ffAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIA==";

    char decoded[1024];
    decode_b64(b64_logo, decoded);

    char *line = decoded;
    for (char *p = decoded; *p; p++) {
        if (*p == '\n') {
            *p = '\0';
            tty_direct_write("\033[1;31m%s\033[0m\n", line);
            fflush(stdout);
#ifdef _WIN32
            Sleep(30);
#else
            usleep(30000);
#endif
            line = p + 1;
        }
    }
    if (*line) {
        tty_direct_write("\033[1;31m%s\033[0m\n", line);
    }

    server_send_response(conn, 200, "application/json", "{\"message\":\"Animacao de Logo acionada!\"}");
}
