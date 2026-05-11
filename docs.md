# ALRI CWB - Official Documentation
**Developed by ALRI Development**

**ALRI CWB** is an advanced modular ecosystem consisting of a micro-framework and Web server written in native **C**. Designed for ultra-high performance and availability systems, it abstracts the complexity of networking (Sockets, TCP, TLS) and process orchestration, allowing you to focus exclusively on business logic.

Inspired by the simplicity of Flask and the robustness of enterprise frameworks, ALRI CWB offers fluid routing, automatic JSON parsing, Hot-Reload, and native support for E2EE (End-to-End Encryption).

---

## 📑 Index
1. [General Architecture](#1-general-architecture)
2. [Quickstart Guide](#2-quickstart-guide)
3. [The Framework (ar-bemf) - Developer Guide](#3-the-framework-ar-bemf---developer-guide)
    * Handling Requests (HttpRequest)
    * Responses and JSON
    * Serving Static Files
4. [The Application (ar-ws) - Business Rules](#4-the-application-ar-ws---business-rules)
    * Routing
    * Authentication and RBAC
    * Persistent Database
5. [The Orchestrator (ar-core) - Lifecycle](#5-the-orchestrator-ar-core---lifecycle)
6. [Synchronization Engine (Hot-Reload)](#6-synchronization-engine-hot-reload)

---

## 1. General Architecture

The project breaks the monolithic pattern, dividing responsibilities into three strict directories and modules:

* **`/ar-core` (The Manager):** Uninterruptible Master Process. Keeps the server alive, intercepts crashes, and handles hot recompilations.
* **`/ar-bemf` (The HTTP Engine):** The Micro-framework itself. Handles Sockets, OpenSSL, TCP Fragmentation, Header and Body Parsing. It is 100% agnostic and does not know your database or routes.
* **`/ar-ws` (Your Application):** This is where you program. It contains your endpoints, admin panel, user logic, and static files (HTML/JS/CSS).

---

## 2. Quickstart Guide

To create an endpoint, you only need to follow **2 steps**:

**Step 1: Create the Handler function** (Ex: in `ar-ws/routes/route_hello.c`)
```c
#include "../../ar-bemf/server.h"

void hello_handler(ClientConnection *conn, HttpRequest *req) {
    // Responds with plain text
    server_send_response(conn, 200, "text/plain", "Hello World from ALRI CWB!");
}
```

**Step 2: Register the Route** (In `ar-ws/endpoints.c`)
```c
void register_all_endpoints() {
    // ... system routes ...
    add_route("/hello", "GET", hello_handler);
}
```
And that's it. On the next compilation, `https://localhost/hello` will return your string.

---

## 3. The Framework (`ar-bemf`) - Developer Guide

The framework's base library is exposed via `server.h`. It handles abstract pointers ensuring memory safety.

### 3.1. Understanding the Request Object (`HttpRequest`)
When a request arrives at your handler function, the `req` object already contains all pre-processed and validated data from the network engine.

```c
typedef struct {
    char *method;              // GET, POST, PUT, DELETE
    char *path;                // Requested Route (/api/users)
    char *query_params;        // Raw query string (id=1&sort=asc)
    char *body;                // Raw request body (JSON, Text)
    int header_count;          // Number of headers read
    // ... parsed queries and references structures ...
} HttpRequest;
```

### 3.2. Extracting Request Data
Use the thread-safe helper functions to read information cleanly:

```c
// Get a header (Case-Insensitive)
const char *token = get_header(req, "Authorization");

// Get parameters passed in the URL (?id=5&status=active)
const char *id = get_query_param(req, "id"); 

// Get parameters passed directly in the dynamic Path (e.g.: /users/:id)
const char *path_id = get_path_param(req, "id");
```

> **Attention:** The strings returned by these functions belong to the request's lifecycle in memory (Buffer). Do not attempt to `free()` them manually.

### 3.3. Receiving JSON Payload (Tutorial)
The framework has full and secure integration (leak-free) with the `cJSON` library. Parsing is done on-demand to save CPU.

```c
void login_api_handler(ClientConnection *conn, HttpRequest *req) {
    // The framework parses and stores the document in RAM associated with the request
    cJSON *json = parse_json_body(req);
    
    if (!json) {
        server_send_response(conn, 400, "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
    }

    cJSON *user = cJSON_GetObjectItem(json, "username");
    
    if (user && cJSON_IsString(user)) {
        printf("Received login for: %s\n", user->valuestring);
    }
    
    // IMPORTANT: Never free the `json` (req->json_doc) manually!
    // The orchestrator clears the JSON parse at the end of the request cycle automatically.
}
```

### 3.4. Sending Responses
To respond to the client, you use the abstract `ClientConnection *conn` structure. The framework will set the HTTP flags, inject appropriate Headers, inject tracking Cookies, and finalize the clean Socket connection.

#### HTML / Text Response
```c
server_send_response(conn, 200, "text/html", "<h1>Hello!</h1>");
```

#### Redirect Response
```c
server_redirect(conn, "/manager/login");
```

#### Dynamic JSON Response (The Correct Way)
```c
void api_metrics_handler(ClientConnection *conn, HttpRequest *req) {
    // Create the cJSON object normally
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "online");
    cJSON_AddNumberToObject(resp, "users_active", 50);

    // By calling 'server_send_json', AR-BEMF compiles the string, 
    // sends it over the network via TLS, and IMMEDIATELY destroys (cJSON_Delete) the object
    // to prevent any Memory Leak.
    server_send_json(conn, 200, resp);
}
```

#### Serving Files from Disk
Sends files to the client using optimized Chunked Streams, ideal for videos, large images, or HTML without using much RAM. Natively protected against Path Traversal (`../`).
```c
server_serve_file(conn, "web/img/logo.png", "image/png");
```

---

## 4. The Application (`ar-ws`) - Business Rules

This module was designed with resilient administrative panels and complex REST APIs in mind.

### 4.1. The Persistent Database (PostgreSQL)
ALRI CWB utilizes PostgreSQL as its primary data store, ensuring ACID compliance, strong data integrity, and enterprise-grade performance. The Data Access Object (DAO) layer is strictly isolated in `ar-ws/core/database.c`.

**How it works:**
* Connection state and auto-reconnect are handled natively. If the database goes offline, the server gracefully returns `503 Service Unavailable` for protected routes.
* **Security:** All queries use Prepared Statements (`PQexecParams`), making SQL Injection impossible.
* The system uses an idempotent initialization process to create required tables (`users`, `audit_logs`, `system_config`) on boot.
* **Configuration:** Connection parameters are securely loaded from a local `.env` file using the `DATABASE_URL` key.

### 4.2. Security and Session Control
The library handles persistent in-memory sessions and heavy mitigations against attacks.

1. **Universal Rate Limiting:** Built-in protection against L7 DDoS and Brute Force attacks. Login endpoints are limited to 5 requests per minute per IP, while general endpoints allow up to 100 requests per minute (`429 Too Many Requests`).
2. **Anti-Brute Force:** If an IP fails an administrative password 5 times, an algorithmic Firewall lockout occurs, preventing eviction bypass under stress.
3. **Timing-Attack Security:** Token and Password comparisons (`constant_time_compare`) evaluate the entire buffer bit-by-bit so that Hackers cannot discover tokens based on server response time.
3. **RBAC (Role-Based Access Control):** 
    * Level `0`: ROOT (Full Access, TTY, Updates, Deletions).
    * Level `1`: ADMIN (Can see logs, manage other admins).
    * Level `2`: SUP (Restricted viewer).
5. **Zero-Trust Access:** Static assets in the `/manager/` directory are strictly protected. The server verifies active sessions before serving any administrative HTML, JS, or CSS.

### 4.3. Sudo Mode
Critical infrastructure routes (`/manager/api/upload`, `/manager/api/delete`, `/manager/api/system/restart`) intercept the payload requesting the Sudo password (`confirm_pass`). The system revalidates the raw password against the PostgreSQL hash via Zero-Trust before allowing changes to the filesystem or database.

### 4.4. Telemetry and Audit Logs
The framework separates logs into two categories:
* **Console Logs (`ar_log`)**: Infrastructure telemetry printed to the TTY, stored in memory, and saved to `ar_server.log`.
* **System Logs (Audit)**: Administrative actions are securely persisted in the PostgreSQL `audit_logs` table, tracking `event_type`, `admin_user`, `ip_address`, and `description`. They are accessible via the Dashboard with advanced filtering and timezone localization.

```c
// Types: INFO, WARN, ERROR, MAP
ar_log("INFO", "User %s logged in successfully.", username);
ar_log("ERROR", "Connection failure with database ID: %d", req_id);
```

---

## 5. The Orchestrator (`ar-core`) - Lifecycle

Unlike PHP scripts or Node.js (PM2), here you are dealing with processes at the Operating System level under the C hood.

**The Process Dance (Fork/Exec):**
1. You run `sudo ./core`.
2. The `core` (Parent Process PID 1) displays the logo and compiles (make) the API.
3. The `core` calls `fork()`. It creates the `arc_server` (Child Process PID 2).
4. The `arc_server` opens the Ports (443 and 80) and handles billions of requests. The `core` enters an idle loop, consuming zero processing.

**Fault Tolerance:**
If the server suffers a violent crash (Segfault/Memory Access Violation), it dies. The Orchestrator notices and restarts the service instantly.

---

## 6. Synchronization Engine (Hot-Reload)

The biggest innovation of ALRI CWB V2.0. Syncing the production environment never required Git Push, FTP, or dangerous network interruptions. The server performs "Hot" updates.

### 6.1 How the Magic Happens
The protocol avoids `multipart/form-data` and uses a **Chunked Binary Stream (Batch Sync)** for ultra-high performance, protected against Slowloris attacks by an absolute timeout boundary (30 seconds).

1. The local Frontend calculates the **SHA-256 Hash** of all your code files modified by VSCode.
2. The server sends a list of its own Hashes.
3. The Frontend condenses only the "differences" into a hexadecimal super `Blob` block, injecting Size Prefixes (`Little Endian`) before each file.
4. The Server (`server_conn_read`) sucks the bytes through the TCP Socket and unpacks the Stream directly onto the hard drive without allocating the entire string in RAM (avoiding *Out Of Memory* with large images and DLLs).

### 6.2 Restart with System Signals (SIGUSR1 / SIGUSR2)
After downloading all new codes, the API triggers a `kill(getppid(), SIGUSR1)`. The Master process (`ar-core`) receives the signal (Custom Unix User Signal), re-executes the Makefile compilation, and replaces the network process. The absolute downtime is typically between 120 and 350 milliseconds.

---

## 7. Build and Execution

The project must be compiled in a Linux/POSIX environment equipped with `gcc`, `make`, `libssl-dev`, and `libpq-dev`.

```bash
# 1. Give execution permission to the build script
chmod +x compile.sh

# 2. Compile and run the orchestrator (requires root for port 80/443 binding)
./compile.sh
sudo ./core
```
