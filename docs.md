# ARCAPI - Official Documentation
**Developed by ALRI Development**

**ARCAPI** is a micro-framework and Web server written from scratch in **C**. It is designed to be lightweight, modular, and provide full control, from TCP Socket connections to the assembly of the final HTTP/HTTPS response.

---

## 1. System Architecture

The project is divided into three main layers:

1. **`core.c` (Process Manager):** Responsible for starting the system, compiling the source codes (`server.c` and `api.c`), applying startup animations, and creating the child process (`arc_server`) via `fork()`. It also manages safe shutdown (process cleanup on `SIGINT`).
2. **`server.c` / `server.h` (Core Server):** The low-level layer. Strictly handles sockets, threads (`pthread`), OpenSSL (for HTTPS), HTTP requests (`parse`), vulnerability mitigation (Path Traversal), and raw responses.
3. **`api.c` / `api.h` (Application Layer):** The developer interface. Contains the router, handles static file delivery (`.html`, `.js`, `.css`), SPA resources, and custom user functions.

---

## 2. Data Structures

When building API endpoints, you will frequently interact with the HTTP request object and the client connection.

### `HttpRequest`
Structure containing all processed request data:
```c
typedef struct {
    char *method;              // E.g.: "GET", "POST"
    char *path;                // E.g.: "/home", "/api/data"
    char *query_params;        // E.g.: "id=1&user=2" (after the '?')
    char *cookies;             // Cookie strings
    char *body;                // Request body (JSON/Text payload)
    HttpHeader headers[50];    // Array containing key/value pairs of the headers
    int header_count;          // Number of headers found
    PathParam path_params[20]; // Parameters extracted from the route (e.g.: /user/:id)
    int path_param_count;      // Number of path parameters
} HttpRequest;
```

### `ClientConnection`
Um ponteiro opaco (opaque pointer) que esconde detalhes do Socket e SSL do cliente. Ele deve ser passado para todas as funções de resposta do ARCAPI.

---

## 3. Criando Rotas e Endpoints

As rotas são definidas no arquivo `source/api.c` dentro da função `api_init()`.

### 3.1 Registrando uma Rota
A função `add_route` é utilizada para registrar um endpoint.
```c
void add_route(const char *path, const char *method, RouteHandler handler);
```
**Exemplo:**
```c
add_route("/api/users", "GET", get_users_handler);
```

### 3.2 Construindo um Handler
O `RouteHandler` é uma função de callback com a seguinte assinatura:
```c
void meu_handler(ClientConnection *conn, HttpRequest *req) {
    // Lógica da Rota aqui...
}
```

### 3.3 Lendo Parâmetros e Headers
O ARCAPI provê funções seguras para buscar dados da requisição:
* `get_header(req, "Authorization")`: Busca um header (case-insensitive).
* `get_query_param(req, "id")`: Puxa o valor do parâmetro passado na URL (ex: `?id=5`).
* `get_path_param(req, "id")`: Puxa parâmetros mapeados dinamicamente.

---

## 4. Respondendo Requisições

O servidor expõe diversos métodos em `server.h` para retornar informações ao cliente.

### 4.1 Respostas em Texto Simples / HTML Customizado
```c
void server_send_response(ClientConnection *conn, int status, const char *content_type, const char *body);
```
**Uso:**
```c
server_send_response(conn, 200, "text/plain", "Olá, Mundo!");
```

### 4.2 Respondendo em JSON (cJSON integrado)
O ARCAPI já traz suporte nativo a JSON através da biblioteca cJSON.
```c
void server_send_json(ClientConnection *conn, int status, cJSON *json_obj);
```
**Uso:**
```c
static void api_data_handler(ClientConnection *conn, HttpRequest *req) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "success");
    cJSON_AddNumberToObject(response, "code", 200);
    
    // A função limpa o cJSON da memória automaticamente
    server_send_json(conn, 200, response);
}
```

### 4.3 Recebendo JSON no Corpo da Requisição
```c
cJSON* parse_json_body(HttpRequest *req);
```

### 4.4 Redirecionamentos
Redireciona o cliente HTTP via Status 302 Found.
```c
void server_redirect(ClientConnection *conn, const char *url);
```

---

## 5. Servindo Arquivos e Páginas Dinâmicas

O framework possui um recurso robusto para lidar com Single Page Applications (SPAs) e sites estáticos localizados na pasta `/web`.

### 5.1 Envio Simples de Páginas Web
A função utilitária `sendpage` resolve automaticamente `index.html`, `main.html` ou entradas de pastas `dist/` do Vite/Webpack.

**Exemplo de integração no `api.c`:**
```c
static void dashboard_handler(ClientConnection *conn, HttpRequest *req) {
    // Irá procurar por web/dashboard/index.html automaticamente
    sendpage(conn, "dashboard");
}
```

### 5.2 Requisições de Assets Internos
O roteador (`api_request_handler`) é inteligente o suficiente para interceptar pedidos de arquivos `.js`, `.css`, `.png` e `.jpg` que chegam para as rotas das pastas e servi-los diretamente da pasta correta no sistema de arquivos, antes mesmo de cair nos fallbacks da API (404).

---

## 6. Segurança e Performance

### Modo Híbrido Automático (HTTP/HTTPS)
Quando configurado para `MODE_SECURE`, o servidor inicia o HTTPS na porta `443` (usando `cert.pem` e `key.pem`) e **automaticamente sobe uma Thread Redirecionadora na porta `80`**, aplicando um redirect `301 Moved Permanently` para o IP seguro.

### Proteção de Path Traversal
Para evitar ataques de leitura do filesystem (`../../etc/passwd`), o ARCAPI executa a função `scan_web_directory` no início do boot. Ele varre todos os arquivos disponíveis na pasta `web/` e os coloca em uma lista de aprovação alocada na memória (`allowed_paths`).

Qualquer tentativa de requisição usando `..` ou buscando arquivos não indexados resultará num **Acesso Negado Silencioso**.

---

## 7. Como Executar

O programa necessita ser executado no Linux / Subsistema WSL com o pacote padrão `build-essential` e `libssl-dev`.

1. Gere seus certificados (Para testes locais):
   ```bash
   openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365 -nodes
   ```
2. Conceda permissão e compile o sistema de bootstrap:
   ```bash
   chmod +x compile.sh
   ./compile.sh
   ```
3. Execute utilizando privilégios elevados para acesso às portas 80/443:
   ```bash
   sudo ./core
   ```
   
O **Builder Embutido** se encarregará de compilar suas modificações em código dinamicamente e subir os novos binários do `arc_server`.
