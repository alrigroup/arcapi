const translations = {
    en: {
        lang_btn: "PT-BR",
        tab_analytics: "Analytics & Firewall",
        tab_tty: "Terminal (TTY)",
        tab_logs: "Console Logs",
        tab_system_logs: "System Logs",
        tab_users: "Management",
        tab_settings: "Settings",
        tab_update: "Updates",
        p_analytics_h: "ANALYTICS: ACCESS BY ROUTE",
        p_firewall_h: "FIREWALL: ORIGINS & TRAFFIC",
        th_ts: "Timestamp",
        th_ip: "IP Address",
        th_path: "Requested Path",
        th_status: "Status",
        th_timestamp: "Timestamp",
        th_event: "Event",
        th_admin: "Admin",
        th_description: "Description",
        th_ip_origin: "IP Origin",
        p_tty_h: "SERVER: TTY CONTROL",
        ph_tty: "Message for terminal...",
        btn_tty_send: "Send Secure Text",
        btn_tty_logo: "Send Logo",
        btn_tty_clear: "Clear Screen",
        p_logs_h: "SYSTEM: INTERNAL LOGS",
        p_admin_h: "SEC: ADMIN MANAGEMENT",
        p_admin_info: "Credentials are encrypted (Client-Side Hashing) before leaving the browser.",
        ph_new_user: "New Username",
        ph_new_pass: "New Password",
        btn_create_admin: "Create Administrator",
        p_update_h: "HOT-RELOAD: FILE MANAGER & RESTART",
        p_update_info: "Select the project root folder. The system will sync only modified files (Smart Sync SHA-256) with E2EE security.",
        span_drop: "[ Click to Select Folder (Sync) ]",
        opt_none: "Save Only (No Reload)",
        opt_api: "Restart API (Recompile arc_server)",
        opt_core: "Full Core Restart (Recompile All)",
        btn_reload: "Trigger Hot-Reload",
        btn_view_users: "View Users",
        btn_add_user: "Add User",
        admin_actions: "Actions for:",
        ph_sudo_pass: "Your Admin Password",
        btn_cancel: "Cancel",
        btn_confirm: "Confirm",
        modal_del_title: "Delete Administrator",
        modal_role_title: "Change Admin Role",
        modal_create_title: "Create Administrator",
        modal_restart_title: "System Restart",
        p_settings_h: "SYSTEM: CONFIGURATIONS",
        p_settings_tty_title: "Global TTY Print",
        p_settings_tty_desc: "Replicate all system logs and messages to physical /dev/tty1.",
        p_settings_recompile_title: "Manual System Recompilation",
        p_settings_recompile_desc: "Force the system to rebuild and restart modules. Requires ROOT privileges and Sudo confirmation.",
        btn_recompile_api: "Recompile API",
        btn_recompile_core: "Full Core Restart"
    },
    pt: {
        lang_btn: "EN-US",
        tab_analytics: "Análise e Firewall",
        tab_tty: "Terminal (TTY)",
        tab_logs: "Logs do Console",
        tab_system_logs: "Logs do Sistema",
        tab_users: "Gerenciamento",
        tab_settings: "Configurações",
        tab_update: "Atualizações",
        p_analytics_h: "ANALYTICS: ACESSOS POR ROTA",
        p_firewall_h: "FIREWALL: ORIGENS E TRÁFEGO",
        th_ts: "Data/Hora",
        th_ip: "Endereço IP",
        th_path: "Caminho Requisitado",
        th_status: "Status",
        th_timestamp: "Data/Hora",
        th_event: "Evento",
        th_admin: "Administrador",
        th_description: "Descrição",
        th_ip_origin: "IP de Origem",
        p_tty_h: "SERVIDOR: CONTROLE TTY",
        ph_tty: "Mensagem para o terminal...",
        btn_tty_send: "Enviar Texto Seguro",
        btn_tty_logo: "Enviar Logo",
        btn_tty_clear: "Limpar Tela",
        p_logs_h: "SISTEMA: LOGS INTERNOS",
        p_admin_h: "SEC: GESTÃO DE ADMINISTRADORES",
        p_admin_info: "Credenciais são criptografadas (Client-Side Hashing) antes de deixar o navegador.",
        ph_new_user: "Novo Usuário",
        ph_new_pass: "Nova Senha",
        btn_create_admin: "Criar Administrador",
        p_update_h: "HOT-RELOAD: SINCRONISMO E REINICIALIZAÇÃO",
        p_update_info: "Selecione a pasta raiz do projeto. O sistema sincronizará apenas arquivos modificados (Smart Sync SHA-256) com segurança E2EE.",
        span_drop: "[ Clique para Selecionar a Pasta (Sync) ]",
        opt_none: "Apenas Salvar (Nenhum Reload)",
        opt_api: "Restart API (Recompilar arc_server)",
        opt_core: "Full Core Restart (Recompilar Tudo)",
        btn_reload: "Executar Hot-Reload",
        btn_view_users: "Ver Usuários",
        btn_add_user: "Adicionar Usuário",
        admin_actions: "Ações para:",
        ph_sudo_pass: "Sua Senha Sudo",
        btn_cancel: "Cancelar",
        btn_confirm: "Confirmar",
        modal_del_title: "Deletar Administrador",
        modal_role_title: "Alterar Cargo",
        modal_create_title: "Criar Administrador",
        modal_restart_title: "Reinício do Sistema",
        p_settings_h: "SISTEMA: CONFIGURAÇÕES",
        p_settings_tty_title: "Impressão Global no TTY",
        p_settings_tty_desc: "Replica todos os logs e mensagens do sistema fisicamente no /dev/tty1.",
        p_settings_recompile_title: "Recompilação Manual do Sistema",
        p_settings_recompile_desc: "Força a reconstrução e o reinício dos módulos do sistema. Requer privilégios ROOT e confirmação Sudo.",
        btn_recompile_api: "Recompilar API",
        btn_recompile_core: "Recompilar Todo o Core"
    }
};

