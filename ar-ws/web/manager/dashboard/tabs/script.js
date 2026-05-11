const translations = {
    en: {
        lang_btn: "PT-BR",
        hero_p: '"From a vision in 2020 to a multi-disciplinary tech reality."',
        hero_btn: "Our Divisions",
        story_h2: "Our History",
        story_p1: "The <strong>ALRI GROUP</strong> started as a dream in <strong>2020</strong>. What began as a conceptual idea from its founder has evolved over the years into a developing ecosystem of specialized technological subdivisions.",
        story_p2: "Born from a passion for understanding how systems work and how to push their limits, the organization is now transitioning from a personal project to a professional organization.",
        struct_h2: "Corporate Structure",
        struct_p: "The ALRI GROUP acts as a <strong>Holding Company</strong>, serving as the parent organization for a set of specialized technological subsidiaries.",
        ard_p: "Systems engineering and deep kernel modifications.",
        arga_p: "Interactive experience development and digital art.",
        arcs_p: "Government-level offensive and defensive security.",
        proj_h2: "Elite Projects",
        wmaros_p: "The <strong>Windows Mod ALRI Operating System</strong> is our performance flagship. A Windows Professional environment rebuilt and optimized to deliver maximum FPS and the lowest possible latency for power users.",
        arbemf_p: "<strong>AR-BEMF:</strong> The backbone of our web operations. A native C micro-framework focused on E2EE (End-to-End Encryption) and granular hot-reloading for systems that cannot stop.",
        lic_h2: "ARGL Licensing System",
        lic_p: "Our licenses (ALRI Group Licenses) ensure the balance between open innovation and institutional security.",
        lic_arglp: "Free usage and modification for non-commercial purposes (Open Source).",
        lic_arglfu: "Free to use and distribute, but prohibited from undergoing modifications.",
        lic_arglr: "Restricted use for partners and clients. Hardened code with warranty.",
        founder_h2: "Mission & Founder",
        founder_p1: "Our goal is to provide innovative solutions in the most demanding areas of technology. We solve complex problems through our specialized branches:",
        founder_li1: "Identification and remediation of critical flaws.",
        founder_li2: "Custom solutions for digital challenges.",
        founder_li3: "Bespoke tools for specific hardware.",
        founder_p2: '<strong>Founder:</strong> ALRI GROUP was founded and is led by <strong>Alexsander (@alexsanderalri)</strong>. The name "ALRI" is an acronym derived from his surnames, <strong>Al</strong>meida + <strong>Ri</strong>beiro. With a strong background in security research and deep system modifications, the founder\'s vision remains as the pillar of every project.'
    },
    pt: {
        lang_btn: "EN-US",
        hero_p: '"De uma visão em 2020 para uma realidade tecnológica multidisciplinar."',
        hero_btn: "Nossas Divisões",
        story_h2: "Nossa História",
        story_p1: "O <strong>ALRI GROUP</strong> começou como um sonho em <strong>2020</strong>. O que iniciou como uma ideia conceitual de seu fundador evoluiu ao longo dos anos para um ecossistema em desenvolvimento de subdivisões tecnológicas especializadas.",
        story_p2: "Nascido da paixão por entender como os sistemas funcionam e como ultrapassar seus limites, a organização está agora em transição de um projeto pessoal para uma organização profissional.",
        struct_h2: "Estrutura Corporativa",
        struct_p: "O ALRI GROUP atua como uma <strong>Holding Company</strong>, servindo como a organização matriz para um conjunto de subsidiárias tecnológicas especializadas.",
        ard_p: "Engenharia de sistemas e modificações profundas de kernel.",
        arga_p: "Desenvolvimento de experiências interativas e arte digital.",
        arcs_p: "Segurança ofensiva e defensiva de nível governamental.",
        proj_h2: "Projetos de Elite",
        wmaros_p: "O <strong>Windows Mod ALRI Operating System</strong> é a nossa flagship de performance. Um ambiente Windows Professional reconstruído e otimizado para entregar o máximo de FPS e a menor latência possível para power users.",
        arbemf_p: "<strong>AR-BEMF:</strong> A espinha dorsal de nossas operações web. Um micro-framework em C nativo, focado em E2EE (Criptografia de Ponta a Ponta) e hot-reloading granular para sistemas que não podem parar.",
        lic_h2: "Sistema de Licenciamento ARGL",
        lic_p: "Nossas licenças (ALRI Group Licenses) garantem o equilíbrio entre inovação aberta e segurança institucional.",
        lic_arglp: "Uso e modificação livres para fins não comerciais (Open Source).",
        lic_arglfu: "Livre para uso e distribuição, mas proibido de sofrer modificações.",
        lic_arglr: "Uso restrito a parceiros e clientes. Código blindado com garantia.",
        founder_h2: "Missão & Fundador",
        founder_p1: "Nosso objetivo é fornecer soluções inovadoras nas áreas mais exigentes da tecnologia. Nós resolvemos problemas complexos através de nossas filiais especializadas:",
        founder_li1: "Identificação e correção de falhas críticas.",
        founder_li2: "Soluções customizadas para desafios digitais.",
        founder_li3: "Ferramentas sob medida para hardwares específicos.",
        founder_p2: '<strong>Fundador:</strong> O ALRI GROUP foi fundado e é liderado por <strong>Alexsander (@alexsanderalri)</strong>. O nome "ALRI" é um acrônimo derivado de seus sobrenomes, <strong>Al</strong>meida + <strong>Ri</strong>beiro. Com forte base em security research e modificações profundas de sistema, a visão do fundador se mantém como pilar de todo projeto.'
    }
};

