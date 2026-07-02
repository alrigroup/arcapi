#include "../ar-bemf/server.h"
#include "../ar-bemf/router.h"
#include "endpoints.h"
#include "core/database.h"
#include "core/logs.h"
#include "core/user_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void app_request_handler(ClientConnection *conn, HttpRequest *req) {
    track_route(req->path);
    
    req->admin_role = 2;
    req->admin_user[0] = '\0';

    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid == 1) {
        req->admin_role = auth.role;
        strncpy(req->admin_user, auth.user, sizeof(req->admin_user) - 1);
        req->admin_user[sizeof(req->admin_user) - 1] = '\0';
    }

    router_dispatch(conn, req);
}

int main() {
    // 1. Initialize Core Systems
    init_db_json();
    
    arc_log("INFO", "ALRI CWB Ecosystem Starting...");
    
    // 2. Bootstrap: cria admin inicial via env var se vazio
    const char *admin_hash = getenv("ADMIN_PASS_HASH");
    if (admin_hash && admin_hash[0] != '\0') {
        cJSON *existing = db_get_all_users();
        if (existing) {
            if (cJSON_GetArraySize(existing) == 0) {
                db_add_user("admin", admin_hash, 0);
                arc_log("INFO", "Initial admin user created from ADMIN_PASS_HASH.");
            }
            cJSON_Delete(existing);
        }
    }
    
    // 3. Register all endpoints (Business Logic)
    register_all_endpoints();
    
    // 4. Configure Agnostic Framework
    server_set_logger(track_access);
    
    // 5. Start the Server
    server_start(443, MODE_SECURE, app_request_handler);
    
    return 0;
}
