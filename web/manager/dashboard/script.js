const translations = {
    en: {
        lang_btn: "PT-BR",
        tab_analytics: "Analytics & Firewall",
        tab_tty: "Terminal (TTY)",
        tab_logs: "System Logs",
        tab_users: "Management",
        tab_settings: "Settings",
        tab_update: "Updates",
        p_analytics_h: "ANALYTICS: ACCESS BY ROUTE",
        p_firewall_h: "FIREWALL: ORIGINS & TRAFFIC",
        th_ts: "Timestamp",
        th_ip: "IP Address",
        th_path: "Requested Path",
        th_status: "Status",
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
        tab_logs: "Logs do Sistema",
        tab_users: "Gerenciamento",
        tab_settings: "Configurações",
        tab_update: "Atualizações",
        p_analytics_h: "ANALYTICS: ACESSOS POR ROTA",
        p_firewall_h: "FIREWALL: ORIGENS E TRÁFEGO",
        th_ts: "Data/Hora",
        th_ip: "Endereço IP",
        th_path: "Caminho Requisitado",
        th_status: "Status",
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
}

// Função de Toggle para o Mobile Sidebar
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
                tabEl.innerHTML = `<div class="panel"><div class="panel-content" style="color:var(--danger);">Acesso Restrito ao Conteúdo Protegido.</div></div>`;
                return false;
            }
            if (res.status === 401) { logout(); return false; }
            if (!res.ok) throw new Error("Falha ao carregar componente dinâmico da API.");

            tabEl.innerHTML = await res.text();
            updateUI(); // Refaz a tradução do HTML fresco inserido

            if (tabId === 'tab-analytics') {
                initChart();
            } else if (tabId === 'tab-update') {
                const fileInput = document.getElementById('fileUpload');
                const lblDisplay = document.getElementById('fileNameDisplay');
                const btnReload = document.getElementById('btnReload');
                if (fileInput) {
                    fileInput.addEventListener('change', (e) => {
                        if (e.target.files.length > 0) {
                            lblDisplay.innerText = `[ ${e.target.files.length} arquivo(s) selecionado(s) na pasta ]`;
                            btnReload.disabled = false;
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
    const ctx = document.getElementById('metricsChart').getContext('2d');
    metricsChartInstance = new Chart(ctx, {
        type: 'bar',
        data: { labels: [], datasets: [{ label: 'Requisições', data: [], backgroundColor: '#00ff41', borderWidth: 1 }] },
        options: {
            responsive: true, maintainAspectRatio: false,
            plugins: { legend: { display: false } },
            scales: {
                y: { beginAtZero: true, grid: { color: '#222' }, ticks: { color: '#888' } },
                x: { grid: { color: '#222' }, ticks: { color: '#888' } }
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
            alert("Bloqueio de Segurança: " + (err.error || "Ação não permitida para o seu cargo."));
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

    // Atualiza botões
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    if (btnElement) btnElement.classList.add('active');

    // Reseta visualização e animações
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
        tab.style.animation = 'none';
    });

    const activeTab = document.getElementById(tabId);
    activeTab.classList.add('active');

    // Recolhe a Sidebar ao trocar de aba se estiver no celular
    if (window.innerWidth <= 768) {
        const sidebar = document.querySelector('.sidebar');
        if (sidebar && sidebar.classList.contains('active')) toggleSidebar();
    }

    await loadTabContent(tabId);

    // Animação de entrada Staggered para os painéis
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

function pollData() {
    if (currentTab === 'tab-analytics') {
        fetchMetrics(); fetchIPs();
    } else if (currentTab === 'tab-logs') {
        fetchLogs();
    } else if (currentTab === 'tab-users') {
        fetchAdmins();
    }
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

    const actionText = newState ? "ativar" : "desativar";
    const bodyHtml = `<p>Você tem certeza que deseja ${actionText} a impressão global no TTY1?</p><p style="color: var(--text-muted); font-size: 0.9em; margin-top: 5px;">Confirme sua senha Sudo para autorizar.</p>`;

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
            if (res.status === 401 && (!err.error || !err.error.includes("Senha sudo"))) {
                logout(); return;
            }
            alert(`Falha ao atualizar configuração: ${err.error || 'HTTP ' + res.status}`);
            toggle.checked = !newState;
        }
    } catch (err) {
        alert("Falha de comunicação com o servidor.");
        toggle.checked = !newState;
    }
}

async function getFileHash(file) {
    const buffer = await file.arrayBuffer();
    const hashBuffer = await crypto.subtle.digest('SHA-256', buffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

async function triggerUpdate() {
    const fileInput = document.getElementById('fileUpload');
    const btnReload = document.getElementById('btnReload');
    const lblDisplay = document.getElementById('fileNameDisplay');
    const statusDiv = document.getElementById('updateStatus');
    const restartMode = document.getElementById('restartMode');

    if (!fileInput || !restartMode) return;
    const mode = restartMode.value;

    if (fileInput.files.length === 0) {
        alert("Selecione uma pasta com arquivos para sincronizar!");
        return;
    }

    const bodyHtml = `<p>Você tem certeza que deseja sincronizar os arquivos selecionados e aplicar o Hot-Reload?</p><p style="color: var(--text-muted); font-size: 0.9em; margin-top: 5px;">Confirme sua senha Sudo para autorizar esta ação crítica.</p>`;
    const result = await modalManager.open('modal_restart_title', bodyHtml);
    if (!result) return;
    const sudoHash = result.confirm_pass;

    btnReload.disabled = true; btnReload.innerText = "Sincronizando...";
    statusDiv.innerHTML = "<span style='color: var(--warning)'>Mapeando Hashes Locais e Remotos...</span>";

    const remoteData = await fetchAPI('/manager/api/hashes');
    if (!remoteData || !remoteData.files) {
        statusDiv.innerHTML = `<span style='color: var(--danger)'>FALHA: Não foi possível obter o mapa de hashes do servidor.</span>`;
        btnReload.disabled = false;
        btnReload.innerText = "Trigger Hot-Reload";
        return;
    }

    const remoteHashes = remoteData.files;
    const uploadQueue = [];
    const localPaths = new Set();

    for (let i = 0; i < fileInput.files.length; i++) {
        const file = fileInput.files[i];
        const match = file.webkitRelativePath.match(/(source|web)\/(.*)/);
        if (!match) continue; // Ignora arquivos fora do escopo padrão
        const serverPath = match[0];
        localPaths.add(serverPath);
        const localHash = await getFileHash(file);

        if (remoteHashes[serverPath] !== localHash) {
            uploadQueue.push({ file, serverPath });
        }
    }

    const deleteQueue = [];
    for (const remotePath of Object.keys(remoteHashes)) {
        if (!localPaths.has(remotePath)) {
            deleteQueue.push(remotePath);
        }
    }

    if (uploadQueue.length === 0 && deleteQueue.length === 0) {
        statusDiv.innerHTML = "<span style='color: var(--text-main)'>Todos os arquivos já estão sincronizados com o Servidor (Hashes Idênticos)!</span>";
        fileInput.value = ''; lblDisplay.innerText = "[ Clique para Selecionar a Pasta (Sync) ]";
        btnReload.disabled = false; btnReload.innerText = "Trigger Hot-Reload";
        return;
    }

    let operationsDone = 0;
    const totalOperations = uploadQueue.length + deleteQueue.length;

    for (let i = 0; i < uploadQueue.length; i++) {
        const item = uploadQueue[i];
        operationsDone++;
        const isLast = (operationsDone === totalOperations);
        const rmode = isLast ? mode : 'none'; // Aplica o restart somente na requisição final

        statusDiv.innerHTML = `<span style='color: var(--warning)'>Upload: [${i + 1}/${uploadQueue.length}] ${item.serverPath}</span>`;

        const res = await fetch('/manager/api/upload', {
            method: 'POST', credentials: 'same-origin',
            headers: { 'Content-Type': 'application/octet-stream', 'X-Target-Path': item.serverPath, 'X-Restart-Mode': rmode, 'X-Confirm-Pass': sudoHash },
            body: item.file
        });

        if (res.status === 401) { logout(); return; }
        if (!res.ok) {
            const err = await res.json().catch(() => ({ error: "Erro desconhecido" }));
            statusDiv.innerHTML = `<span style='color: var(--danger)'>FALHA em ${item.serverPath}: ${sanitizeHTML(err.error)}</span>`;
            btnReload.disabled = false; btnReload.innerText = "Trigger Hot-Reload";
            return;
        }
    }

    for (let i = 0; i < deleteQueue.length; i++) {
        const path = deleteQueue[i];
        operationsDone++;
        const isLast = (operationsDone === totalOperations);
        const rmode = isLast ? mode : 'none';

        statusDiv.innerHTML = `<span style='color: var(--warning)'>Removendo: [${i + 1}/${deleteQueue.length}] ${path}</span>`;

        const res = await fetch('/manager/api/delete', {
            method: 'POST', credentials: 'same-origin',
            headers: { 'X-Target-Path': path, 'X-Restart-Mode': rmode, 'X-Confirm-Pass': sudoHash }
        });

        if (res.status === 401) { logout(); return; }
        if (!res.ok) {
            const err = await res.json().catch(() => ({ error: "Erro ao deletar" }));
            statusDiv.innerHTML = `<span style='color: var(--danger)'>FALHA ao deletar ${path}: ${sanitizeHTML(err.error)}</span>`;
            btnReload.disabled = false; btnReload.innerText = "Trigger Hot-Reload";
            return;
        }
    }

    statusDiv.innerHTML = `<span style='color: var(--text-main)'>Full Mirror Sync Concluído! ${uploadQueue.length} arquivo(s) atualizado(s), ${deleteQueue.length} arquivo(s) removido(s).</span>`;
    fileInput.value = ''; lblDisplay.innerText = "[ Clique para Selecionar a Pasta (Sync) ]";
    btnReload.disabled = false;
    btnReload.innerText = "Trigger Hot-Reload";
}

async function actionSystemRestart(mode) {
    const modeName = mode === 'api' ? 'API (arc_server)' : 'Full Core';
    const bodyHtml = `<p>Você tem certeza que deseja forçar a recompilação e reinício de: <strong style="color:var(--warning);">${modeName}</strong>?</p><p style="color: var(--text-muted); font-size: 0.9em; margin-top: 5px;">Confirme sua senha Sudo para autorizar.</p>`;

    const result = await modalManager.open('modal_restart_title', bodyHtml);
    if (!result) return;

    const res = await fetch('/manager/api/system/restart', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: mode, confirm_pass: result.confirm_pass })
    });

    if (res.ok) {
        alert(`Sinal de reinício (${modeName}) enviado com sucesso! Aguarde enquanto o servidor recompila...`);
    } else {
        const err = await res.json().catch(() => ({ error: "Falha ao enviar sinal de reinício." }));
        alert(err.error);
    }
}

async function sendTTYText() {
    const text = document.getElementById('tty-text').value;
    if (!text) return;
    document.getElementById('tty-status').innerText = "Criptografando payload...";
    document.getElementById('tty-status').style.color = "var(--warning)";

    // Encode seguro previne Plain Text e lida nativamente com Acentuação UTF-8
    const b64Text = btoa(unescape(encodeURIComponent(text)));

    const res = await fetch('/manager/api/tty/text', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ text: b64Text })
    });

    if (res.ok) {
        document.getElementById('tty-status').innerText = "Texto impresso fisicamente no servidor!";
        document.getElementById('tty-status').style.color = "var(--text-main)";
        document.getElementById('tty-text').value = '';
    } else {
        document.getElementById('tty-status').innerText = "Falha ao enviar texto.";
        document.getElementById('tty-status').style.color = "var(--danger)";
    }
}

async function clearTTY() {
    const res = await fetch('/manager/api/tty/clear', { method: 'POST', credentials: 'same-origin' });
    if (res.ok) document.getElementById('tty-status').innerHTML = "<span style='color: var(--text-main)'>Tela do servidor TTY1 limpa.</span>";
    else document.getElementById('tty-status').innerHTML = "<span style='color: var(--danger)'>Falha ao limpar tela.</span>";
}

async function sendTTYLogo() {
    document.getElementById('tty-status').innerText = "Processando logo animada...";
    const res = await fetch('/manager/api/tty/logo', { method: 'POST', credentials: 'same-origin' });
    if (res.ok) document.getElementById('tty-status').innerHTML = "<span style='color: var(--text-main)'>Animação rodando no TTY1!</span>";
    else document.getElementById('tty-status').innerHTML = "<span style='color: var(--danger)'>Falha ao enviar logo.</span>";
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
            // Pequeno delay para acionar a transição CSS suave
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
    const myRole = parseInt(sessionStorage.getItem('arc_admin_role') || '2');

    if (data && data.admins) {
        tbody.innerHTML = '';
        data.admins.forEach(admin => {
            const roleNames = ["ROOT", "ADMIN", "SUP"];
            let roleHtml = roleNames[admin.role] || "UNKNOWN";

            const tr = document.createElement('tr');
            tr.style.cursor = 'pointer';
            tr.onclick = () => selectAdmin(admin.user, admin.role);

            tr.innerHTML = `
                <td style="text-align: center;"><input type="radio" name="admin_select" value="${admin.user}"></td>
                <td>${sanitizeHTML(admin.user)}</td>
                <td>${roleHtml}</td>
            </tr>`;
            tbody.appendChild(tr);
        });
        // Clear selection on refresh
        document.getElementById('admin-actions-bar').style.display = 'none';
        selectedAdminUser = null;
        selectedAdminRole = null;
    }
}

function selectAdmin(user, role) {
    selectedAdminUser = user;
    selectedAdminRole = role;

    document.querySelectorAll('input[name="admin_select"]').forEach(radio => {
        radio.checked = (radio.value === user);
    });

    const actionsBar = document.getElementById('admin-actions-bar');
    const actionsContainer = document.getElementById('admin-action-buttons');
    const myRole = parseInt(sessionStorage.getItem('arc_admin_role') || '2');

    actionsBar.style.display = 'block';
    document.getElementById('selected-admin-name').innerText = sanitizeHTML(user);
    actionsContainer.innerHTML = '';

    if (myRole === 0) {
        if (user !== 'admin') {
            actionsContainer.innerHTML += `<button onclick="actionChangeRole()" class="btn-reload" style="border-color: var(--info); color: var(--info);">Alterar Cargo</button>`;
            actionsContainer.innerHTML += `<button onclick="actionDelete()" class="btn-reload" style="border-color: var(--danger); color: var(--danger);">Deletar Usuário</button>`;
        }
        actionsContainer.innerHTML += `<button onclick="actionResetPass()" class="btn-reload" style="border-color: var(--warning); color: var(--warning);">Resetar Senha</button>`;
    } else if (myRole === 1) {
        if (role === 2) {
            actionsContainer.innerHTML += `<button onclick="actionResetPass()" class="btn-reload" style="border-color: var(--warning); color: var(--warning);">Resetar Senha</button>`;
        } else {
            actionsContainer.innerHTML += `<span style="color: var(--text-muted);">Acesso Negado (Alvo tem mesma ou superior hierarquia).</span>`;
        }
    }
}

async function actionChangeRole() {
    const targetUser = selectedAdminUser;
    const targetRole = selectedAdminRole;
    if (!targetUser) return;

    const bodyHtml = `
        <p style="margin-bottom: 15px;">Selecione o novo cargo para <strong style="color:var(--text-main);">${sanitizeHTML(targetUser)}</strong>:</p>
        <select id="modalExtraInput">
            <option value="0" ${targetRole === 0 ? 'selected' : ''}>ROOT (Nível 0)</option>
            <option value="1" ${targetRole === 1 ? 'selected' : ''}>ADMIN (Nível 1)</option>
            <option value="2" ${targetRole === 2 ? 'selected' : ''}>SUP (Nível 2)</option>
        </select>
    `;

    const result = await modalManager.open('modal_role_title', bodyHtml);
    if (!result) return;

    const res = await fetch('/manager/api/admin/role', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: targetUser, role: parseInt(result.extraData), confirm_pass: result.confirm_pass })
    });

    if (res.ok) fetchAdmins();
    else { const err = await res.json().catch(() => ({ error: "Falha ao alterar cargo." })); alert(err.error); }
}

