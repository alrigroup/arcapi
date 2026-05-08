# ALRI-CORE

> **Notice:** This is a "hobby" project created for study, testing, and personal experimentation. It is not intended for a large-scale corporate or commercial environment without extra structural reviews, being focused on learning and controlling low-level architectures.

## About the Project

**ALRI-CORE** is a micro-framework and Web server written from scratch in **C**. Its main goal is to deliver an ultra-lightweight modular environment, controlling everything from Socket connections to the final HTTP/HTTPS response.

The system abstracts the entire backend part, leaving a ready-to-use flow for you to host and serve static applications, SPAs (created in React, Vite, Angular, etc), and even build a REST API using C.

### Key Features:
- **Native HTTPS**: Out-of-the-box secure support using the OpenSSL library.
- **Automatic Redirect**: Isolated thread that listens on port 80 and automatically forwards all HTTP traffic to HTTPS.
- **Simple Router (`api.c`)**: Allows easy registration of GET/POST routes. With the generic `sendpage("folder")` function, it serves your application's HTML and all `.css` and `.js` files automatically.
- **Anti-Path Traversal Security**: Upon startup, the server scans the local files in the `/web` folder and maps what exists in memory, denying any malicious access attempting to fetch system files (`../../`).
- **Self-Managed**: The base program (`core.c`) handles compiling, isolating, and running your main server (`alri_server`) using `fork/exec`.

## How to Run

### 1. System Dependencies
Since the server was built in C with SSL support, you will need the following packages on your Linux/WSL:
```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

### 2. SSL Certificates (Required)
The server will not start without valid encryption keys in the root folder! Since the `.gitignore` accompanying the project prevents these keys from leaking to github, **you must generate them locally** (or copy real keys):
```bash
# In the project root, generate a self-signed test certificate:
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365 -nodes
```

### 3. Compilation and Execution
In the project's main folder, use the automation script for the initial build:

```bash
# Grant permission and compile
chmod +x compile.sh
./compile.sh

# Start the main system (Requires sudo to access low ports 80/443)
sudo ./core
```

Done! Access in your browser: `https://localhost` (or the IP where you are running it).

---

## Creating New Pages and Sites
The modular architecture makes everything very easy:
1. Create a folder inside `web/` with your site/SPA. (Ex: `web/dashboard`)
2. Open the `source/api.c` file.
3. Register your route at the end of the file and point to the created folder:

```c
// Inside api_init()...
add_route("/dashboard", "GET", dashboard_handler);

// Your new handler pointing to the folder
static void dashboard_handler(ClientConnection *conn, HttpRequest *req) {
    sendpage(conn, "dashboard"); 
}
```
The `sendpage` function will do all the hard work of fetching `index.html` and releasing the assets.
