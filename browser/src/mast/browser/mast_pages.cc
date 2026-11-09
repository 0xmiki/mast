// Copyright 2026 The Mast Authors

#include "mast/browser/mast_pages.h"

#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"
#include "mast/browser/mast_page_backdrop.h"
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
html { scrollbar-width:thin; scrollbar-color:rgb(255 255 255/.18) transparent }
html:has(body.wall.light) { scrollbar-color:rgb(0 0 0/.28) transparent }
html,body { margin:0; min-height:100%; background:var(--surface); color:var(--text);
  font:13px/1.45 Inter,"Inter Variable","Noto Sans","DejaVu Sans",system-ui,sans-serif;
  font-feature-settings:'tnum' 1; -webkit-font-smoothing:antialiased }
button, input { font:inherit; color:inherit }
button { cursor:pointer }
:focus-visible { outline:2px solid var(--text-2); outline-offset:2px }
.primary, .secondary { display:inline-flex; align-items:center; justify-content:center; gap:8px;
  height:36px; padding:0 20px; border:0; border-radius:999px; font-weight:600 }
/* White (black on a light picture): ember is only for blocked states (the
   dot, "blocked until", the blocked page), so it keeps meaning blocked. */
.primary { background:var(--text); color:var(--bg) }
.primary:hover:not(:disabled) { filter:brightness(.9) }
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
/* The blocked page (design C21): the site, when it opens again, and a way
   back. On Customize's picture it shows the picture (laid out like the new
   tab page's, so it runs on from the tab row) with a soft glow behind the
   text. */
body { display:grid; place-content:center; min-height:100vh; padding:40px 32px 10vh; text-align:center }
.card { display:flex; flex-direction:column; align-items:center; gap:18px; max-width:min(720px,100%) }
h1 { margin:0; font-size:56px; font-weight:600; letter-spacing:-.025em; line-height:1.1; overflow-wrap:anywhere }
.st { display:flex; flex-direction:column; align-items:center; gap:8px }
.st .line { display:flex; align-items:center; gap:10px; font-size:16px; font-weight:600 }
.st .dot { width:8px; height:8px; flex:none; border-radius:50%; background:var(--signal) }
.st .left { color:var(--text-2); font-size:14px; font-variant-numeric:tabular-nums }
.back { margin-top:12px; padding:4px 6px; border:0; background:none; color:var(--text-2); font-size:13px; font-weight:500 }
.back:hover { color:var(--text) }
#wall { position:fixed; inset:0; z-index:-1; pointer-events:none; background-repeat:no-repeat;
  background-image: radial-gradient(62% 75% at 50% 47%, rgb(0 0 0/.65), rgb(0 0 0/.35) 45%, transparent),
    linear-gradient(rgb(0 0 0/.22), rgb(0 0 0/.22)), var(--wall);
  background-size: 100% 100%, 100% 100%, var(--wall-size, cover);
  background-position: 0 0, 0 0, var(--wall-pos, center) }
