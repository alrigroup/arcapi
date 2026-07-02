# AR-CWB Technical Documentation

## Overview

AR-CWB (Arc WebServer) is a custom HTTPS server ecosystem written in C. It serves as the control panel and API gateway for the ALRI Group infrastructure, handling:

- Public web pages and static assets
- Admin authentication and session management
- Real-time server monitoring (CPU, RAM, disk, network, processes)
- File synchronization and hot-reload deployment
- PostgreSQL-backed user management and audit logging
- TTY1 terminal control and CDN asset management

## Module Details

### ar-core (Orchestrator)

**File:** `ar-core/main.c` (259 lines)

The orchestrator runs as root and manages the `arc_server` lifecycle:

| Function | Line | Description |
|----------|------|-------------|
| `main()` | 1 | Entry point — initializes signals, configures terminal, enters main loop |
| `program()` | 28 | Main loop — animation, compilation, hot-reload, 30s sleep cycle |
| `core_print()` | 11 | Universal print — stdout + `/dev/tty1` if enabled |
| `building_logo_animation()` | 68 | "AR-CWB" logo animation in terminal |
| `loading_animation()` | 108 | Progress bar `[#####·····]` with percentage |
| `compile_and_update()` | 141 | Compiles ar-ws (make) + ar-core (gcc), kills old process, starts new |
| `cleanup()` | 163 | SIGINT handler — SIGTERM → 3s wait → SIGKILL |
| `handle_sigusr1()` | 185 | SIGUSR1 → hot-reload API signal to arc_server |
| `handle_sigusr2()` | 198 | SIGUSR2 → full restart flag |
| `safe_compile()` | 210 | Fork + 30s timeout compilation |

### ar-bemf (Framework)

The reusable HTTP/HTTPS framework providing:
- Event-driven request parsing
- SSL/TLS via OpenSSL
- Routing with hash-table O(1) lookup (djb2, 128 buckets)
- Domain-based virtual hosting
- Rate limiting (5 req/min login, 100 req/min general)
- MIME type detection and file serving
- Circular buffer logging (500 entries)
- Access tracking (1000 entries with IP, path, status)
- SHA-256 file hashing via `popen("sha256sum")`

**Files:**
- `server.c` (936 lines) — Main server: sockets, SSL, request handling, response, file serving
- `router.c` (88 lines) — Route table with domain matching
- `api.c` (239 lines) — Legacy API plugin system
- `ratelimit.c` (81 lines) — Per-IP rate limiting with LRU eviction

### ar-ws (Web Server)

**Build:** `ar-ws/Makefile`

The web server handles all HTTP/HTTPS traffic. Key subsystems:

#### Route Registration (`endpoints.c`)
All routes are registered with domain binding (`alrigroup.com`) for security. Only exact match and `www.` prefix are accepted.

#### Authentication (`route_api_auth.c`)
Token-based sessions with:
- SHA-256 password hashing (client-side)
- IP-bound session tokens
- Configurable session TTL (default: 1 hour)
- Automatic cleanup of expired sessions
- Anti-brute-force: 5 attempts → 5min block

#### Admin Management (`route_api_admin.c`)
Full CRUD for admin users with role-based access:
- **ROOT** (role 0): Full access — system, admin, settings, TTY
- **ADMIN** (role 1): User management, logs
- **SUP** (role 2): Read-only dashboard, analytics

#### System Monitor (`route_api_monitoring.c`)
Reads Linux `/proc` filesystem for real-time metrics:
- CPU usage (delta-based from `/proc/stat`)
- RAM usage from `/proc/meminfo`
- Disk usage via `statvfs()`
- Network I/O from `/proc/net/dev`
- Process list with per-process CPU%, RSS, owner, and executable path

#### TTY Control (`route_api_tty.c`)
Terminal control endpoints for `/dev/tty1`:
- Write text (base64-encoded)
- Clear screen
- Print logo

