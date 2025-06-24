function getCookie(name) {
    const match = document.cookie.match(new RegExp('(?:^|; )' + name.replace(/([.$?*|{}()\[\]\\/+^])/g, '\\$1') + '=([^;]*)'));
    return match ? decodeURIComponent(match[1]) : null;
}

function setCookie(name, value, days) {
    const date = new Date();
    date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
    document.cookie = `${name}=${encodeURIComponent(value)}; Path=/; Expires=${date.toUTCString()}`;
}

function applyCookieTheme() {
    const theme = getCookie('gallery_theme');
    const personality = getCookie('personality_mode');
    if (theme) document.body.classList.add('theme-' + theme);
    if (personality) document.body.classList.add('personality-' + personality);

    let visitCount = parseInt(getCookie('visit_count') || '0', 10) + 1;
    setCookie('visit_count', visitCount, 365);
    const el = document.getElementById('visit-count');
    if (el) el.textContent = visitCount;

    const visitor = getCookie('visitor_name');
    if (visitor) {
        showWelcomeMessage(`Welcome back, ${visitor}! ✨`);
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