let currentLang = localStorage.getItem('alri_lang') || 'en';

function updateUI() {
    const langData = translations[currentLang];
    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.getAttribute('data-i18n');
        if (langData[key]) el.innerHTML = langData[key];
    });
    document.querySelectorAll('[data-i18n-ph]').forEach(el => {
        const key = el.getAttribute('data-i18n-ph');
        if (langData[key]) el.placeholder = langData[key];
    });
    document.getElementById('langText').innerText = langData.lang_btn;
}

function toggleLanguage() {
    currentLang = currentLang === 'en' ? 'pt' : 'en';
    localStorage.setItem('alri_lang', currentLang);
    updateUI();
}

// Global State

// Theme Management
function toggleThemePanel() {
    document.getElementById('themePanel').classList.toggle('active');
}

function setTheme(color, glow) {
    document.documentElement.style.setProperty('--accent', color);
    document.documentElement.style.setProperty('--accent-glow', glow);
    localStorage.setItem('arc_accent', color);
    localStorage.setItem('arc_glow', glow);
    toggleThemePanel();

    // Atualiza o gradiente do gráfico em tempo real ao trocar de tema
    if (metricsChartInstance) {
        const canvas = document.getElementById('metricsChart');
        if (canvas) {
            const ctx = canvas.getContext('2d');
            const gradient = ctx.createLinearGradient(0, 0, 0, 400);
            gradient.addColorStop(0, color);
            gradient.addColorStop(1, 'rgba(0, 0, 0, 0.05)');
            metricsChartInstance.data.datasets[0].backgroundColor = gradient;
            metricsChartInstance.update();
        }
    }
}

// Toggle function for Mobile Sidebar
function toggleSidebar() {
    const sidebar = document.querySelector('.sidebar');
    const overlay = document.getElementById('sidebarOverlay');
    if (sidebar && overlay) {
        sidebar.classList.toggle('active');
        const isActive = sidebar.classList.contains('active');
        if (isActive) { overlay.classList.add('active'); }
        else { overlay.classList.remove('active'); }
    }
}

function applyTheme() {
    const savedColor = localStorage.getItem('arc_accent');
    const savedGlow = localStorage.getItem('arc_glow');
    if (savedColor && savedGlow) {
        document.documentElement.style.setProperty('--accent', savedColor);
        document.documentElement.style.setProperty('--accent-glow', savedGlow);
    }
}

document.addEventListener('DOMContentLoaded', () => {
    applyTheme();
    updateUI();
});

let currentTab = 'tab-analytics';

let metricsChartInstance = null;
const errIndicator = document.getElementById('err-indicator');

function enforcePermissions() {
    const myRole = parseInt(sessionStorage.getItem('arc_admin_role') || '2');

    // Restrictions for ADMIN (1) and SUP (2)
    if (myRole > 0) {
        const navTty = document.getElementById('nav-tty');
        if (navTty) navTty.style.display = 'none';
        const navUpdate = document.getElementById('nav-update');
        if (navUpdate) navUpdate.style.display = 'none';
        const navAddUser = document.getElementById('nav-add-user');
        if (navAddUser) navAddUser.style.display = 'none';
        const navSettings = document.getElementById('nav-settings');
        if (navSettings) navSettings.style.display = 'none';
        const btnClearLogs = document.getElementById('btn-clear-logs');
        if (btnClearLogs) btnClearLogs.style.display = 'none';
    }
    // Restrictions for SUP (2)
    if (myRole > 1) {
        const navUsers = document.getElementById('nav-users');
        if (navUsers) navUsers.style.display = 'none';
    }
}

