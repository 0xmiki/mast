// Copyright 2026 The Mast Authors

#include "mast/browser/mast_pages.h"

#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace mast {

namespace {

// Shared look: matches the Mast demo (dark, ember accent).
constexpr char kStyle[] = R"MAST(
:root { color-scheme: dark; --bg:#0c0d0f; --chrome:#141518; --raised:#22242a;
  --line:rgb(255 255 255/.08); --text:#ececef; --muted:#9a9ca5; --faint:#6b6e78;
  --ember:#ff6a3d; --ember2:#ff8f5e; --soft:rgb(255 106 61/.14); }
* { box-sizing:border-box }
html,body { margin:0; min-height:100%; background:var(--bg); color:var(--text);
  font:14px/1.45 "Inter Variable",Inter,"Noto Sans","DejaVu Sans",system-ui,sans-serif; -webkit-font-smoothing:antialiased }
button { font:inherit; color:inherit; cursor:pointer }
.eyebrow { margin:0 0 12px; font-size:11.5px; font-weight:650; letter-spacing:.08em; text-transform:uppercase; color:var(--faint) }
.btn { display:inline-flex; align-items:center; justify-content:center; gap:8px; height:38px; padding:0 18px;
  border:1px solid rgb(255 255 255/.12); border-radius:10px; background:var(--raised); font-weight:600 }
.btn:hover { background:#2a2c33 }
.btn.primary { background:var(--ember); border-color:transparent; color:#1a0b05 }
.btn.primary:hover { background:var(--ember2) }
.btn.ghost { background:none; border-color:transparent; color:var(--muted) }
.time { font-family:"JetBrains Mono","DejaVu Sans Mono","Noto Sans Mono",ui-monospace,monospace; font-weight:600; letter-spacing:-.02em; font-variant-numeric:tabular-nums }
)MAST";

constexpr char kLogo[] = R"MAST(<svg width="28" height="28" viewBox="0 0 32 32" aria-label="Mast"><defs><linearGradient id="g" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ff6a3d"/><stop offset="1" stop-color="#c2300f"/></linearGradient></defs><rect width="32" height="32" rx="8" fill="url(#g)"/><path d="M14 10.5V24.5H6.6Q7.4 16 14 10.5Z" fill="#fff" stroke="#fff" stroke-width="1.2" stroke-linejoin="round"/><path d="M17 6V24.5H25.4Q24.7 13 17 6Z" fill="#fff" stroke="#fff" stroke-width="1.2" stroke-linejoin="round"/></svg>)MAST";

constexpr char kBlockPage[] = R"MAST(<!doctype html>
<html><head><meta charset="utf-8"><title>{{HOST}} is blocked</title><style>{{STYLE}}
body { display:grid; place-items:center; min-height:100vh; padding:48px 24px; text-align:center;
  background:radial-gradient(60% 50% at 50% 0%,rgb(255 106 61/.14),transparent 70%),var(--bg) }
.seal { display:grid; place-items:center; width:76px; height:76px; margin:0 auto 28px; border-radius:50%;
  color:var(--ember); background:var(--soft); box-shadow:0 0 0 1px rgb(255 106 61/.35),0 0 0 10px rgb(255 106 61/.06);
  animation:breathe 4s ease-in-out infinite }
@keyframes breathe { 50% { box-shadow:0 0 0 1px rgb(255 106 61/.35),0 0 0 18px rgb(255 106 61/.03) } }
.eyebrow { color:var(--ember) }
h1 { margin:0; font-size:34px; line-height:1.15; letter-spacing:-.02em }
.time { display:block; margin:36px 0 6px; font-size:64px; line-height:1 }
.label { color:var(--muted); font-size:13px }
.btn { margin-top:32px }
</style></head><body><main>
<div class="seal"><svg width="30" height="30" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="11" width="16" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/></svg></div>
<p class="eyebrow">Deep work</p>
<h1>{{HOST}} is blocked.</h1>
<span class="time" id="t">--:--</span>
<span class="label" id="l"></span><br>
<button class="btn" id="back" onclick="history.back()">&larr; Back to work</button>
</main><script>
const end = {{END}};
const t = document.getElementById('t');
document.getElementById('l').textContent =
  'Opens at ' + new Date(end).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'});
if (history.length < 2) document.getElementById('back').style.display = 'none';
function pad(n) { return String(n).padStart(2, '0'); }
function tick() {
  const s = Math.max(0, Math.ceil((end - Date.now()) / 1000));
  const h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60);
  t.textContent = (h ? h + ':' : '') + pad(m) + ':' + pad(s % 60);
  if (s === 0) location.reload();
}
tick(); setInterval(tick, 250);
</script></body></html>)MAST";

