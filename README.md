# AR-CWB — Arc WebServer Ecosystem

Secure, zero-trust HTTPS server and control panel for the ALRI Group ecosystem.

## Architecture

```
ar-core/       Orchestrator — compiles, manages, and monitors arc_server
ar-ws/         Web server — routes, APIs, dashboard, static files
ar-bemf/       Framework — HTTP parser, router, SSL, rate limiter
shared/        Shared utilities (cJSON)
```

- **ar-core** runs as root, handles compilation, hot-reload, and lifecycle
- **ar-ws** is the web server with all routes and business logic
- **ar-bemf** provides the HTTP/HTTPS server framework and routing engine

## Security Model

- Zero-Trust architecture with session-based authentication
- Role-based access: **ROOT** (0), **ADMIN** (1), **SUP** (2)
- Endpoints protected by role checks (ROOT-only for system/admin operations)
- Session tokens with IP-binding and automatic expiration
- Domain whitelist — routes only match `alrigroup.com` and `www.alrigroup.com`
- Sudo confirmation required for destructive actions (delete user, restart, sync)
- All passwords hashed client-side (SHA-256) before transmission
- HTTPS-only with automatic HTTP→HTTPS redirect

## Requirements

- Linux (Debian/Ubuntu)
- GCC, Make, OpenSSL, libpq (PostgreSQL)
- Root access (for port 80/443 and `/dev/tty1`)

## Quick Start

```bash
sudo apt install gcc make libssl-dev libpq-dev postgresql
sudo make run
```

The server will:
1. Generate a self-signed certificate (if missing)
2. Initialize the PostgreSQL schema
3. Start HTTPS on port 443 with HTTP→443 redirect on port 80

## Make Targets

| Target  | Description                               |
| ------- | ----------------------------------------- |
| `all`   | Build `arc_server` and `core`             |
| `run`   | Build + start the orchestrator (as root)  |
| `clean` | Remove all build artifacts                |

## Routes

### Pages
| Path                  | Description          |
| --------------------- | -------------------- |
| `/`                   | Home page            |
| `/home`               | Home page            |
| `/manager/login`      | Admin login          |
| `/manager/dashboard`  | Admin dashboard      |

### Auth API
| Method | Path                     | Description        |
| ------ | ------------------------ | ------------------ |
| POST   | `/manager/api/login`     | Authenticate admin |
| POST   | `/manager/api/logout`    | End session        |

### Admin API (ROOT only)
| Method | Path                             | Description            |
| ------ | -------------------------------- | ---------------------- |
| GET    | `/manager/api/admin/list`        | List admins            |
| POST   | `/manager/api/admin/create`      | Create admin           |
| POST   | `/manager/api/admin/role`        | Change role            |
| POST   | `/manager/api/admin/delete`      | Delete admin           |
| POST   | `/manager/api/admin/update`      | Reset password         |
| GET    | `/manager/api/admin/audit`       | View audit logs        |
| POST   | `/manager/api/admin/audit/clear` | Clear audit logs       |

### System API (ROOT only)
| Method | Path                         | Description              |
| ------ | ---------------------------- | ------------------------ |
| GET    | `/manager/api/system/info`   | CPU, RAM, disk, network  |
| POST   | `/manager/api/system/restart`| Recompile and restart    |
| GET    | `/manager/api/config`        | Get system config        |

### Data API
| Method | Path                     | Description               |
| ------ | ------------------------ | ------------------------- |
| GET    | `/manager/api/metrics`   | Route access metrics      |
| GET    | `/manager/api/ips`       | Recent IP access log      |
| GET    | `/manager/api/logs`      | Server console logs       |
| GET    | `/manager/api/hashes`    | File hashes for sync      |
| POST   | `/manager/api/sync/batch`| Hot-reload file sync      |
| POST   | `/manager/api/upload`    | Upload file               |
| POST   | `/manager/api/delete`    | Delete file               |
| GET    | `/api/data`              | Public data endpoint      |

## Frontend

The admin dashboard is a single-page application with tabs for:
- **Analytics & Firewall** — route access charts, recent IPs
- **Console Logs** — real-time server log viewer
- **System Logs** — PostgreSQL audit trail
- **Management** — user CRUD with role assignment
- **System Monitor** — CPU, RAM, disk, network, process table
- **Updates** — hot-reload file synchronization
- **Settings** — manual recompilation

## Sync & Hot-Reload

The dashboard includes a file synchronization system:
1. Select local project folder in the browser
2. SHA-256 hashes are compared with the server
3. Only changed files are transferred
4. Server auto-restarts with the new code

## License

Proprietary — ALRI Group. All rights reserved.
