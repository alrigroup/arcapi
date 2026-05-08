<div align="center">
  <img src="https://i.ibb.co/Fk4sSC7X/ALRI-SF-W.png" width="150" alt="ALRI Group Logo">

  # ARCAPI
  **Developed by ALRI Development**
  
  *A micro-framework and Web server written from scratch in C.*
</div>

---

### ⚠️ Notice & Scope
> **Notice:** This is a "hobby" project created for study, testing, and personal experimentation. It is not intended for a large-scale corporate or commercial environment without extra structural reviews, being focused on learning and controlling low-level architectures.

> **OS Compatibility:** The project is currently **100% focused on Linux/Unix environments** and relies on system-specific functions like `fork/exec`. **There is no native support for Windows.** To run it on a Windows machine, you must use WSL (Windows Subsystem for Linux).

### 📌 About the Project
**ARCAPI** is a micro-framework and Web server written from scratch in **C**. Its main goal is to deliver an ultra-lightweight modular environment, controlling everything from Socket connections to the final HTTP/HTTPS response.

The system abstracts the entire backend part, leaving a ready-to-use flow for you to host and serve static applications, SPAs (created in React, Vite, Angular, etc), and even build a REST API using C.

### 🛡️ Protection & License
This project, including all its source code, scripts, and digital assets, is strictly protected by the **[ARGLP - ALRI GROUP LICENSE PERMISSIVE](https://raw.githubusercontent.com/alrigroup/licenses/refs/heads/main/LICENSE-ARGLP)**.
* You can use, modify, and distribute this project as long as you credit **ALRI Group** and follow the terms of the license.

---

### ⚡ Key Features
* **Native HTTPS:** Out-of-the-box secure support using the OpenSSL library.
* **Automatic Redirect:** Isolated thread that listens on port 80 and automatically forwards all HTTP traffic to HTTPS.
* **Simple Router (`api.c`):** Allows easy registration of GET/POST routes. With the generic `sendpage("folder")` function, it serves your application's HTML and all `.css` and `.js` files automatically.
* **Anti-Path Traversal Security:** Upon startup, the server scans the local files in the `/web` folder and maps what exists in memory, denying any malicious access attempting to fetch system files (`../../`).
* **Self-Managed:** The base program (`core.c`) handles compiling, isolating, and running your main server (`arc_server`) using `fork/exec`.

---

### 🚀 How to Run

**1. System Dependencies**
Since the server was built in C with SSL support, you will need the following packages on your Linux/WSL:
```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

**2. SSL Certificates (Required)**
The server will not start without valid encryption keys in the root folder! Since the `.gitignore` accompanying the project prevents these keys from leaking to github, **you must generate them locally** (or copy real keys):
```bash
# In the project root, generate a self-signed test certificate:
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365 -nodes
```

**3. Compilation and Execution**
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

### 🛠️ Creating New Pages and Sites
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

---

### 👤 Credits
* **Developer Company:** [ALRI Development](https://github.com/alrigroup/)
* **Lead Developer:** [AlexAR](https://github.com/alexsanderalri)


---

### 📧 Contact
For inquiries about **ALRI Group**'s portfolio of solutions or technical questions:

[![Instagram](https://img.shields.io/badge/Instagram-%23E4405F.svg?style=for-the-badge&logo=Instagram&logoColor=white)](https://www.instagram.com/alrigroup)
[![GitHub](https://img.shields.io/badge/GitHub-black?style=for-the-badge&logo=github&logoColor=white)](https://github.com/alrigroup)
[![Discord](https://img.shields.io/badge/Discord-%235865F2.svg?style=for-the-badge&logo=discord&logoColor=white)](https://dsc.gg/alrigroup)

<br>

<div align="center">
  <i>"Building the future, one line of code at a time."</i>
  <br>
  <p>Copyright © 2020-2026 <b>ALRI Group</b></p>
</div>