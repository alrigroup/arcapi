#ifndef SERVER_H
#define SERVER_H

// Server operation modes
#define MODE_INSECURE 0
#define MODE_SECURE   1

// Formatting constants
#define PURPLE "\033[1;35m"
#define RESET  "\033[0m"
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define CYAN   "\033[1;36m"

// Abstract structure of an HTTP request
typedef struct {
    char *method;       // GET, POST, etc.
    char *path;         // /home, /api/data
    char *query_params; // id=1&user=2
    char *cookies;      // Cookie strings (simplified)
    char *body;         // Request body (if any)
} HttpRequest;

// Opaque pointer hiding client details (e.g. socket fd and SSL*)
typedef struct ClientConnection ClientConnection;

// Request handler signature provided by the API
typedef void (*RequestHandler)(ClientConnection *conn, HttpRequest *req);

/**
 * Initializes the server and starts listening for connections.
 * 
 * @param port Port to run the main server (e.g. 443 or 8080)
 * @param mode MODE_SECURE or MODE_INSECURE
 * @param handler API layer callback function
 */
void server_start(int port, int mode, RequestHandler handler);

/**
 * Sends a basic HTTP response to the client.
 * 
 * @param conn Current client connection
 * @param status HTTP code (200, 404, etc)
 * @param content_type Mime type (e.g. text/html)
 * @param body Response body
 */
void server_send_response(ClientConnection *conn, int status, const char *content_type, const char *body);

/**
 * Reads and serves a local file (with built-in Path Traversal protection).
 *
 * @param conn Current client connection
 * @param filepath Full path to the file to be served
 * @param content_type File mime type
 * @return 1 on success, 0 on failure (e.g. file not found or access denied)
 */
int server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type);

#endif // SERVER_H
