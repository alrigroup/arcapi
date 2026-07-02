#include "endpoints.h"
#include "routes.h"
#include "../ar-bemf/router.h"
#include "core/logs.h"
#include <stdio.h>

void register_all_endpoints() {
    arc_log("INFO", "Initializing routes...");

    const char *main_domain = "alrigroup.com";

    // Pages (restritas ao domínio principal + www)
    add_route("/", "GET", main_domain, home_handler);
    add_route("/home", "GET", main_domain, home_handler);
    add_route("/manager/login", "GET", main_domain, manager_login_handler);
    add_route("/manager/dashboard", "GET", main_domain, manager_dashboard_handler);
    
    // Components (Wildcard Route)
    add_route("/manager/api/component/*", "GET", main_domain, api_component_handler);

    // Data & Auth API
    add_route("/manager/api/login", "POST", main_domain, api_login_handler);
    add_route("/manager/api/logout", "POST", main_domain, api_logout_handler);
    add_route("/manager/api/metrics", "GET", main_domain, api_metrics_handler);
    add_route("/manager/api/ips", "GET", main_domain, api_ips_handler);
    add_route("/manager/api/logs", "GET", main_domain, api_logs_handler);
    
    // Admin API
    add_route("/manager/api/admin/list", "GET", main_domain, api_admin_list_handler);
    add_route("/manager/api/admin/create", "POST", main_domain, api_admin_create_handler);
    add_route("/manager/api/admin/role", "POST", main_domain, api_admin_role_handler);
    add_route("/manager/api/admin/delete", "POST", main_domain, api_admin_delete_handler);
    add_route("/manager/api/admin/audit", "GET", main_domain, api_admin_audit_handler);
    add_route("/manager/api/admin/audit/clear", "POST", main_domain, api_admin_audit_clear_handler);
    add_route("/manager/api/admin/update", "POST", main_domain, api_admin_update_handler);
    add_route("/manager/api/sync/batch", "POST", main_domain, api_sync_batch_handler);

    // System API
    add_route("/manager/api/config", "GET", main_domain, api_config_get_handler);
    add_route("/manager/api/system/restart", "POST", main_domain, api_system_restart_handler);
    add_route("/manager/api/system/info", "GET", main_domain, api_system_info_handler);
    add_route("/manager/api/hashes", "GET", main_domain, api_hashes_handler);
    add_route("/manager/api/upload", "POST", main_domain, api_upload_handler);
    add_route("/manager/api/delete", "POST", main_domain, api_delete_handler);

    // Data API
    add_route("/api/data", "GET", main_domain, api_data_handler);

    // Sitemap (subdomínio dev)
    add_route("/arcwb/sitemap", "GET", "dev.alrigroup.com", sitemap_handler);

    // Sub-brands
    add_route("/", "GET", "prsm.alrigroup.com", prsm_handler);
    add_route("/prsm", "GET", main_domain, prsm_handler);

    // CDN Public (subdomínio cdn.alrigroup.com)
    add_route("/*", "GET", "cdn.alrigroup.com", cdn_handler);

    // CDN Admin API
    add_route("/manager/api/cdn/files", "GET", main_domain, cdn_admin_list_handler);
    add_route("/manager/api/cdn/upload", "POST", main_domain, cdn_admin_upload_handler);
    add_route("/manager/api/cdn/link", "POST", main_domain, cdn_admin_link_handler);
    add_route("/manager/api/cdn/delete", "POST", main_domain, cdn_admin_delete_handler);
    add_route("/manager/api/cdn/mkdir", "POST", main_domain, cdn_admin_mkdir_handler);
    add_route("/manager/api/cdn/stats", "GET", main_domain, cdn_admin_stats_handler);
    add_route("/manager/api/cdn/rename", "POST", main_domain, cdn_admin_rename_handler);

    // Static Fallback (disponível em todos os domínios)
    add_route("/*", "GET", NULL, static_handler);
}
