# AR-CWB Technical Documentation

## Overview

AR-CWB (Arc WebServer) is a custom HTTPS server ecosystem written in C. It serves as the control panel and API gateway for the ALRI Group infrastructure, handling:

- Public web pages and static assets
- Admin authentication and session management
- Real-time server monitoring (CPU, RAM, disk, network, processes)
- File synchronization and hot-reload deployment
- PostgreSQL-backed user management and audit logging

## Module Details

### ar-core (Orchestrator)

**File:** `ar-core/main.c`

The orchestrator runs as root and manages the `arc_server` lifecycle:

- Generates SSL certificates on first run (via `compile.sh`)
- Compiles `arc_server` from source using the Makefile
- Monitors the child process and restarts on crash
- Handles hot-reload signals — recompiles and restarts modules
- Prints compilation logs directly to `/dev/tty1`

### ar-ws (Web Server)

**Build:** `ar-ws/Makefile`

The web server handles all HTTP/HTTPS traffic. Key subsystems:

#### Route Registration (`endpoints.c`)
All routes are registered with domain binding (`alrigroup.com`) for security. Only exact match and `www.` prefix are accepted — arbitrary subdomains are rejected.

#### Authentication (`route_api_auth.c`)
Token-based sessions with:
- SHA-256 password hashing (client-side)
- IP-bound session tokens
- Configurable session TTL (default: 1 hour)
- Automatic cleanup of expired sessions

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

#### Sync Engine (`sync_engine.c`)
Zero-trust file synchronization:
- SHA-256 integrity verification
- Binary batch upload protocol
- Atomic swap — only replaces verified files
- Automatic recompilation and restart after sync

### ar-bemf (Framework)

Reusable HTTP/HTTPS server framework providing:
- Event-driven request parsing
- SSL/TLS via OpenSSL
- Routing with hash-table O(1) lookup
- Domain-based virtual hosting
- Rate limiting
- MIME type detection and file serving

## Database Schema

PostgreSQL database `alri` with tables:

```sql
users       (id, username, password_hash, role, created_at)
audit_logs  (id, event_type, admin_user, ip_address, description, created_at)
system_config (key, value)
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
