-- ==============================================================================
-- ALRI-CORE Database Schema
-- ==============================================================================

-- 1. Users Table (RBAC)
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role INT NOT NULL DEFAULT 2, -- 0: ROOT, 1: ADMIN, 2: SUP
    created_at TIMESTAMP DEFAULT NOW()
);

-- 2. Audit Logs Table (Full Telemetry)
CREATE TABLE IF NOT EXISTS audit_logs (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT NOW(),
    event_type TEXT NOT NULL, -- LOGIN_SUCCESS, LOGIN_FAIL, SYSTEM_UPDATE, etc.
    user_id INT REFERENCES users(id) ON DELETE SET NULL,
    ip_address TEXT,
    description TEXT
);

-- 3. System Config Table (Persistent Settings)
CREATE TABLE IF NOT EXISTS system_config (
    key TEXT PRIMARY KEY,
    val_bool BOOLEAN,
    val_text TEXT
);

-- ==============================================================================
-- INITIAL DATA
-- ==============================================================================

-- Default Admin (admin / admin)
INSERT INTO users (username, password_hash, role)
VALUES ('admin', '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918', 0)
ON CONFLICT (username) DO NOTHING;

-- Default Configs
INSERT INTO system_config (key, val_bool) VALUES ('tty_print', false) ON CONFLICT (key) DO NOTHING;