async function loadTabContent(tabId) {
    const tabEl = document.getElementById(tabId);
    if (tabEl.innerHTML.trim() === '') {
        try {
            const res = await fetch(`/manager/api/component/${tabId}`, { credentials: 'same-origin' });
            if (res.status === 403) {
                tabEl.innerHTML = `<div class="panel"><div class="panel-content" style="color:var(--danger);">Restricted Access to Protected Content.</div></div>`;
                return false;
            }
            if (res.status === 401) { logout(); return false; }
            if (!res.ok) throw new Error("Failed to load dynamic API component.");

            tabEl.innerHTML = await res.text();
            updateUI(); // Redo translation of freshly inserted HTML

            if (tabId === 'tab-analytics') {
                initChart();
            } else if (tabId === 'tab-update') {
                const fileInput = document.getElementById('fileUpload');
                const lblDisplay = document.getElementById('fileNameDisplay');
                const btnReload = document.getElementById('btnReload');
                if (fileInput) {
                    fileInput.addEventListener('change', (e) => {
                        if (e.target.files.length > 0) {
                            lblDisplay.innerText = `[ ${e.target.files.length} file(s) selected ]`;
                            scanFolder();
                        }
                    });
                }
            } else if (tabId === 'tab-settings') {
                loadConfig();
            }
            return true;
        } catch (e) {
            tabEl.innerHTML = `<div class="panel"><div class="panel-content" style="color:var(--danger);">${e.message}</div></div>`;
            return false;
        }
    }
    return true;
}

function checkAuth() {
    enforcePermissions();
    loadTabContent(currentTab).then(() => {
        pollData();
    });
}

async function logout() {
    sessionStorage.removeItem('arc_admin_flag');
    await fetch('/manager/api/logout', { method: 'POST', credentials: 'same-origin' }).catch(() => ({}));
    window.location.href = '/manager/login';
}

function sanitizeHTML(str) {
    if (!str) return '';
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

function formatTime(unixSeconds) {
    const d = new Date(unixSeconds * 1000);
    return d.toLocaleTimeString('pt-BR', { hour12: false });
}

function initChart() {
    const canvas = document.getElementById('metricsChart');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const accent = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim() || '#00ff41';
    const gradient = ctx.createLinearGradient(0, 0, 0, 400);
    gradient.addColorStop(0, accent);
    gradient.addColorStop(1, 'rgba(0, 0, 0, 0.05)');

    metricsChartInstance = new Chart(ctx, {
        type: 'line',
        data: { labels: [], datasets: [{ label: 'Requests', data: [], backgroundColor: gradient, borderColor: accent, borderWidth: 2, fill: true, tension: 0.4, pointBackgroundColor: accent, pointBorderColor: '#fff', pointRadius: 4, pointHoverRadius: 6 }] },
        options: {
            responsive: true, maintainAspectRatio: false,
            plugins: { legend: { display: false } },
            scales: {
                y: { beginAtZero: true, grid: { color: 'rgba(255,255,255,0.05)' }, ticks: { color: '#a1a1aa' } },
                x: { grid: { display: false }, ticks: { color: '#a1a1aa' } }
            }
        }
    });
}

async function fetchAPI(endpoint, options = {}) {
    try {
        const res = await fetch(endpoint, {
            ...options,
            credentials: 'same-origin',
            headers: { ...options.headers }
        });
        if (res.status === 401) {
            logout();
            throw new Error("Unauthorized");
        }
        if (res.status === 403) {
            const err = await res.json().catch(() => ({}));
            alert("Security Block: " + (err.error || "Action not allowed for your role."));
            throw new Error("Forbidden");
        }
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        errIndicator.style.display = 'none';
        return await res.json();
    } catch (err) {
        errIndicator.style.display = 'inline';
        return null;
    }
}

async function fetchMetrics() {
    if (!document.getElementById('metricsChart')) return;
    const data = await fetchAPI('/manager/api/metrics');
    if (data && data.metrics) {
        const sorted = data.metrics.sort((a, b) => b.count - a.count).slice(0, 10);
        metricsChartInstance.data.labels = sorted.map(m => sanitizeHTML(m.path));
        metricsChartInstance.data.datasets[0].data = sorted.map(m => m.count);
        metricsChartInstance.update();
    }
}

async function fetchIPs() {
    const tbody = document.querySelector('#ipsTable tbody');
    if (!tbody) return;
    const data = await fetchAPI('/manager/api/ips');
    if (data && data.ips) {
        tbody.innerHTML = '';
        const recentIps = data.ips.reverse().slice(0, 50);
        for (const log of recentIps) {
            let statClass = 'status-200';
            if (log.status >= 400 && log.status < 500) statClass = 'status-404';
            if (log.status >= 500) statClass = 'status-500';
            tbody.innerHTML += `<tr><td>${formatTime(log.timestamp)}</td><td>${sanitizeHTML(log.ip)}</td><td>${sanitizeHTML(log.path)}</td><td class="${statClass}">${log.status}</td></tr>`;
        }
    }
}

let lastLogCount = 0;
async function fetchLogs() {
    const term = document.getElementById('terminal');
    if (!term) return;
    const data = await fetchAPI('/manager/api/logs');
    if (data && data.logs) {
        term.innerHTML = data.logs.map(log => `[${formatTime(log.timestamp)}] <span class="log-level-${log.level}">[${log.level}]</span> ${sanitizeHTML(log.message)}<br>`).join('');
        if (data.logs.length !== lastLogCount) { term.scrollTop = term.scrollHeight; lastLogCount = data.logs.length; }
    }
}

async function switchTab(tabId, btnElement) {
    currentTab = tabId;

    // Update buttons
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    if (btnElement) btnElement.classList.add('active');

    // Reset view and animations
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
        tab.style.animation = 'none';
    });

    const activeTab = document.getElementById(tabId);
    activeTab.classList.add('active');

    // Collapse Sidebar when switching tabs on mobile
    if (window.innerWidth <= 768) {
        const sidebar = document.querySelector('.sidebar');
        if (sidebar && sidebar.classList.contains('active')) toggleSidebar();
    }

    await loadTabContent(tabId);

    // Staggered entry animation for panels
    const panels = activeTab.querySelectorAll('.panel');
    panels.forEach((p, idx) => {
        p.style.opacity = '0';
        p.style.animation = `fadeSlideUp 0.5s ease-out ${idx * 0.1}s forwards`;
    });

    pollData();
}

