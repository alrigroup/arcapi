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
    HttpHeader headers[100]; // Limite expandido para evitar overflow
    int header_count;       // Quantidade de headers lidos
    PathParam path_params[20]; // Array de Path Parameters
    int path_param_count;      // Quantidade de parâmetros capturados na rota
    QueryParam parsed_query[50]; // Array de query strings
    int query_count;           // Quantidade de queries parseadas
    cJSON *json_doc;           // Referência interna p/ cJSON (Evita Memory Leak)
    int admin_role;            // Cargo autenticado pelo DB (Zero-Trust)
    char admin_user[64];       // Username do solicitante logado
} HttpRequest;

// Opaque pointer hiding client details (e.g. socket fd and SSL*)
typedef struct ClientConnection ClientConnection;

// Request handler signature provided by the API
typedef void (*RequestHandler)(ClientConnection *conn, HttpRequest *req);

/**
 * Busca um header específico na requisição (case-insensitive).
 * 
 * @param req Ponteiro para a requisição
 * @param header_name Nome do header a ser buscado (ex: "Authorization")
 * @return Valor do header ou NULL se não encontrado
 */
const char* get_header(HttpRequest *req, const char *header_name);

/**
 * Busca o valor de um query param na URL (ex: ?id=1).
 * 
 * @param req Ponteiro para a requisição
 * @param key Nome do parâmetro
 * @return Valor do parâmetro ou NULL se não encontrado
 */
const char* get_query_param(HttpRequest *req, const char *key);

/**
 * Busca o valor de um path param capturado na rota (ex: /users/:id).
 * 
 * @param req Ponteiro para a requisição
 * @param key Nome do parâmetro
 * @return Valor do parâmetro ou NULL se não encontrado
 */
const char* get_path_param(HttpRequest *req, const char *key);

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
 * Redireciona o cliente para outra URL via status HTTP 302.
 * 
 * @param conn Current client connection
 * @param url A URL de destino para redirecionamento
 */
void server_redirect(ClientConnection *conn, const char *url);

/**
 * Reads and serves a local file (with built-in Path Traversal protection).
 *
 * @param conn Current client connection
 * @param filepath Full path to the file to be served
 * @param content_type File mime type
 * @return 1 on success, 0 on failure (e.g. file not found or access denied)
 */
int server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type);

/**
 * Lê o corpo da requisição e faz o parse para um objeto cJSON.
 * 
 * @param req Ponteiro para a requisição HTTP
 * @return Ponteiro para o objeto cJSON criado, ou NULL se houver erro
 */
cJSON* parse_json_body(HttpRequest *req);

/**
 * Envia um objeto JSON como resposta HTTP e limpa o objeto cJSON da memória.
 * 
 * @param conn Conexão atual do cliente
 * @param status Código HTTP (ex: 200)
 * @param json_obj Objeto cJSON a ser enviado e destruído
 */
void server_send_json(ClientConnection *conn, int status, cJSON *json_obj);

/**
 * Valida credenciais usando hash SHA-256 cruzado com o db.json.
 * @return 1 se válido, 0 se inválido. Preenche out_role com o cargo.
 */
int server_validate_admin_login(const char *username, const char *pass_hash, const char *ip, int *out_role);

/**
 * Cria uma nova sessão administrativa e persiste no db.json atrelada ao IP.
 * @return Token Hexadecimal de 64 caracteres ou NULL se limite atingido.
 */
const char* server_create_admin_session(const char *username, const char *ip);

/**
 * Core print function replacing printf. Outputs to stdout and conditionally to /dev/tty1.
 *
 * @param format Format string
 */
void alri_print(const char *format, ...);

/**
 * Impressão forçada para TTY1 (Ignora as configurações de dashboard).
 * Ideal para processos vitais de boot.
 */
void alri_print_force(const char *format, ...);

#endif // SERVER_H