body.wall { background:transparent; color:#f2f2f2; --text:#f2f2f2; --text-2:rgb(242 242 242/.75) }
body.wall h1, body.wall .st, body.wall .back { text-shadow:0 1px 6px rgb(0 0 0/.5) }
body.wall.light { color:#141414; --text:#141414; --text-2:#3d3d3d }
body.wall.light #wall { background-image: radial-gradient(62% 75% at 50% 47%, rgb(255 255 255/.6), rgb(255 255 255/.3) 45%, transparent), var(--wall);
  background-size: 100% 100%, var(--wall-size, cover); background-position: 0 0, var(--wall-pos, center) }
body.wall.light h1, body.wall.light .st, body.wall.light .back { text-shadow:0 1px 4px rgb(255 255 255/.55) }
</style></head><body class="{{BODY_CLASS}}"><div id="wall" aria-hidden="true"></div><main class="card">
<h1>{{HOST}}</h1>
<div class="st" role="status"><div class="line"><i class="dot"></i><span id="state"></span></div><div class="left" id="left"></div></div>
<button class="back" id="back" onclick="history.back()">&larr; Back</button>
</main><script>
const until = {{UNTIL}};
const quiet = {{QUIET}};  // always blocked (adult, gambling): no end time
const label = {{LABEL}};  // "Blocked", or "Music only" (YouTube)
const wall = {{WALL}}, wallTop = {{WALL_TOP}};
const D = 864e5;
const hm = (t) => new Date(t).toLocaleTimeString([], {hour: 'numeric', minute: '2-digit'});
const day = (t) => new Date(t).toLocaleDateString([], {month: 'short', day: 'numeric'});
const midnight = () => { const d = new Date(); d.setHours(24, 0, 0, 0); return d.getTime(); };
function untilText(u) {
  if (u === midnight()) return 'until midnight';
  if (new Date(u).toDateString() === new Date().toDateString()) return 'until ' + hm(u);
  if (u - Date.now() < 2 * D) return 'until tomorrow ' + hm(u);
  return 'until ' + day(u);
}
// "59:12 left" or "5:47:12 left" under a day, "6 days, 23 h left" beyond.
function leftText(u) {
  const ms = Math.max(0, u - Date.now());
  const days = Math.floor(ms / D), h = Math.floor((ms % D) / 36e5);
  if (days) return days + (days === 1 ? ' day, ' : ' days, ') + h + ' h left';
  const m = Math.floor((ms % 36e5) / 6e4), s = Math.floor((ms % 6e4) / 1e3);
  const mm = String(m).padStart(h ? 2 : 1, '0'), ss = String(s).padStart(2, '0');
  return (h ? h + ':' + mm : mm) + ':' + ss + ' left';
}
function paint() {
  const forever = until === null;
  document.getElementById('state').textContent = quiet ? 'This site is blocked'
      : forever ? label + ' forever' : label + ' ' + untilText(until);
  document.getElementById('left').textContent = forever ? '' : leftText(until);
  if (!forever && Date.now() >= until) location.reload();
}
if (wall) {
  // As on the new tab page: the picture covers the screen as seen from its
  // top-left, which is wallTop above this page.
  const img = new Image();
  img.onload = () => {
    const sw = screen.width, sh = screen.height + wallTop;
    const k = Math.max(sw / img.naturalWidth, sh / img.naturalHeight);
    const w = img.naturalWidth * k, h = img.naturalHeight * k, s = document.body.style;
    s.setProperty('--wall', 'url(' + wall + ')');
    s.setProperty('--wall-size', w + 'px ' + h + 'px');
    s.setProperty('--wall-pos', (sw - w) / 2 + 'px ' + ((sh - h) / 2 - wallTop) + 'px');
  };
  img.src = wall;
}
if (history.length < 2) document.getElementById('back').style.display = 'none';
paint(); setInterval(paint, 1000);
</script></body></html>)MAST";

// resources/start.html, embedded at build time (remote/embed-pages.py writes the
// .inc into the generated-sources dir). Edit the HTML, not this file.
constexpr char kStartPage[] =
#include "mast/browser/resources/start_html.inc"
    ;

constexpr char kDesktopPage[] =
#include "mast/browser/resources/desktop_html.inc"
    ;

constexpr char kWelcomePage[] =
#include "mast/browser/resources/welcome_html.inc"
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

namespace {

// `until`: ms since the epoch, or "null" for forever.
std::string BlockPage(std::string_view host,
                      std::string_view until,
                      bool quiet,
                      std::string_view label,
                      const PageBackdrop& backdrop) {
  std::string page = Fill(kBlockPage);
  const bool wall = !backdrop.data_url.empty();
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{HOST}}",
                                     base::EscapeForHTML(host));
  base::ReplaceSubstringsAfterOffset(
      &page, 0, "{{BODY_CLASS}}",
      wall ? (backdrop.light ? "wall light" : "wall") : "");
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALL_TOP}}",
                                     base::NumberToString(backdrop.top));
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{UNTIL}}", until);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{QUIET}}",
                                     quiet ? "true" : "false");
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{LABEL}}",
                                     "\"" + std::string(label) + "\"");
  // Last: the picture is the one large value (a base64 data: URL; no quotes
  // or braces in it).
  base::ReplaceSubstringsAfterOffset(
      &page, 0, "{{WALL}}", wall ? "\"" + backdrop.data_url + "\"" : "null");
  return page;
}

}  // namespace

std::string BlockPageHtml(std::string_view host,
                          const BlockMatch& match,
                          const PageBackdrop& backdrop,
                          std::string_view label) {
  return BlockPage(
      host,
      match.block.until
          ? base::NumberToString(match.block.until->InMillisecondsSinceUnixEpoch())
          : std::string("null"),
      /*quiet=*/false, label, backdrop);
}

std::string QuietBlockPageHtml(std::string_view host,
                               const PageBackdrop& backdrop) {
  return BlockPage(host, "null", /*quiet=*/true, "Blocked", backdrop);
}

std::string WelcomePageHtml(int wallpaper, std::string_view credit) {
  std::string page = Fill(kWelcomePage);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALLPAPER}}",
                                     base::NumberToString(wallpaper));
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{CREDIT}}",
                                     base::EscapeForHTML(credit));
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

std::string DesktopPageHtml(int wallpaper, int wallpaper_top, bool light_image) {
  std::string page = Fill(kDesktopPage);
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALLPAPER}}",
                                     base::NumberToString(wallpaper));
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALLPAPER_TOP}}",
                                     base::NumberToString(wallpaper_top));
  base::ReplaceSubstringsAfterOffset(&page, 0, "{{WALLPAPER_TONE}}",
                                     light_image ? "\"light\"" : "\"dark\"");
  return page;
}

}  // namespace mast