function switchUserTab(tabId) {
    document.querySelectorAll('.user-tab-btn').forEach(btn => btn.classList.remove('active'));
    document.querySelectorAll('.user-view').forEach(view => view.style.display = 'none');
    document.getElementById('nav-' + tabId).classList.add('active');
    document.getElementById(tabId).style.display = 'block';
}


async function loadConfig() {
    const data = await fetchAPI('/manager/api/config');
    if (data) {
        const toggle = document.getElementById('tty-toggle');
        if (toggle) toggle.checked = data.tty_print;
    }
}

async function toggleTTYConfig() {
    const toggle = document.getElementById('tty-toggle');
    if (!toggle) return;
    const newState = toggle.checked;

    const actionText = newState ? "enable" : "disable";
    const bodyHtml = `<p>Are you sure you want to ${actionText} global TTY1 printing?</p><p style="color: var(--text-muted); font-size: 0.9em; margin-top: 5px;">Confirm your Sudo password to authorize.</p>`;

    const result = await modalManager.open('modal_restart_title', bodyHtml);
    if (!result) {
        toggle.checked = !newState;
        return;
    }

    try {
        const res = await fetch('/manager/api/config/tty', {
            method: 'POST', credentials: 'same-origin',
            headers: { 'Content-Type': 'application/json', 'X-Confirm-Pass': result.confirm_pass },
            body: JSON.stringify({ tty_print: newState })
        });
        if (!res.ok) {
            const err = await res.json().catch(() => ({}));
            if (res.status === 401 && (!err.error || !err.error.includes("password"))) {
                logout(); return;
            }
            alert(`Failed to update configuration: ${err.error || 'HTTP ' + res.status}`);
            toggle.checked = !newState;
        }
    } catch (err) {
        alert("Communication with server failed.");
        toggle.checked = !newState;
    }
}

