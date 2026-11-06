// Copyright 2026 The Mast Authors

#include "mast/browser/mast_pages.h"

#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"
#include "mast/ui/color/mast_palette.h"

namespace mast {

namespace {

// The shared tokens (onyx; PaletteCss() follows them with the palette in use).
// One signal colour, used only for blocking and the state of being blocked.
constexpr char kStyle[] = R"MAST(
:root { color-scheme: dark; --bg:#000000; --surface:#070707; --field:#141414; --pressed:#1e1e1e;
  --seg:#141414; --seg-on:#070707; --line:#171717; --text:#ededed; --text-2:#9a9a9a; --text-3:#5c5c5c;
  --signal:#ff8f63; --signal-text:#3d1000; --r:6px;
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

// The blocklist (categories, sites, how long), shared by chrome://mast and the
// new tab page's panel.
constexpr char kBlocklist[] =
#include "mast/browser/resources/blocklist_html.inc"
    ;

// The Mast mark, as a monochrome outline (like other browsers' new-tab icons),
// for the tab icon of Mast's own pages. Same image as the IDR_NTP_FAVICON
// override, so the tab never switches between two icons.
constexpr char kIcon[] =
    R"(<link rel="icon" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAC/0lEQVR4nMSXS2gTURSGTyYJsdFimlUliAu19YXdq+hW6wMpGrW4CXkhCFV0J2J2ougmkJi3C9006EawuJKI4EKpQheKVSsVmmwkRtwk5uV/6p0STTKZyQzkh5tzbnLvOd99zL0TiQYsiQYs3QDRaHQkFotNUZ/SDWA2m1OSJD1OJpM3qA8ZsQQb+aPZbIYSiYSfNMrQPQCIGCBOaOljJMCyyWTieLPxePyA2k66AZC0yRajvwnzBGUdvptLpVJ71PTXDIDRbcWuP4+EJpHYJEAaDofjNNwcynC9Xn+EttZe8TQB8KiQ6DV2/QOs9an/f3e73b8BxBAltBtHmekVUzUAHrO9jUbjBVwn1xHc0aldMBj8DnNNVEOYhU2kFwCj3Y3kOTk5C3VTa5vWut/vvwezgLIe5Q7pAcB6T2BaX2LEI6jm4b9f7ShJzX8CtdTFxvQJfxpLt78vgEgksgEBnnJyJF5Bkn34usC/Kc0AKxAIvIFJs48Nmcpms2bNABaL5S6Su5C8huSTPp9vmTTIarVepb8bckexWJzUBMCHCToGRPU61nWBNMrj8ZQAn1xNJEnnVANguoaQ/KGozhcKhdvUp5B4li1ATnJcVQClUukCzBaUMjqeCYVCDepTmLl5xPiGAQ0h7vGeAHyyoVwS1ct4rr+QfsmzeZZ6AeDAOQzazXDzoI+TAWpZhmPpdHpYEQC6KBqn5YtGr3gDI94nxLPWarWprgCZTIZHfkRQ3ycDheTPhT3YFaBarfLlwQfKK1B/JWO1KOz2rgDQUf7AdD0jg4WYH4XtDIBHjf0x9jFN78hgIfGiiD0aDodtbQAul2tcvFLxm+5bMlhOp3MJps6+3W7f1QaAC2OnIP3p9XrzZLDwslJH7M/s4+JaWwaL7PCFIayE+z+nEGuCPzpcx1fQb1qhHw9uVNhtnQBknw+KQ9RDaL8irDxbY6Io9Vlz2wDK5fItm832i8QfDSVhtEt4TOfYx8Eyg2v7A0ZlIxUCxI9KpRJrIxmUBv7v+A8AAAD//wUQdZAAAAAGSURBVAMAKHos3DzW9GgAAAAASUVORK5CYII=">)";

std::string Hex(SkColor c) {
  return base::StringPrintf("#%02x%02x%02x", SkColorGetR(c), SkColorGetG(c),
                            SkColorGetB(c));
}

std::string Fill(std::string page) {
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{STYLE}}", 
                                     std::string(kStyle) + PaletteCss());
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{ICON}}", kIcon);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{BLOCKLIST}}", kBlocklist);
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

std::string PaletteCss() {
  const palette::Palette& p = palette::Current();
  return base::StringPrintf(
      ":root{--bg:%s;--surface:%s;--field:%s;--pressed:%s;--seg:%s;"
      "--seg-on:%s;--line:%s;--text:%s;--text-2:%s;--text-3:%s}",
      Hex(p.bg).c_str(), Hex(p.chrome).c_str(), Hex(p.chrome2).c_str(),
      Hex(p.raised2).c_str(), Hex(p.chrome2).c_str(), Hex(p.chrome).c_str(),
      Hex(p.line).c_str(), Hex(p.text).c_str(), Hex(p.muted).c_str(),
      Hex(p.faint).c_str());
}

std::string DesktopPageHtml(int wallpaper, int wallpaper_top) {
  std::string page = Fill(kDesktopPage);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALLPAPER}}",
                                     base::NumberToString(wallpaper));
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALLPAPER_TOP}}",
                                     base::NumberToString(wallpaper_top));
  return page;
}

}  // namespace mast
