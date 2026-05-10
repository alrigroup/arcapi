if (sessionStorage.getItem('arc_admin_flag')) {
    // Ping no backend para verificar se o Cookie ainda é válido antes de redirecionar cego
    fetch('/manager/api/metrics', { credentials: 'same-origin', cache: 'no-store' })
        .then(res => {
            if (res.ok) {
                window.location.href = '/manager/dashboard';
            } else {
                sessionStorage.removeItem('arc_admin_flag');
            }
        })
        .catch(() => sessionStorage.removeItem('arc_admin_flag'));
}

// Client-Side Hashing Security 
async function sha256(message) {
    const msgBuffer = new TextEncoder().encode(message);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

const translations = {
    en: {
        lang_btn: "PT-BR",
        ph_user: "Username",
        ph_pass: "Password",
        login_btn: "AUTHENTICATE",
        login_footer: "© 2026 ALRI Development // Secure Protocol",
        err_invalid: "Invalid credentials.",
        err_conn: "Connection error."
    },
    pt: {
        lang_btn: "EN-US",
        ph_user: "Usuário",
        ph_pass: "Senha",
        login_btn: "AUTENTICAR",
        login_footer: "© 2026 ALRI Development // Protocolo Seguro",
        err_invalid: "Credenciais inválidas.",
        err_conn: "Erro de conexão."
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

async function doLogin() {
    const user = document.getElementById('login-user').value;
    const pass = document.getElementById('login-pass').value;
    const errDiv = document.getElementById('login-error');
    const btn = document.getElementById('login-btn');

    btn.innerText = "Authenticating...";
    btn.disabled = true;

    try {
        const passHash = await sha256(pass);

        const res = await fetch('/manager/api/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ user: user, pass: passHash })
        });

        if (res.ok) {
            const data = await res.json();
            sessionStorage.setItem('arc_admin_flag', '1');
            sessionStorage.setItem('arc_admin_role', data.role);
            errDiv.innerText = "";
            window.location.href = '/manager/dashboard';
        } else {
            const err = await res.json().catch(() => ({}));
            errDiv.innerText = err.error || "Invalid credentials or Blocked IP.";
        }
    } catch (e) {
        errDiv.innerText = "Connection failed. Check Server SSL.";
    }
    btn.innerText = "AUTHENTICATE";
    btn.disabled = false;
}