async function getFileHash(file) {
    const buffer = await file.arrayBuffer();
    const hashBuffer = await crypto.subtle.digest('SHA-256', buffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

let syncQueue = { upload: [], delete: [] };

async function scanFolder() {
    const fileInput = document.getElementById('fileUpload');
    const statusDiv = document.getElementById('updateStatus');
    const planDiv = document.getElementById('syncPlan');
    const planList = document.getElementById('planList');
    const confirmArea = document.getElementById('confirmArea');
    const btnReload = document.getElementById('btnReload');

    if (!fileInput || fileInput.files.length === 0) return;

    statusDiv.innerHTML = "<span style='color: var(--warning)'>Mapping integrity (SHA-256)...</span>";
    planDiv.style.display = 'none';
    confirmArea.style.display = 'none';
    planList.innerHTML = '';
    syncQueue = { upload: [], delete: [] };

    const remoteData = await fetchAPI('/manager/api/hashes');
    if (!remoteData || !remoteData.files) {
        statusDiv.innerHTML = "<span style='color: var(--danger)'>FAILED: Could not fetch hashes from the server.</span>";
        return;
    }

    const remoteHashes = remoteData.files;
    const localPaths = new Set();
    const files = Array.from(fileInput.files);
    const concurrency = 10;

    async function processFile(file) {
        const parts = file.webkitRelativePath.split('/');
        parts.shift(); // Remove selected folder name (e.g., "arc-bemf/")
        const serverPath = parts.join('/');

        if (!serverPath) return;

        // Filter to ignore files that should not be synced (binaries and control)
        if (serverPath.endsWith('.o') ||
            serverPath.startsWith('.git/') ||
            serverPath === 'core' ||
            serverPath === 'arc_server' ||
            serverPath.includes('/.git/')) {
            return;
        }

        localPaths.add(serverPath);
        const localHash = await getFileHash(file);

        if (remoteHashes[serverPath] !== localHash) {
            syncQueue.upload.push({ file, serverPath });
            planList.innerHTML += `<li style="color:var(--warning); margin-bottom:2px;">[MOD] ${serverPath}</li>`;
        }
    }

    for (let i = 0; i < files.length; i += concurrency) {
        const chunk = files.slice(i, i + concurrency);
        await Promise.all(chunk.map(f => processFile(f)));
        statusDiv.innerHTML = `<span style='color: var(--warning)'>Mapping integrity: ${Math.min(i + concurrency, files.length)} / ${files.length}</span>`;
    }

    for (const remotePath of Object.keys(remoteHashes)) {
        if (!localPaths.has(remotePath)) {
            syncQueue.delete.push(remotePath);
            planList.innerHTML += `<li style="color:var(--danger); margin-bottom:2px;">[DEL] ${remotePath}</li>`;
        }
    }

    if (syncQueue.upload.length === 0 && syncQueue.delete.length === 0) {
        statusDiv.innerHTML = "<span style='color:var(--text-main)'>✓ System is already 100% synchronized with the local folder.</span>";
    } else {
        statusDiv.innerHTML = `<span style='color:var(--text-main)'>Scan completed. ${syncQueue.upload.length} changes detected.</span>`;
        planDiv.style.display = 'block';
        confirmArea.style.display = 'block';
    }
}

async function confirmAndSync() {
    const statusDiv = document.getElementById('updateStatus');
    const btnReload = document.getElementById('btnReload');
    const restartMode = document.getElementById('restartMode');
    const mode = restartMode.value;

    if (syncQueue.upload.length === 0 && syncQueue.delete.length === 0) return;

    const bodyHtml = `<p>Confirm synchronization of <strong>${syncQueue.upload.length + syncQueue.delete.length}</strong> changes and Hot-Reload (${mode}).</p><p style="color: var(--text-muted); font-size: 0.9em; margin-top: 5px;">Enter your Sudo password to authorize.</p>`;
    const result = await modalManager.open('modal_restart_title', bodyHtml);
    if (!result) return;
    const sudoHash = result.confirm_pass;

    btnReload.disabled = true; btnReload.innerText = "Synchronizing...";

    // Prepare JSON Header
    const header = {
        sudo_pass: sudoHash,
        user: sessionStorage.getItem('arc_admin_user') || 'admin',
        restart_mode: mode,
        deletes: syncQueue.delete
    };
    const headerJson = JSON.stringify(header);
    const headerBytes = new TextEncoder().encode(headerJson);

    // Calculate estimated total size (optional, for progress)
    let totalSize = 4 + headerBytes.length;
    for (const item of syncQueue.upload) {
        totalSize += 4 + new TextEncoder().encode(item.serverPath).length + 4 + item.file.size;
    }
    totalSize += 4; // End of Stream

    // Build Binary Blob (Simulated Stream for Fetch)
    const chunks = [];

    // 1. JSON Header
    const jsonLenBuf = new ArrayBuffer(4);
    new DataView(jsonLenBuf).setUint32(0, headerBytes.length, true);
    chunks.push(jsonLenBuf);
    chunks.push(headerBytes);

    // 2. Files
    for (let i = 0; i < syncQueue.upload.length; i++) {
        const item = syncQueue.upload[i];
        statusDiv.innerHTML = `<span style='color: var(--warning)'>Preparing: ${item.serverPath}</span>`;

        const pathBytes = new TextEncoder().encode(item.serverPath);
        const pathLenBuf = new ArrayBuffer(4);
        new DataView(pathLenBuf).setUint32(0, pathBytes.length, true);
        chunks.push(pathLenBuf);
        chunks.push(pathBytes);

        const dataLenBuf = new ArrayBuffer(4);
        new DataView(dataLenBuf).setUint32(0, item.file.size, true);
        chunks.push(dataLenBuf);
        chunks.push(item.file);
    }

    // 3. End of Stream
    const endBuf = new ArrayBuffer(4);
    new DataView(endBuf).setUint32(0, 0, true);
    chunks.push(endBuf);

    const fullBlob = new Blob(chunks, { type: 'application/octet-stream' });

    statusDiv.innerHTML = `<span style='color: var(--warning)'>Sending binary batch (${(fullBlob.size / 1024).toFixed(1)} KB)...</span>`;

    try {
        const res = await fetch('/manager/api/sync/batch', {
            method: 'POST', credentials: 'same-origin',
            body: fullBlob
        });

        if (res.status === 401) { logout(); return; }
        if (res.ok) {
            statusDiv.innerHTML = `<span style='color: var(--text-main)'>✓ Batch synchronization completed successfully!</span>`;
            setTimeout(() => { location.reload(); }, 2000);
        } else {
            const err = await res.json().catch(() => ({ error: "Critical stream error" }));
            statusDiv.innerHTML = `<span style='color: var(--danger)'>FAILED: ${err.error}</span>`;
            btnReload.disabled = false; btnReload.innerText = "Confirm & Sync";
        }
    } catch (e) {
        statusDiv.innerHTML = `<span style='color: var(--danger)'>Connection error: ${e.message}</span>`;
        btnReload.disabled = false; btnReload.innerText = "Confirm & Sync";
    }
}

async function actionSystemRestart(mode) {
    const modeName = mode === 'api' ? 'API (arc_server)' : 'Full Core';
    const bodyHtml = `<p>Are you sure you want to force recompilation and restart of: <strong style="color:var(--warning);">${modeName}</strong>?</p><p style="color: var(--text-muted); font-size: 0.9em; margin-top: 5px;">Confirm your Sudo password to authorize.</p>`;

    const result = await modalManager.open('modal_restart_title', bodyHtml);
    if (!result) return;

    const res = await fetch('/manager/api/system/restart', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: mode, confirm_pass: result.confirm_pass })
    });

    if (res.ok) {
        alert(`Restart signal (${modeName}) sent successfully! Please wait while the server recompiles...`);
    } else {
        const err = await res.json().catch(() => ({ error: "Failed to send restart signal." }));
        alert(err.error);
    }
}