async function actionDelete() {
    const targetUser = selectedAdminUser;
    if (!targetUser) return;

    const bodyHtml = `<p>Tem certeza que deseja deletar permanentemente <strong style="color:var(--danger);">${sanitizeHTML(targetUser)}</strong>?</p>`;
    const result = await modalManager.open('modal_del_title', bodyHtml);
    if (!result) return;

    const res = await fetch('/manager/api/admin/delete', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: targetUser, confirm_pass: result.confirm_pass })
    });

    if (res.ok) fetchAdmins();
    else { const err = await res.json().catch(() => ({ error: "Falha ao deletar." })); alert(err.error); }
}

async function actionResetPass() {
    const targetUser = selectedAdminUser;
    if (!targetUser) return;
    const newPass = prompt(`Digite a nova senha para o administrador '${targetUser}':`);
    if (!newPass) return;
    const passHash = await sha256(newPass);
    const res = await fetch('/manager/api/admin/update', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: targetUser, pass: passHash })
    });
    if (res.ok) alert('Senha atualizada com sucesso!');
    else alert('Falha ao atualizar a senha.');
}

async function createAdmin() {
    const u = document.getElementById('new-admin-user').value;
    const p = document.getElementById('new-admin-pass').value;
    if (!u || !p) return;

    const bodyHtml = `<p>Confirme sua senha Sudo para registrar <strong style="color:var(--text-main);">${sanitizeHTML(u)}</strong>.</p>`;
    const result = await modalManager.open('modal_create_title', bodyHtml);
    if (!result) return;

    document.getElementById('admin-status').innerText = "Criptografando...";
    const passHash = await sha256(p);

    const res = await fetch('/manager/api/admin/create', {
        method: 'POST', credentials: 'same-origin', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user: u, pass: passHash, role: 2, confirm_pass: result.confirm_pass })
    });

    if (res.ok) {
        document.getElementById('admin-status').innerText = "Administrador criado com sucesso! (Salvo via SHA-256)";
        document.getElementById('admin-status').style.color = "var(--text-main)";
        document.getElementById('new-admin-user').value = '';
        document.getElementById('new-admin-pass').value = '';
        fetchAdmins();
        switchUserTab('view-users'); // Volta para visualização automaticamente
    } else {
        const err = await res.json().catch(() => ({ error: "Erro ao criar administrador." }));
        document.getElementById('admin-status').innerText = err.error;
        document.getElementById('admin-status').style.color = "var(--danger)";
    }
}

async function validateSession() {
    try {
        // Um ping rápido que passará pelo middleware de sessão
        const res = await fetch('/manager/api/metrics', { credentials: 'same-origin' });
        if (res.status === 401) logout();
    } catch (err) {
        // Ignora erros puros de rede para evitar deslogar se a sua internet cair
    }
}

checkAuth();
setInterval(() => { pollData(); }, 3000);
setInterval(validateSession, 10000);