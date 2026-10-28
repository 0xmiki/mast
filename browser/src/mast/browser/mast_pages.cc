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

// resources/start.html, embedded at build time (remote/embed-pages.py writes the
// .inc into the generated-sources dir). Edit the HTML, not this file.
constexpr char kStartPage[] =
#include "mast/browser/resources/start_html.inc"
    ;

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
