const translations = {
    en: {
        lang_btn: "PT-BR",
        title: "SECURE PORTAL",
        subtitle: "Authenticate to access the ALRI CWB dashboard.",
        ph_user: "Administrator ID",
        ph_pass: "Secure Password",
        btn_login: "ACCESS SYSTEM",
        security_notice: "E2EE Client-Side Hashing Enabled",
        err_fill: "Please fill in all fields.",
        err_network: "Connection to server failed.",
        msg_wait: "Authenticating..."
    },
    pt: {
        lang_btn: "EN-US",
        title: "PORTAL SEGURO",
        subtitle: "Autentique-se para acessar o painel ALRI CWB.",
        ph_user: "ID do Administrador",
        ph_pass: "Senha Segura",
        btn_login: "ACESSAR SISTEMA",
        security_notice: "Hashing E2EE no Cliente Ativado",
        err_fill: "Por favor, preencha todos os campos.",
        err_network: "Falha de conexão com o servidor.",
        msg_wait: "Autenticando..."
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
    document.documentElement.lang = currentLang === 'en' ? 'en' : 'pt-BR';
}

function toggleLanguage() {
    currentLang = currentLang === 'en' ? 'pt' : 'en';
    localStorage.setItem('alri_lang', currentLang);
    updateUI();
}

function toggleThemeMenu() {
    document.getElementById('themeMenu').classList.toggle('active');
}

function setTheme(color, glow) {
    document.documentElement.style.setProperty('--accent', color);
    document.documentElement.style.setProperty('--accent-glow', glow);
    localStorage.setItem('arc_accent', color);
    localStorage.setItem('arc_glow', glow);

    const orb1 = document.querySelector('.orb-1');
    if (orb1) orb1.style.background = color;

    document.getElementById('themeMenu').classList.remove('active');
}

function applyTheme() {
    const savedColor = localStorage.getItem('arc_accent');
    const savedGlow = localStorage.getItem('arc_glow');
    if (savedColor && savedGlow) setTheme(savedColor, savedGlow);
}

document.addEventListener('click', (e) => {
    if (!e.target.closest('.theme-dropdown')) {
        const menu = document.getElementById('themeMenu');
        if (menu) menu.classList.remove('active');
    }
});

async function sha256(message) {
    const msgBuffer = new TextEncoder().encode(message);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

function showStatus(message, isError) {
    const statusEl = document.getElementById('statusMessage');
    statusEl.innerText = message;
    statusEl.className = 'status-message ' + (isError ? 'status-error' : 'status-success');
}

async function handleLogin(e) {
    e.preventDefault();
    const u = document.getElementById('username').value.trim();
    const p = document.getElementById('password').value;
    const btn = document.getElementById('submitBtn');
    const langData = translations[currentLang];

    if (!u || !p) { showStatus(langData.err_fill, true); return; }
    btn.disabled = true; showStatus(langData.msg_wait, false);

    try {
        const passHash = await sha256(p);
        const res = await fetch('/manager/api/login', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ user: u, pass: passHash }) });
        const data = await res.json().catch(() => ({}));

        if (res.ok) {
            showStatus("Success! Redirecting...", false);
            sessionStorage.setItem('arc_admin_flag', 'true'); sessionStorage.setItem('arc_admin_role', data.role); sessionStorage.setItem('arc_admin_user', u);
            setTimeout(() => { window.location.href = '/manager/dashboard'; }, 500);
        } else { showStatus(data.error || "Authentication failed.", true); btn.disabled = false; }
    } catch (err) { showStatus(langData.err_network, true); btn.disabled = false; }
}

document.addEventListener('DOMContentLoaded', () => { applyTheme(); updateUI(); setTimeout(() => document.getElementById('username').focus(), 500); });