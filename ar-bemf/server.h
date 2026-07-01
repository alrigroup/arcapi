#ifndef SERVER_H
#define SERVER_H

// JSON parser (cJSON)
#include "cJSON.h"

// Server operation modes
#define MODE_INSECURE 0
#define MODE_SECURE   1

// Formatting constants
#define PURPLE "\033[1;35m"
#define RESET  "\033[0m"
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define CYAN   "\033[1;36m"
#define YELLOW "\033[1;33m"
#define WHITE  "\033[1;37m"
#define GRAY   "\033[1;30m"

// Structure to hold an HTTP header (Key-Value)
typedef struct {
    const char *name;
    const char *value;
} HttpHeader;

// Structure to hold dynamic path parameters (e.g., :id)
typedef struct {
    const char *key;
    const char *value;
} PathParam;

// Structure to hold query strings (e.g. ?id=1)
typedef struct {
    const char *key;
    const char *value;
} QueryParam;

// Abstract structure of an HTTP request
typedef struct {
    char *method;       // GET, POST, etc.
    char *path;         // /home, /api/data
    char *query_params; // id=1&user=2
    char *cookies;      // Cookie strings (simplified)
    char *body;         // Request body (if any)
    HttpHeader headers[100]; 
    int header_count;       
    PathParam path_params[20]; 
    int path_param_count;      
    QueryParam parsed_query[50]; 
    int query_count;           
    cJSON *json_doc;           
    int body_length_in_buffer; // Bytes do body já lidos no buffer inicial
    char host[256];            // Host header (domínio) sem porta
    int admin_role;            // Cargo autenticado (Zero-Trust)
    char admin_user[64];       // Username autenticado
} HttpRequest;

// Opaque pointer hiding client details
typedef struct ClientConnection ClientConnection;

// Request handler signature
typedef void (*RequestHandler)(ClientConnection *conn, HttpRequest *req);

// Logger callback signature
typedef void (*LoggerCallback)(const char *ip, const char *path, int status, const char *anon_id);

// API Functions
const char* get_header(HttpRequest *req, const char *header_name);
const char* get_query_param(HttpRequest *req, const char *key);
const char* get_path_param(HttpRequest *req, const char *key);

void server_start(int port, int mode, RequestHandler handler);
void server_set_logger(LoggerCallback callback);
void server_send_response(ClientConnection *conn, int status, const char *content_type, const char *body);
void server_add_header(ClientConnection *conn, const char *header_line);
void server_redirect(ClientConnection *conn, const char *url);
const char* server_get_client_ip(ClientConnection *conn);
int  server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type);
void server_send_404(ClientConnection *conn);
cJSON* parse_json_body(HttpRequest *req);
void server_send_json(ClientConnection *conn, int status, cJSON *json_obj);

int  server_conn_read(ClientConnection *conn, void *buf, int num);
int  server_conn_write(ClientConnection *conn, const void *buf, int num);

void alri_print(const char *format, ...);
void alri_print_force(const char *format, ...);

#endif // SERVER_H