async function sendTTYText() {
    const text = document.getElementById('tty-text').value;
    if (!text) return;
    document.getElementById('tty-status').innerText = "Encrypting payload...";
    document.getElementById('tty-status').style.color = "var(--warning)";

    // Secure encode prevents Plain Text and natively handles UTF-8 Accents
    const b64Text = btoa(unescape(encodeURIComponent(text)));

    const res = await fetch('/manager/api/tty/text', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ text: b64Text })
    });

    if (res.ok) {
        document.getElementById('tty-status').innerText = "Text physically printed on the server!";
        document.getElementById('tty-status').style.color = "var(--text-main)";
        document.getElementById('tty-text').value = '';
    } else {
        document.getElementById('tty-status').innerText = "Failed to send text.";
        document.getElementById('tty-status').style.color = "var(--danger)";
    }
}

async function clearTTY() {
    const res = await fetch('/manager/api/tty/clear', { method: 'POST', credentials: 'same-origin' });
    if (res.ok) document.getElementById('tty-status').innerHTML = "<span style='color: var(--text-main)'>TTY1 server screen cleared.</span>";
    else document.getElementById('tty-status').innerHTML = "<span style='color: var(--danger)'>Failed to clear screen.</span>";
}

async function sendTTYLogo() {
    document.getElementById('tty-status').innerText = "Processing animated logo...";
    const res = await fetch('/manager/api/tty/logo', { method: 'POST', credentials: 'same-origin' });
    if (res.ok) document.getElementById('tty-status').innerHTML = "<span style='color: var(--text-main)'>Animation running on TTY1!</span>";
    else document.getElementById('tty-status').innerHTML = "<span style='color: var(--danger)'>Failed to send logo.</span>";
}

