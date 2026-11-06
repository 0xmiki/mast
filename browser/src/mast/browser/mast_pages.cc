// Copyright 2026 The Mast Authors

#include "mast/browser/mast_pages.h"

#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"

namespace mast {

namespace {

// The shared tokens (mirrors mast_palette.h and the Rams proposal). One signal
// colour, used only for blocking and the state of being blocked.
constexpr char kStyle[] = R"MAST(
:root { color-scheme: dark; --bg:#141414; --surface:#1c1c1c; --field:#262626; --pressed:#303030;
  --seg:#262626; --seg-on:#1c1c1c; --line:#2c2c2c; --text:#e9e7e3; --text-2:#a29f99; --text-3:#6f6c67;
  --signal:#ff6a3d; --signal-text:#1a0d07; --r:6px;
  --shadow:0 8px 28px rgb(0 0 0/.45),0 0 0 1px var(--line); }
* { box-sizing:border-box }
html,body { margin:0; min-height:100%; background:var(--surface); color:var(--text);
  font:13px/1.45 Inter,"Inter Variable","Noto Sans","DejaVu Sans",system-ui,sans-serif;
  font-feature-settings:'tnum' 1; -webkit-font-smoothing:antialiased }
button, input { font:inherit; color:inherit }
button { cursor:pointer }
:focus-visible { outline:2px solid var(--text-2); outline-offset:2px }
.primary, .secondary { display:inline-flex; align-items:center; justify-content:center; gap:8px;
  height:36px; padding:0 18px; border:0; border-radius:var(--r); font-weight:600 }
.primary { background:var(--signal); color:var(--signal-text) }
.primary:hover:not(:disabled) { filter:brightness(1.06) }
.primary:active:not(:disabled) { filter:brightness(.94) }
.primary:disabled { background:var(--field); color:var(--text-3); cursor:default }
.secondary { background:var(--field) }
.secondary:hover { background:var(--pressed) }
.ind { display:inline-block; width:8px; height:8px; border-radius:50%; flex:none; box-shadow:inset 0 0 0 1.5px var(--text-3) }
.ind.timed { box-shadow:inset 0 0 0 1.5px var(--signal) }
.ind.forever { background:var(--signal); box-shadow:none }
.status { display:flex; align-items:center; gap:8px; color:var(--text-3); white-space:nowrap }
.status.blocked { color:var(--signal) }
)MAST";

constexpr char kBlockPage[] = R"MAST(<!doctype html>
<html><head><meta charset="utf-8"><title>{{HOST}} is blocked</title>{{ICON}}<style>{{STYLE}}
body { display:grid; place-content:center; min-height:100vh; padding:40px 32px 12vh }
.card { width:min(520px,100%) }
.status { font-weight:500 }
h1 { margin:10px 0 6px; font-size:40px; font-weight:600; letter-spacing:-.025em; line-height:1.1; overflow-wrap:anywhere }
dl { display:grid; grid-template-columns:auto 1fr; gap:6px 24px; margin:24px 0 28px; padding:16px 0;
  border-top:1px solid var(--line); border-bottom:1px solid var(--line) }
dt { color:var(--text-3) } dd { margin:0 } dd.signal { color:var(--signal) }
</style></head><body><main class="card">
<span class="status blocked"><i class="ind" id="ind"></i><span id="state"></span></span>
<h1>{{HOST}}</h1>
<dl><dt>List</dt><dd>{{LIST}}</dd><dt>Since</dt><dd id="since"></dd><dt>Ends</dt><dd id="ends"></dd></dl>
<button class="secondary" id="back" onclick="history.back()"><svg width="14" height="14" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M13 8H3M7 4L3 8l4 4"/></svg> Back</button>
</main><script>
const since = {{SINCE}}, until = {{UNTIL}};
const D = 864e5;
const hm = (t) => new Date(t).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'});
const day = (t) => new Date(t).toLocaleDateString([], {month: 'short', day: 'numeric'});
const midnight = () => { const d = new Date(); d.setHours(24, 0, 0, 0); return d.getTime(); };
function untilText(u) {
  if (u === midnight()) return 'until midnight';
  if (new Date(u).toDateString() === new Date().toDateString()) return 'until ' + hm(u);
  if (u - Date.now() < 2 * D) return 'until tomorrow ' + hm(u);
  return 'until ' + day(u);
}
function left(u) {
  const ms = u - Date.now();
  if (ms >= 2 * D) return Math.floor(ms / D) + ' days left';
  const h = Math.floor(ms / 36e5), m = Math.max(1, Math.ceil((ms % 36e5) / 6e4));
  return (h ? h + ' h ' : '') + m + ' min left';
}
function paint() {
  const forever = until === null;
  document.getElementById('ind').className = 'ind ' + (forever ? 'forever' : 'timed');
  document.getElementById('state').textContent = forever ? 'Blocked forever' : 'Blocked ' + untilText(until);
  document.getElementById('since').textContent = day(since) + ', ' + hm(since);
  const ends = document.getElementById('ends');
  ends.textContent = forever ? 'Never' : untilText(until).replace(/^until /, '').replace(/^./, (c) => c.toUpperCase()) + ' · ' + left(until);
  ends.className = forever ? 'signal' : '';
  if (!forever && Date.now() >= until) location.reload();
}
if (history.length < 2) document.getElementById('back').style.display = 'none';
paint(); setInterval(paint, 15000);
</script></body></html>)MAST";

