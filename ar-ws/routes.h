#ifndef ROUTES_H
#define ROUTES_H

#include "../ar-bemf/server.h"

// Route handlers - Frontend/Pages
void home_handler(ClientConnection *conn, HttpRequest *req);
void manager_login_handler(ClientConnection *conn, HttpRequest *req);
void manager_dashboard_handler(ClientConnection *conn, HttpRequest *req);
void api_component_handler(ClientConnection *conn, HttpRequest *req);
void static_handler(ClientConnection *conn, HttpRequest *req);

// Route handlers - API
void api_login_handler(ClientConnection *conn, HttpRequest *req);
void api_logout_handler(ClientConnection *conn, HttpRequest *req);
void api_metrics_handler(ClientConnection *conn, HttpRequest *req);
void api_ips_handler(ClientConnection *conn, HttpRequest *req);
void api_logs_handler(ClientConnection *conn, HttpRequest *req);

// Route handlers - Admin
void api_admin_list_handler(ClientConnection *conn, HttpRequest *req);
void api_admin_create_handler(ClientConnection *conn, HttpRequest *req);
void api_admin_delete_handler(ClientConnection *conn, HttpRequest *req);
void api_admin_role_handler(ClientConnection *conn, HttpRequest *req);
void api_admin_audit_handler(ClientConnection *conn, HttpRequest *req);
void api_admin_audit_clear_handler(ClientConnection *conn, HttpRequest *req);
void api_admin_update_handler(ClientConnection *conn, HttpRequest *req);

// Route handlers - System
void api_config_get_handler(ClientConnection *conn, HttpRequest *req);
void api_system_restart_handler(ClientConnection *conn, HttpRequest *req);
void api_system_info_handler(ClientConnection *conn, HttpRequest *req);
void api_hashes_handler(ClientConnection *conn, HttpRequest *req);
void api_upload_handler(ClientConnection *conn, HttpRequest *req);
void api_delete_handler(ClientConnection *conn, HttpRequest *req);
void api_sync_batch_handler(ClientConnection *conn, HttpRequest *req);

void api_data_handler(ClientConnection *conn, HttpRequest *req);

// Utility
void sendpage(ClientConnection *conn, const char *folder_name);

#endif // ROUTES_H