async function sha256(message) {
    const msgBuffer = new TextEncoder().encode(message);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

// Modal Sudo-Mode Manager
const modalManager = {
    resolve: null,
    open: function (titleKey, bodyHtml) {
        return new Promise((resolve) => {
            const titleEl = document.getElementById('modalTitle');
            if (titleKey) titleEl.setAttribute('data-i18n', titleKey);

            document.getElementById('modalBody').innerHTML = bodyHtml;
            document.getElementById('modalConfirmPass').value = '';
            const modal = document.getElementById('adminActionModal');

            modal.style.display = 'flex';
            // Small delay to trigger smooth CSS transition
            setTimeout(() => modal.classList.add('active'), 10);
            updateUI();

            setTimeout(() => document.getElementById('modalConfirmPass').focus(), 150);
            this.resolve = resolve;
        });
    },
    close: function () {
        const modal = document.getElementById('adminActionModal');
        modal.classList.remove('active');
        setTimeout(() => modal.style.display = 'none', 300);
    },
    cancel: function () {
        this.close();
        if (this.resolve) {
            this.resolve(null);
            this.resolve = null;
        }
    },
    confirm: async function () {
        const pass = document.getElementById('modalConfirmPass').value;
        if (!pass) return;
        const extraInput = document.getElementById('modalExtraInput');
        const extraData = extraInput ? extraInput.value : null;

        const passHash = await sha256(pass);
        const res = this.resolve;
        this.resolve = null;
        this.close();
        if (res) res({ confirm_pass: passHash, extraData });
    }
};

let selectedAdminUser = null;
let selectedAdminRole = null;

async function fetchAdmins() {
    const tbody = document.querySelector('#adminsTable tbody');
    if (!tbody) return;
    const data = await fetchAPI('/manager/api/admin/list');
    if (data && data.admins) {
        tbody.innerHTML = data.admins.map(a => `
            <tr onclick="selectAdmin('${sanitizeHTML(a.user)}', ${a.role})" style="cursor:pointer;">
                <td><div class="user-avatar">${(a.user[0] || 'U').toUpperCase()}</div></td>
                <td>${sanitizeHTML(a.user)}</td>
                <td><span class="role-badge role-${a.role}">${a.role === 0 ? 'ROOT' : (a.role === 1 ? 'ADMIN' : 'SUP')}</span></td>
            </tr>
        `).join('');
        document.getElementById('admin-actions-bar').style.display = 'none';
        selectedAdminUser = null;
    }
}

function selectAdmin(name, role) {
    selectedAdminUser = name;
    selectedAdminRole = role;
    const bar = document.getElementById('admin-actions-bar');
    const btnArea = document.getElementById('admin-action-buttons');
    if (bar) bar.style.display = 'block';
    if (document.getElementById('selected-admin-name')) document.getElementById('selected-admin-name').innerText = name;

    if (btnArea) {
        let btns = `<button class="btn-reload" style="border-color:var(--info); color:var(--info);" onclick="actionChangeRole()">Change Role</button>`;
        if (name !== 'admin') {
            btns += `<button class="btn-reload" style="border-color:var(--danger); color:var(--danger);" onclick="actionDelete()">Delete User</button>`;
        }
        btns += `<button class="btn-reload" style="border-color:var(--warning); color:var(--warning);" onclick="actionResetPass()">Reset Password</button>`;
        btnArea.innerHTML = btns;
    }
}

async function actionChangeRole() {
    const name = selectedAdminUser;
    const currentRole = selectedAdminRole;
    const bodyHtml = `
        <p style="margin-bottom: 15px;">Select new role for <strong style="color:var(--accent);">${sanitizeHTML(name)}</strong>:</p>
        <select id="modalExtraInput" style="width:100%; padding:10px; background:#000; border:1px solid var(--border-color); color:var(--text-main); font-family:var(--font-mono); outline:none; cursor:pointer;">
            <option value="0" ${currentRole === 0 ? 'selected' : ''}>ROOT (Nível 0)</option>
            <option value="1" ${currentRole === 1 ? 'selected' : ''}>ADMIN (Nível 1)</option>
            <option value="2" ${currentRole === 2 ? 'selected' : ''}>SUP (Nível 2)</option>
        </select>
    `;

    const res = await modalManager.open('modal_role_title', bodyHtml);
    if (!res) return;

    const apiRes = await fetch('/manager/api/admin/role', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: name, role: parseInt(res.extraData), confirm_pass: res.confirm_pass })
    });

    if (apiRes.ok) fetchAdmins();
}

async function actionDelete() {
    const name = selectedAdminUser;
    const res = await modalManager.open('modal_del_title', `Are you sure you want to delete user <b>${name}</b>? This action is permanent.`);
    if (!res) return;

    const apiRes = await fetch('/manager/api/admin/delete', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: name, confirm_pass: res.confirm_pass })
    });

    if (apiRes.ok) fetchAdmins();
}