let currentLang = localStorage.getItem('alri_lang') || 'en';

function updateUI() {
    const langData = translations[currentLang];
    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.getAttribute('data-i18n');
        if (langData[key]) {
            el.innerHTML = langData[key];
        }
    });
    document.getElementById('langText').innerText = langData.lang_btn;
    document.documentElement.lang = currentLang === 'en' ? 'en' : 'pt-BR';
}

function toggleLanguage() {
    currentLang = currentLang === 'en' ? 'pt' : 'en';
    localStorage.setItem('alri_lang', currentLang);
    updateUI();
}

document.addEventListener('DOMContentLoaded', () => {
    updateUI();

    // O "Botão Secreto" escondido no cadeado do footer
    const easterEgg = document.getElementById('easter-egg');
    if (easterEgg) {
        easterEgg.addEventListener('click', () => {
            window.location.href = '/manager/login';
        });
    }

    // Engine de Animação de rolagem (Staggered Fade-in)
    const fadeElements = document.querySelectorAll('.fade-in');

    const observer = new IntersectionObserver((entries, observer) => {
        let delay = 0;
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                // Staggered effect for grid elements
                if (entry.target.classList.contains('stagger-item')) {
                    setTimeout(() => {
                        entry.target.classList.add('visible');
                    }, delay);
                    delay += 150; // Adiciona 150ms entre cada item
                } else {
                    entry.target.classList.add('visible');
                }
                observer.unobserve(entry.target); // Impede que a animação repita
            }
        });
    }, { threshold: 0.10, rootMargin: "0px 0px -50px 0px" });

    fadeElements.forEach(el => observer.observe(el));

    // 3D Tilt Effect on Cards
    const cards = document.querySelectorAll('.card');
    cards.forEach(card => {
        card.addEventListener('mousemove', (e) => {
            const rect = card.getBoundingClientRect();
            const x = e.clientX - rect.left; // Mouse X na div
            const y = e.clientY - rect.top;  // Mouse Y na div
            const centerX = rect.width / 2;
            const centerY = rect.height / 2;
            const rotateX = ((y - centerY) / centerY) * -5; // Max 5 graus de inclinação
            const rotateY = ((x - centerX) / centerX) * 5;
            card.style.transform = `perspective(1000px) rotateX(${rotateX}deg) rotateY(${rotateY}deg) scale3d(1.02, 1.02, 1.02)`;
        });
        card.addEventListener('mouseleave', () => {
            card.style.transform = `perspective(1000px) rotateX(0deg) rotateY(0deg) scale3d(1, 1, 1)`;
            card.style.transition = 'transform 0.5s ease'; // Transição suave ao sair
        });
        card.addEventListener('mouseenter', () => {
            card.style.transition = 'none'; // Remove transição contínua ao mover
        });
    });

    // Efeito de Paralaxe Global para as Esferas de Fundo (Liquid Orbs)
    document.addEventListener('mousemove', (e) => {
        const x = (e.clientX / window.innerWidth - 0.5) * 60;
        const y = (e.clientY / window.innerHeight - 0.5) * 60;
        document.documentElement.style.setProperty('--mouse-x', `${x}px`);
        document.documentElement.style.setProperty('--mouse-y', `${y}px`);
    });
});