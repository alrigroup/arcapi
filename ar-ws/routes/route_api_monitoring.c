#include "../routes.h"
#include "../core/logs.h"
#include "../core/user_manager.h"
#include <string.h>
#include <stdio.h>

static int is_admin(ClientConnection *conn, HttpRequest *req) {
    int logged_in_role = 2;
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

    if (is_valid_admin_session(final_token, server_get_client_ip(conn), &logged_in_role) == 1) {
        return 1;
    }
    
    server_send_response(conn, 401, "application/json", "{\"error\": \"Unauthorized\"}");
    return 0;
}

void api_metrics_handler(ClientConnection *conn, HttpRequest *req) {
    if (!is_admin(conn, req)) return;

    cJSON *resp = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(resp, "metrics");
    
    pthread_mutex_lock(&stats_mutex);
    for(int i=0; i < route_stat_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "path", route_stats[i].path);
        cJSON_AddNumberToObject(item, "count", route_stats[i].count);
        cJSON_AddItemToArray(arr, item);
    }
    pthread_mutex_unlock(&stats_mutex);
    
    server_send_json(conn, 200, resp);
}

void api_ips_handler(ClientConnection *conn, HttpRequest *req) {
    if (!is_admin(conn, req)) return;

    cJSON *resp = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(resp, "ips");
    
    pthread_mutex_lock(&access_mutex);
    int start = access_count < MAX_ACCESS_LOGS ? 0 : access_head;
    for(int i=0; i < access_count; i++) {
        int idx = (start + i) % MAX_ACCESS_LOGS;
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ip", access_logs[idx].ip);
        cJSON_AddNumberToObject(item, "timestamp", (double)access_logs[idx].ts);
        cJSON_AddStringToObject(item, "path", access_logs[idx].path);
        cJSON_AddNumberToObject(item, "status", access_logs[idx].status);
        cJSON_AddStringToObject(item, "anon_id", access_logs[idx].anon_id);
        cJSON_AddItemToArray(arr, item);
    }
    pthread_mutex_unlock(&access_mutex);
    
    server_send_json(conn, 200, resp);
}

void api_logs_handler(ClientConnection *conn, HttpRequest *req) {
    if (!is_admin(conn, req)) return;

    cJSON *resp = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(resp, "logs");
    
    pthread_mutex_lock(&log_mutex);
    int start = log_count < MAX_LOG_ENTRIES ? 0 : log_head;
    for(int i=0; i < log_count; i++) {
        int idx = (start + i) % MAX_LOG_ENTRIES;
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "timestamp", (double)server_logs[idx].ts);
        cJSON_AddStringToObject(item, "level", server_logs[idx].level);
        cJSON_AddStringToObject(item, "message", server_logs[idx].message);
        cJSON_AddItemToArray(arr, item);
    }
    pthread_mutex_unlock(&log_mutex);
    
    server_send_json(conn, 200, resp);
}
