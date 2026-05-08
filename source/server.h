#ifndef SERVER_H
#define SERVER_H

// Modos de operação do servidor
#define MODE_INSECURE 0
#define MODE_SECURE   1

// Constantes de formatação
#define PURPLE "\033[1;35m"
#define RESET  "\033[0m"
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define CYAN   "\033[1;36m"

// Estrutura abstrata de uma requisição HTTP
typedef struct {
    char *method;       // GET, POST, etc.
    char *path;         // /home, /api/data
    char *query_params; // id=1&user=2
    char *cookies;      // Strings de cookies (simplificado)
    char *body;         // Corpo da requisição (se houver)
} HttpRequest;

// Ponteiro opaco que esconde detalhes do cliente (ex: socket fd e SSL*)
typedef struct ClientConnection ClientConnection;

// Assinatura do handler de requisições que será fornecido pela API
typedef void (*RequestHandler)(ClientConnection *conn, HttpRequest *req);

/**
 * Inicializa o servidor e começa a escutar por conexões.
 * 
 * @param port Porta para rodar o servidor principal (ex: 443 ou 8080)
 * @param mode MODE_SECURE ou MODE_INSECURE
 * @param handler Função de callback da camada de API
 */
void server_start(int port, int mode, RequestHandler handler);

/**
 * Envia uma resposta HTTP básica ao cliente.
 * 
 * @param conn Conexão atual do cliente
 * @param status Código HTTP (200, 404, etc)
 * @param content_type Mime type (ex: text/html)
 * @param body Corpo da resposta
 */
void server_send_response(ClientConnection *conn, int status, const char *content_type, const char *body);

/**
 * Lê e serve um arquivo local (com proteção contra Path Traversal embutida).
 *
 * @param conn Conexão atual do cliente
 * @param filepath Caminho completo para o arquivo a ser servido
 * @param content_type Mime type do arquivo
 * @return 1 se sucesso, 0 se falhou (ex: arquivo não encontrado ou acesso negado)
 */
int server_serve_file(ClientConnection *conn, const char *filepath, const char *content_type);

#endif // SERVER_H