async function actionResetPass() {
    const name = selectedAdminUser;
    const newPass = prompt(`Enter new password for ${name}:`);
    if (!newPass) return;

    const res = await modalManager.open('modal_restart_title', `Confirm Sudo to reset password for <b>${name}</b>.`);
    if (!res) return;

    const passHash = await sha256(newPass);
    const apiRes = await fetch('/manager/api/admin/update', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: name, pass: passHash, confirm_pass: res.confirm_pass })
    });

    if (apiRes.ok) alert("Password updated!");
}

async function createAdmin() {
    const user = document.getElementById('new-admin-user').value;
    const pass = document.getElementById('new-admin-pass').value;
    if (!user || !pass) return;

    const res = await modalManager.open('modal_create_title', `Confirm Sudo to create administrator <b>${user}</b>.`);
    if (!res) return;

    const passHash = await sha256(pass);
    const apiRes = await fetch('/manager/api/admin/create', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user, pass: passHash, role: 2, confirm_pass: res.confirm_pass })
    });

    if (apiRes.ok) {
        document.getElementById('new-admin-user').value = '';
        document.getElementById('new-admin-pass').value = '';
        fetchAdmins();
        switchUserTab('view-users');
    }
}

function switchUserTab(tabId) {
    document.querySelectorAll('.user-view').forEach(v => v.style.display = 'none');
    document.querySelectorAll('.user-tab-btn').forEach(b => b.classList.remove('active'));
    if (document.getElementById(tabId)) document.getElementById(tabId).style.display = 'block';
    if (document.getElementById(`nav-${tabId}`)) document.getElementById(`nav-${tabId}`).classList.add('active');
    if (tabId === 'view-users') fetchAdmins();
}

function switchLogTab(tabId) {
    document.querySelectorAll('.log-view').forEach(v => v.style.display = 'none');
    document.querySelectorAll('.log-tab-btn').forEach(b => b.classList.remove('active'));
    if (document.getElementById(tabId)) document.getElementById(tabId).style.display = 'block';
    if (document.getElementById(`nav-${tabId}`)) document.getElementById(`nav-${tabId}`).classList.add('active');

    if (tabId === 'console-logs') fetchLogs();
    if (tabId === 'admin-logs') fetchAuditLogs();
}

async function fetchAuditLogs() {
    const filterType = document.getElementById('audit-filter-type') ? document.getElementById('audit-filter-type').value : '';
    const filterUser = document.getElementById('audit-filter-user') ? document.getElementById('audit-filter-user').value : '';
    const filterIp = document.getElementById('audit-filter-ip') ? document.getElementById('audit-filter-ip').value : '';

    const params = new URLSearchParams();
    if (filterType) params.append('type', filterType);
    if (filterUser) params.append('user', filterUser);
    if (filterIp) params.append('ip', filterIp);

    const endpoint = `/manager/api/admin/audit${params.toString() ? '?' + params.toString() : ''}`;
    const data = await fetchAPI(endpoint);
    const tbody = document.querySelector('#auditTable tbody');
    if (data && data.logs && tbody) {
        tbody.innerHTML = data.logs.map(l => `
            <tr>
                <td style="color:var(--text-muted); font-size:0.8rem;">${new Date(l.ts).toLocaleString()}</td>
                <td style="color:var(--accent); font-weight:bold;">${l.type}</td>
                <td>${sanitizeHTML(l.user)}</td>
                <td style="color:var(--text-main);">${sanitizeHTML(l.desc)}</td>
                <td style="color:var(--info); font-family:monospace;">${sanitizeHTML(l.ip || 'N/A')}</td>
            </tr>
        `).join('');
    }
}

async function clearAuditLogs() {
    const res = await modalManager.open('modal_del_title', `Are you sure you want to permanently clear all audit logs? This action requires Sudo.`);
    if (!res) return;

    const apiRes = await fetch('/manager/api/admin/audit/clear', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ confirm_pass: res.confirm_pass })
    });

    if (apiRes.ok) {
        fetchAuditLogs();
    } else {
        const err = await apiRes.json().catch(() => ({ error: "Failed to clear logs." }));
        alert(err.error);
    }
}

async function validateSession() {
    try { const res = await fetch('/manager/api/metrics', { credentials: 'same-origin' }); if (res.status === 401) logout(); } catch (e) { }
}

function pollData() {
    if (currentTab === 'tab-analytics') {
        fetchMetrics(); fetchIPs();
    } else if (currentTab === 'tab-logs') {
        const activeLogTab = document.querySelector('.log-tab-btn.active');
        if (activeLogTab && activeLogTab.id === 'nav-admin-logs') fetchAuditLogs();
        else fetchLogs();
    } else if (currentTab === 'tab-users') {
        fetchAdmins();
    }
}

checkAuth();
setInterval(pollData, 5000);
setInterval(validateSession, 10000);
