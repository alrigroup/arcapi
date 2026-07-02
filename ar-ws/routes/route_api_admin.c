#include "../routes.h"
#include "../core/user_manager.h"
#include "../core/database.h"
#include "../core/utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void api_admin_list_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 1) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); 
        return; 
    }
    
    cJSON *arr = db_get_all_users();
    if (!arr) arr = cJSON_CreateArray();
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "admins", arr);
    server_send_json(conn, 200, resp);
}

void api_admin_create_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *u = cJSON_GetObjectItem(json, "user");
        cJSON *p = cJSON_GetObjectItem(json, "pass");
        cJSON *r = cJSON_GetObjectItem(json, "role");
        cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");

        if (u && p && r && cp && cJSON_IsString(u) && cJSON_IsString(p) && cJSON_IsNumber(r) && cJSON_IsString(cp)) {
            if (!verify_sudo(auth.user, cp->valuestring)) {
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha de confirmação inválida (Sudo Mode).\"}");
            } else {
                int success = db_add_user(u->valuestring, p->valuestring, r->valueint);
                
                if (success) {
                    char desc[256];
                    snprintf(desc, sizeof(desc), "User '%s' created with role %d", u->valuestring, r->valueint);
                    db_log_event("USER_CREATED", auth.user, desc);
                    server_send_response(conn, 200, "application/json", "{\"message\": \"Admin created successfully!\"}");
                } else {
                    server_send_response(conn, 400, "application/json", "{\"error\": \"Username already exists or Database error.\"}");
                }
            }
        } else {
            server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}");
        }
    } else {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON.\"}");
    }
}

void api_admin_delete_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *u = cJSON_GetObjectItem(json, "user");
        cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
        if (u && cp && cJSON_IsString(u) && cJSON_IsString(cp)) {
            if (!verify_sudo(auth.user, cp->valuestring)) {
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta.\"}");
            } else if (strcmp(u->valuestring, "admin") == 0) {
                server_send_response(conn, 403, "application/json", "{\"error\": \"Cannot delete master root admin.\"}");
            } else {
                int success = db_delete_user(u->valuestring);
                if (success) {
                    char desc[256];
                    snprintf(desc, sizeof(desc), "User '%s' deleted", u->valuestring);
                    db_log_event("USER_DELETED", auth.user, desc);
                    server_send_response(conn, 200, "application/json", "{\"message\": \"Admin deleted successfully!\"}");
                } else {
                    server_send_response(conn, 404, "application/json", "{\"error\": \"Admin not found or Database error.\"}");
                }
            }
        } else {
            server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}");
        }
    } else {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON.\"}");
    }
}

void api_admin_role_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 0) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: ROOT only.\"}"); 
        return; 
    }
    
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *u = cJSON_GetObjectItem(json, "user");
        cJSON *r = cJSON_GetObjectItem(json, "role");
        cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
        
        if (!u || !cJSON_IsString(u)) { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload: user missing or not a string.\"}"); }
        else if (!r || (!cJSON_IsNumber(r) && !cJSON_IsString(r))) { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload: role missing or not a number/string.\"}"); }
        else if (!cp || !cJSON_IsString(cp)) { server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload: confirm_pass missing or not a string.\"}"); }
        else {
            int target_role = cJSON_IsNumber(r) ? r->valueint : atoi(r->valuestring);
            
            if (!verify_sudo(auth.user, cp->valuestring)) {
                server_send_response(conn, 401, "application/json", "{\"error\": \"Sudo confirmation failed.\"}");
            } else if (strcmp(u->valuestring, "admin") == 0) {
                server_send_response(conn, 403, "application/json", "{\"error\": \"Cannot change role of master root admin.\"}");
            } else {
                int success = db_change_role(u->valuestring, target_role);
                if (success) {
                    char desc[256];
                    snprintf(desc, sizeof(desc), "Role of '%s' changed to %d", u->valuestring, target_role);
                    db_log_event("ROLE_CHANGED", auth.user, desc);
                    server_send_response(conn, 200, "application/json", "{\"message\": \"Admin role updated!\"}");
                } else {
                    server_send_response(conn, 404, "application/json", "{\"error\": \"Admin not found or Database error.\"}");
                }
            }
        }
    } else {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON payload.\"}");
    }
}

void api_admin_audit_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 1) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); 
        return; 
    }
    
    const char *f_type = get_query_param(req, "type");
    const char *f_user = get_query_param(req, "user");
    const char *f_ip = get_query_param(req, "ip");

    cJSON *arr = db_get_audit_logs_filtered(f_type, f_user, f_ip);
    if (!arr) arr = cJSON_CreateArray();
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "logs", arr);
    server_send_json(conn, 200, resp);
}

void api_admin_audit_clear_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 1) {
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden: Requires higher privileges.\"}");
        return;
    }

    cJSON *json = parse_json_body(req);
    if (!json) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
    }

    cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");
    if (!cp || !cJSON_IsString(cp)) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Sudo password required\"}");
        return;
    }

    if (!verify_sudo(auth.user, cp->valuestring)) {
        server_send_response(conn, 401, "application/json", "{\"error\": \"Invalid sudo password\"}");
        return;
    }

    FILE *audit = fopen("audit_archive.log", "a");
    if (audit) {
        time_t now = time(NULL);
        fprintf(audit, "[%ld] AUDIT_CLEAR by user '%s'\n", (long)now, auth.user);
        fclose(audit);
    }

    db_log_event("SECURITY_ALERT", auth.user, "Full Audit Log Cleared");

    if (db_clear_all_logs()) {
        server_send_response(conn, 200, "application/json", "{\"message\": \"Audit logs cleared successfully\"}");
    } else {
        server_send_response(conn, 500, "application/json", "{\"error\": \"Failed to clear logs\"}");
    }
}

void api_admin_update_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    
    if (auth.role > 1) { 
        server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); 
        return; 
    }
    
    cJSON *json = parse_json_body(req);
    if (json) {
        cJSON *u = cJSON_GetObjectItem(json, "user");
        cJSON *p = cJSON_GetObjectItem(json, "pass");
        cJSON *cp = cJSON_GetObjectItem(json, "confirm_pass");

        if (u && p && cp && cJSON_IsString(u) && cJSON_IsString(p) && cJSON_IsString(cp)) {
            if (!verify_sudo(auth.user, cp->valuestring)) {
                server_send_response(conn, 401, "application/json", "{\"error\": \"Senha sudo incorreta.\"}");
                return;
            }

            int target_role = db_get_user_role(u->valuestring);
            if (auth.role == 1 && target_role <= 1) {
                server_send_response(conn, 403, "application/json", "{\"error\": \"Permission denied to reset this user's password.\"}");
                return;
            }

            // Operação Atômica via DAO
            int updated = db_update_password(u->valuestring, p->valuestring);

            if (updated) {
                char desc[256];
                snprintf(desc, sizeof(desc), "Password of '%s' reset by '%s'", u->valuestring, auth.user);
                db_log_event("PASSWORD_RESET", auth.user, desc);
                server_send_response(conn, 200, "application/json", "{\"message\": \"Admin password updated!\"}");
            } else {
                server_send_response(conn, 404, "application/json", "{\"error\": \"Admin not found or Database error.\"}");
            }
        } else {
            server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid payload.\"}");
        }
    } else {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON.\"}");
    }
}
