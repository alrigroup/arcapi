#include "endpoints.h"
#include "routes.h"
#include "../ar-bemf/router.h"
#include "core/tty.h"
#include <stdio.h>

#define CYAN "\033[1;36m"
#define RESET "\033[0m"

void register_all_endpoints() {
    alri_print_force(CYAN"[API]" RESET " Initializing routes...\n");

    // Pages
    add_route("/", "GET", home_handler);
    add_route("/home", "GET", home_handler);
    add_route("/manager/login", "GET", manager_login_handler);
    add_route("/manager/dashboard", "GET", manager_dashboard_handler);
    
    // Components (Wildcard Route)
    add_route("/manager/api/component/*", "GET", api_component_handler);

    // Data & Auth API
    add_route("/manager/api/login", "POST", api_login_handler);
    add_route("/manager/api/logout", "POST", api_logout_handler);
    add_route("/manager/api/metrics", "GET", api_metrics_handler);
    add_route("/manager/api/ips", "GET", api_ips_handler);
    add_route("/manager/api/logs", "GET", api_logs_handler);
    
    // Admin API
    add_route("/manager/api/admin/list", "GET", api_admin_list_handler);
    add_route("/manager/api/admin/create", "POST", api_admin_create_handler);
    add_route("/manager/api/admin/role", "POST", api_admin_role_handler);
    add_route("/manager/api/admin/delete", "POST", api_admin_delete_handler);
    add_route("/manager/api/admin/audit", "GET", api_admin_audit_handler);
    add_route("/manager/api/admin/audit/clear", "POST", api_admin_audit_clear_handler);
    add_route("/manager/api/admin/update", "POST", api_admin_update_handler);
    add_route("/manager/api/sync/batch", "POST", api_sync_batch_handler);

    // System API
    add_route("/manager/api/config", "GET", api_config_get_handler);
    add_route("/manager/api/config/tty", "POST", api_config_tty_handler);
    add_route("/manager/api/system/restart", "POST", api_system_restart_handler);
    add_route("/manager/api/hashes", "GET", api_hashes_handler);
    add_route("/manager/api/upload", "POST", api_upload_handler);
    add_route("/manager/api/delete", "POST", api_delete_handler);

    add_route("/api/data", "GET", api_data_handler);

    // TTY Commands
    add_route("/manager/api/tty/text", "POST", tty_text_handler);
    add_route("/manager/api/tty/clear", "POST", tty_clear_handler);
    add_route("/manager/api/tty/logo", "POST", tty_logo_handler);
    
    // Static Fallback (Catch-all for CSS, JS, Images)
    add_route("/*", "GET", static_handler);
}