// resources/start.html, embedded at build time (remote/embed-pages.py writes the
// .inc into the generated-sources dir). Edit the HTML, not this file.
constexpr char kStartPage[] =
#include "mast/browser/resources/start_html.inc"
    ;

constexpr char kDesktopPage[] =
#include "mast/browser/resources/desktop_html.inc"
    ;

// The Mast mark as the tab icon of Mast's own pages (32px, from the product
// favicon). Chromium shows the default globe for them otherwise.
constexpr char kIcon[] =
    R"(<link rel="icon" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAABmJLR0QA/wD/AP+gvaeTAAADkElEQVRYhb2XTWhcVRiGn3Puz0ySSWKGWA3iJEIwNEMq2h9TyaZpQ00FsYWuXNW2KV0EtHZRXOhKEIpURaFIi1shSJdCre67EFwk7cJNBSm4aE2bxDSZued1MTPJzczcmYljc1bf5ZzzPe/3d+AaqtbazMEx43TGYQ5LbkiiSxJISKJklm1idvXe5veKpHtO5pbDXcvduDsf55mKodnpVGH10RUZnZNk4w7bgFefjSSuLoXehfzcwvqGAM1OpwpP/v5RmEPVDv5H+MYe0i/L6WA6P7ewbgEKTxa/2EE4kia7/ln7HMCszRwcQ/z2FNNeDa+ci6KIV6xxOtMOPBgbJ9w/uV04kjxr3GnrDEfaiTx8fYreD6+QnnhrO3Ak4QxTVk65dtJemiVD7+ynpMaPtAwXIKdBK5Fpt+YAWI++9y/jD460Bi/Z3fY/wY1B1tuEVx4Vz6P/428xHZlW4Ehgtws3PVmyl38g8+4HNQKwHjbTyzPvXWoJLgm7LXh2F32ffY//4jAm3blxdksWgpDM4ROk8geawssCWoR39dD3yXfY3v4NWE0GYis781FTeCwDTWqOoefil9j+5zG+vwGv3KtZ1hIMjtBx4FBDuERZQJNu7zx+luDlPRg/2MIpNXD9DBhr6Tt5rukE2WZw+1yOzMnztXAojVqiAkN6dC/+QC4RvrUECSq7T11CcrWAioLkNkCFdbon32lQWuICauHeCy+RenWiJvqKgsQeqCQhTNE98WYivP4Yxr673j6FomJCeE1KUF6poRG83mziqNskOJ5P+o2jCdETc9KQj4oF0rtfSxz1xB4I8vshCBu5bvgObB4T6eF84qjX7wEg3L0XXNTIb0sCTJgiPbIncdQTn+Iwvw8TpBoHR2siwoFc/TLX9EBMpT8w2NRxSyUAguyuuvBKBpbq1cekOxo6jVZXkIRbXWkqwPh+XbjEY3P/+OiCDKM1zZHpwXR2116ilPrCX3/iihFYD+/ZgfJ+OQi0+U5JFJcWKT5arIYjMe87zE/IjcbhSESPF1H5Uv0RKtvFAtH9PxLnPCHy0rfjpnW4a5KiOLzu41QPnjBBrcAlRfLcdZu7cXde4uoOwxF8M3774R0LsBR6F5B+3kH4rcA9uAhgAfJzC+vL6eAY0teb5XhKaYevAvfg2L5fKUDs77iyfj86nLfGnXaGKTkNScq0A3diGXEPp5vy3PXx2w/vxHn/Ag7Iq6bq6GeTAAAAAElFTkSuQmCC">)";

std::string Fill(std::string page) {
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{STYLE}}", kStyle);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{ICON}}", kIcon);
  return page;
}

}  // namespace

std::string BlockPageHtml(std::string_view host, const BlockMatch& match) {
  std::string page = Fill(kBlockPage);
  std::string list = CategoryNameOf(match.domain);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{HOST}}",
                                     base::EscapeForHTML(host));
  base::ReplaceSubstringsAfterOffset(
      &page, 0, "{{LIST}}", base::EscapeForHTML(list.empty() ? "Your sites" : list));
  base::ReplaceSubstringsAfterOffset(
      &page, 0, "{{SINCE}}",
      base::NumberToString(match.block.since.InMillisecondsSinceUnixEpoch()));
  base::ReplaceSubstringsAfterOffset(
      &page, 0, "{{UNTIL}}",
      match.block.until
          ? base::NumberToString(match.block.until->InMillisecondsSinceUnixEpoch())
          : std::string("null"));
  return page;
}

std::string StartPageHtml(bool as_new_tab) {
  std::string page = Fill(kStartPage);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{TITLE}}",
                                     as_new_tab ? "New Tab" : "Mast");
  return page;
}

std::string DesktopPageHtml() {
  return Fill(kDesktopPage);
}

}  // namespace mast
