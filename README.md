# ALRI CWB 🚀

**ALRI CWB** is a high-performance, modular ecosystem and micro-framework written entirely in native **C**. Designed for zero-downtime environments, it features End-to-End Encryption (E2EE), native PostgreSQL integration, and an advanced Hot-Reload engine.

## 🏗️ 3-Pillar Architecture

- **`/ar-core` (Orchestrator)**: The uninterruptible Master Process. It handles system signals (SIGUSR1/SIGUSR2), TTY rendering, and safe hot-recompilation.
- **`/ar-bemf` (Framework)**: The agnostic network engine (Back End Micro Framework). It manages TCP Sockets, OpenSSL, TCP Fragmentation, and Universal Rate Limiting.
- **`/ar-ws` (Web Services)**: The business logic layer. Contains your endpoints, PostgreSQL DAO, Zero-Trust session management, and the Liquid Glass Dashboard.

## ✨ Key Features

- **PostgreSQL Native**: Fully integrated with `libpq` using Prepared Statements to prevent SQL Injection.
- **Zero-Trust Security**: Strict endpoint authorization, Sudo-Mode for critical actions, and restricted static asset serving.
- **Universal Rate Limiting**: Built-in L7 DDoS and Brute-Force protection (e.g., 5 req/min for logins, 100 req/min for general API endpoints).
- **Hot-Reload (Batch Sync)**: Update your server logic in production without dropping connections. Uses a highly optimized binary stream over TCP.
- **Liquid Glass UI**: A state-of-the-art administrative dashboard featuring glassmorphism, animated mesh gradients, and dual-language (EN/PT) support.
- **Zero-Copy Delivery**: Utilizes Linux `sendfile()` for extreme performance when serving static files.

## 🚀 Getting Started

### 1. Prerequisites
Ensure you have the required C libraries installed (Debian/Ubuntu):
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev libpq-dev
```

### 2. Environment Setup
Create a `.env` file in the root directory with your PostgreSQL connection string:
```env
DATABASE_URL=host=localhost dbname=alri user=postgres password=your_password
```

### 3. Build & Run
Use the provided build script to compile the orchestrator and the application module:
```bash
chmod +x compile.sh
./compile.sh
sudo ./core
```

## 🛡️ Security & Auditing
All administrative actions are securely logged into the `audit_logs` table in PostgreSQL, tracking timestamps, IP addresses (with proxy bypassing protection), and specific events.

## 📄 Documentation
For a detailed dive into the framework's internal API (`server.h`), HTTP Request handling, and routing, please read the Official Documentation.

---
*Developed by ALRI Development.*