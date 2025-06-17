function getCookie(name) {
    const match = document.cookie.match(new RegExp('(?:^|; )' + name.replace(/([.$?*|{}()\[\]\\/+^])/g, '\\$1') + '=([^;]*)'));
    return match ? decodeURIComponent(match[1]) : null;
}

function applyCookieTheme() {
    const theme = getCookie('gallery_theme');
    const personality = getCookie('personality_mode');
    if (theme) document.body.classList.add('theme-' + theme);
    if (personality) document.body.classList.add('personality-' + personality);
    const visitCount = getCookie('visit_count');
    if (visitCount) {
        const el = document.getElementById('visit-count');
        if (el) el.textContent = visitCount;
    }
}

function showWelcomeMessage(msg) {
    const div = document.createElement('div');
    div.className = 'welcome-msg';
    div.textContent = msg;
    div.style.cssText = 'position:fixed;top:10px;left:50%;transform:translateX(-50%);background:#ff69b4;color:#fff;padding:10px 20px;border-radius:20px;z-index:1000;';
    document.body.appendChild(div);
    setTimeout(() => div.remove(), 3000);
}

document.addEventListener('DOMContentLoaded', applyCookieTheme);