constexpr char kStartPage[] = R"MAST(<!doctype html>
<html><head><meta charset="utf-8"><title>{{TITLE}}</title><style>{{STYLE}}
body { padding:10vh 24px 64px; background:radial-gradient(70% 45% at 50% 0%,rgb(255 106 61/.07),transparent 70%),var(--bg) }
body.running { background:radial-gradient(70% 50% at 50% 0%,rgb(255 106 61/.14),transparent 70%),var(--bg) }
header { position:fixed; top:18px; left:22px; display:flex; align-items:center; gap:10px; font-weight:650; font-size:15px }
main { display:flex; flex-direction:column; align-items:center; text-align:center }
.lengths { display:flex; flex-wrap:wrap; justify-content:center; gap:4px; padding:4px; border-radius:14px;
  background:var(--chrome); border:1px solid var(--line) }
.lengths button { min-width:64px; height:38px; padding:0 14px; border:0; border-radius:10px; background:none; color:var(--muted); font-weight:600 }
.lengths button:hover { color:var(--text) }
.lengths button.on { background:#2a2c33; color:var(--text); box-shadow:inset 0 0 0 1px rgb(255 255 255/.12) }
.go { width:100%; max-width:320px; height:52px; margin-top:16px; border-radius:14px; font-size:16px; font-weight:700;
  box-shadow:0 10px 40px rgb(255 106 61/.25) }
.note { margin:14px 0 0; font-size:12.5px; color:var(--faint) }
.note a { color:var(--muted); cursor:pointer; text-decoration:underline }
.running .eyebrow { color:var(--ember2) }
.big { font-size:96px; line-height:1 }
.sub { margin:14px 0 0; color:var(--muted) }
.row { display:flex; gap:8px; margin-top:22px }
section { width:min(720px,100%); margin:64px auto 0; text-align:left }
section h2 { margin:0 0 4px; font-size:20px }
section > p { margin:0 0 18px; color:var(--muted) }
form { display:flex; gap:8px }
input { flex:1; height:38px; padding:0 12px; border-radius:10px; border:1px solid rgb(255 255 255/.12);
  background:var(--chrome); color:var(--text); outline:0; font:inherit }
input:focus { border-color:var(--ember) }
.chips { display:flex; flex-wrap:wrap; gap:6px; margin-top:12px }
.chip { display:inline-flex; align-items:center; gap:6px; height:30px; padding:0 10px; border-radius:9px;
  background:var(--chrome); border:1px solid var(--line); color:var(--muted); font-size:13px }
.chip.mine { background:var(--raised); color:var(--text) }
.chip button { border:0; background:none; color:var(--muted); padding:0 2px; font-size:15px; line-height:1 }
.hint { margin:10px 0 0; font-size:12.5px; color:var(--ember2) }
</style></head><body>
<header>{{LOGO}} Mast</header>
<main id="app"></main>
<section id="list" hidden>
  <h2>Blocklist</h2>
  <p>Every session blocks all of these. The usual suspects are already here.</p>
  <form id="add"><input id="site" placeholder="Add a site, e.g. espn.com" spellcheck="false">
    <button class="btn primary">Add</button></form>
  <div class="chips" id="mine"></div>
  <p class="hint" id="lockhint" hidden>A session is running: you can add sites now, but not remove them.</p>
  <div class="chips" id="presets"></div>
</section>
<script>
const LENGTHS = [25, 50, 90, 120, 180, 240];
let state = null, minutes = null;
const app = document.getElementById('app');
const label = m => m < 60 ? m + 'm' : (m % 60 ? Math.floor(m / 60) + 'h ' + m % 60 + 'm' : m / 60 + 'h');
const pad = n => String(n).padStart(2, '0');
const clock = t => new Date(t).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'});
function countdown(ms) {
  const s = Math.max(0, Math.ceil(ms / 1000)), h = Math.floor(s / 3600);
  return (h ? h + ':' : '') + pad(Math.floor(s % 3600 / 60)) + ':' + pad(s % 60);
}
function send(msg, args) { chrome.send(msg, args || []); }

window.mastState = s => {
  const was = state && state.active;
  state = s;
  if (minutes === null) minutes = s.lastMinutes;
  if (was !== s.active) app.dataset.mode = '';
  render();
};

function render() {
  if (!state) return;
  const active = state.active && Date.now() < state.endsAt;
  document.body.classList.toggle('running', active);
  const mode = active ? 'run' : 'idle';
  if (app.dataset.mode !== mode) {
    app.dataset.mode = mode;
    app.innerHTML = active
      ? `<p class="eyebrow">Deep work</p><div class="time big" id="cd"></div>
         <p class="sub" id="until"></p>
         <div class="row"><button class="btn" id="extend">+ 30 min</button>
           <button class="btn ghost" id="undo" hidden></button></div>
         <p class="note" id="nostop">There’s no stop button. That’s the point.</p>`
      : `<p class="eyebrow">Block distractions for</p><div class="lengths" id="lengths"></div>
         <button class="btn primary go" id="go"></button>
         <p class="note"><span id="count"></span> sites · <a id="show">see the list</a></p>`;
    if (active) {
      document.getElementById('extend').onclick = () => send('extend', [30]);
      document.getElementById('undo').onclick = () => send('undo');
    } else {
      document.getElementById('go').onclick = () => send('start', [minutes]);
      document.getElementById('show').onclick = () => {
        const list = document.getElementById('list');
        list.hidden = false;
        list.scrollIntoView({behavior: 'smooth'});
      };
    }
  }
  const total = state.presets.length + state.custom.length;
  if (active) {
    document.getElementById('cd').textContent = countdown(state.endsAt - Date.now());
    document.getElementById('until').textContent = 'Until ' + clock(state.endsAt) + ' · ' + total + ' sites blocked';
    const undoLeft = Math.ceil((state.startedAt + 10000 - Date.now()) / 1000);
    const undo = document.getElementById('undo');
    undo.hidden = undoLeft <= 0;
    undo.textContent = 'Undo · ' + undoLeft + 's';
    document.getElementById('nostop').hidden = undoLeft > 0;
  } else {
    document.getElementById('lengths').innerHTML = LENGTHS.map(m =>
      `<button data-m="${m}" class="${m === minutes ? 'on' : ''}">${label(m)}</button>`).join('');
    document.querySelectorAll('#lengths button').forEach(b =>
      b.onclick = () => { minutes = Number(b.dataset.m); render(); });
    document.getElementById('go').textContent = 'Start · ' + label(minutes);
    document.getElementById('count').textContent = total;
  }
  renderList(active);
}

function renderList(active) {
  const mine = document.getElementById('mine');
  const key = active + '|' + state.custom.join();
  if (mine.dataset.key !== key) {
    mine.dataset.key = key;
    mine.innerHTML = state.custom.map(d => `<span class="chip mine">${d}${active
      ? ' 🔒' : `<button data-d="${d}" title="Remove">×</button>`}</span>`).join('');
    mine.querySelectorAll('button').forEach(b => b.onclick = () => send('removeSite', [b.dataset.d]));
  }
  document.getElementById('lockhint').hidden = !active;
  const presets = document.getElementById('presets');
  if (!presets.childElementCount) {
    presets.innerHTML = state.presets.map(d => `<span class="chip">${d}</span>`).join('');
  }
}

document.getElementById('add').onsubmit = e => {
  e.preventDefault();
  const input = document.getElementById('site');
  if (input.value.trim()) send('addSite', [input.value]);
  input.value = '';
};

send('getState');
setInterval(render, 250);
setInterval(() => send('getState'), 3000);
</script></body></html>)MAST";

std::string Fill(std::string page) {
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{STYLE}}", kStyle);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{LOGO}}", kLogo);
  return page;
}

}  // namespace

std::string BlockPageHtml(std::string_view host, base::Time session_end) {
  std::string page = Fill(kBlockPage);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{HOST}}",
                                     base::EscapeForHTML(host));
  base::ReplaceSubstringsAfterOffset(
      &page, 0, "{{END}}",
      base::NumberToString(session_end.InMillisecondsSinceUnixEpoch()));
  return page;
}

std::string StartPageHtml(bool as_new_tab) {
  std::string page = Fill(kStartPage);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{TITLE}}",
                                     as_new_tab ? "New Tab" : "Mast");
  return page;
}

}  // namespace mast