#### CDN Routes (`route_cdn.c`, `route_cdn_admin.c`)
Static asset serving from `storage/cdn/` with admin management.

#### Sync Engine (`sync_engine.c`)
Zero-trust file synchronization:
- SHA-256 integrity verification
- Binary batch upload protocol
- Atomic swap — only replaces verified files
- Automatic recompilation and restart after sync

#### Core Subsystems (`core/`)

| Module | File | Description |
|--------|------|-------------|
| database | `database.c` (450 lines) | PostgreSQL + JSON fallback, 24 functions |
| logs | `logs.c` (123 lines) | Circular buffer logging, route/access tracking |
| tty | `tty.c` (54 lines) | TTY1 direct write (conditional/forced/clear) |
| settings | `settings.c` (3 lines) | Global `tty_print_enabled` flag |
| sync_engine | `sync_engine.c` (147 lines) | Binary sync protocol |
| update | `update.c` (55 lines) | Directory creation, SHA-256 file scan |
| user_manager | `user_manager.c` (189 lines) | Session management, login validation, sudo |
| utils | `utils.c` (89 lines) | URL decode, base64, SHA-256, constant-time compare |

## Database Schema

PostgreSQL database with fallback to `db.json`:

### Table `users`

| Column | Type | Description |
|--------|------|-------------|
| `id` | `SERIAL PRIMARY KEY` | Auto-increment ID |
| `username` | `TEXT UNIQUE NOT NULL` | Unique username |
| `password_hash` | `TEXT NOT NULL` | SHA-256 hash |
| `role` | `INT DEFAULT 2` | 0=ROOT, 1=ADMIN, 2=SUP |
| `created_at` | `TIMESTAMP DEFAULT NOW()` | Creation timestamp |

### Table `audit_logs`

| Column | Type | Description |
|--------|------|-------------|
| `id` | `SERIAL PRIMARY KEY` | Auto-increment ID |
| `timestamp` | `TIMESTAMP DEFAULT NOW()` | Event timestamp |
| `event_type` | `TEXT NOT NULL` | Event type (LOGIN_SUCCESS, etc.) |
| `user_id` | `INT FK → users.id` | User reference (nullable) |
| `ip_address` | `TEXT` | Client IP |
| `description` | `TEXT` | Event description |

### Table `system_config`

| Column | Type | Description |
|--------|------|-------------|
| `key` | `TEXT PRIMARY KEY` | Config key |
| `val_bool` | `BOOLEAN` | Boolean value |
| `val_text` | `TEXT` | Text value |

### Initial Data

```sql
INSERT INTO users VALUES ('arcwb', '75956f5870f71f443fe8b7ac92ab53f47f044885aa59101ee989d69672060707', 0);
INSERT INTO system_config VALUES ('tty_print', false);
```

## Security Architecture

```
Client → HTTPS → Domain Check → Auth Middleware → Route Handler
                  ↓
            alrigroup.com
            www.alrigroup.com
            (all others → 404)
```

1. **Transport**: TLS 1.2+ with self-signed certificate
2. **Domain**: Strict whitelist — only `alrigroup.com` and `www.alrigroup.com`
3. **Auth**: Session token in cookie + Bearer header (optional), IP-bound
4. **Authorization**: Role check (0/1/2) per endpoint
5. **Sudo Confirmation**: All destructive actions require password re-entry
6. **File Sync**: Binary protocol with length-prefixed framing
7. **Rate Limiting**: Per-IP throttle with LRU eviction

## Build System

### Root Makefile
```bash
make        # Build arc_server + core
make run    # Build + start (as root)
make clean  # Remove build artifacts
```

### compile.sh
```bash
gcc ar-core/main.c -o core -pthread -w
make -C ar-ws
```

## CDN Assets

Directory: `storage/cdn/`

- Brand logos: ALRI, ALRIONLY, AR variants (CF/SF, B/W)
- PWA icons: favicon (multiple sizes), android-chrome, apple-touch-icon
- Web manifest: `site.webmanifest`
