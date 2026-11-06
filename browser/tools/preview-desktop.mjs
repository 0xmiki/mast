#!/usr/bin/env node
// Local preview of resources/desktop.html (the new tab desktop) with a fake chrome.send,
// for design work without the builder.   node tools/preview-desktop.mjs [out.html]
// chrome://favicon2 only exists inside the browser, so icons become letters here.
import fs from 'node:fs';
import path from 'node:path';

const here = path.dirname(new URL(import.meta.url).pathname);
const res = path.join(here, '../src/mast/browser/resources/');
const out = process.argv[2] ?? '/tmp/mast-desktop-preview.html';
const pages = fs.readFileSync(path.join(here, '../src/mast/browser/mast_pages.cc'), 'utf8');
const style = pages.match(/kStyle\[\] = R"MAST\(([\s\S]*?)\)MAST"/)[1];
// The palettes, read from mast_palette.h like the browser builds them.
const paletteH = fs.readFileSync(path.join(here, '../src/mast/ui/color/mast_palette.h'), 'utf8');
const hex = (h) => '#' + h.slice(2).toLowerCase();
const palettes = [...paletteH.matchAll(/\{"(\w+)", "(\w+)",((?:\s*Rgb\(0x[0-9A-Fa-f]{6}\),?){11})\}/g)].map(([, id, name, rgbs]) => {
  const [bg, chrome, chrome2, hover, raised, raised2, border, text, muted, faint, line] = [...rgbs.matchAll(/0x[0-9A-Fa-f]{6}/g)].map((m) => hex(m[0]));
  return { id, name, surface: chrome, field: chrome2, text,
    css: `:root{--bg:${bg};--surface:${chrome};--field:${chrome2};--pressed:${raised2};--seg:${chrome2};--seg-on:${chrome};--line:${line};--text:${text};--text-2:${muted};--text-3:${faint}}` };
});
if (palettes.length < 5) throw new Error('palettes not found in mast_palette.h');
const list = JSON.parse(fs.readFileSync(path.join(here, '../../blocklist/blocklist.json'), 'utf8'));

const sites = [
  ['GitHub', 'https://github.com/'], ['Linear', 'https://linear.app/'], ['Figma', 'https://www.figma.com/'],
  ['Notion', 'https://www.notion.so/'], ['Hetzner', 'https://console.hetzner.com/'], ['Svelte', 'https://svelte.dev/docs'],
  ['MDN', 'https://developer.mozilla.org/'], ['Rust Book', 'https://doc.rust-lang.org/book/'],
  ['Hugging Face', 'https://huggingface.co/'], ['Paul Graham', 'https://paulgraham.com/articles.html'],
  ['Vitsœ', 'https://www.vitsoe.com/'], ['Inter', 'https://rsms.me/inter/'], ['Chromium', 'https://source.chromium.org/'],
  ['Wikipedia', 'https://en.wikipedia.org/'],
].map(([title, url], i) => ({ id: String(100 + i), title, url }));

const mock = `<script>
(() => {
  const H = 36e5, D = 864e5;
  const midnight = () => { const d = new Date(); d.setHours(24, 0, 0, 0); return d.getTime(); };
  const cats = ${JSON.stringify(list.categories.map((c) => ({ id: c.id, name: c.name, services: c.services.map((s) => ({ name: s.name, domains: s.domains })) })))};
  const s = { categories: cats, custom: [], blocks: {}, lastDuration: 'today', canUndo: false, stopped: 0,
    distractions: ['social','video','streaming','news','forums','entertainment','sports','gambling','adult'],
    desktop: ${JSON.stringify(sites)}, desktopReady: true, widgets: ['clock', 'distractions'] };
  const palettes = ${JSON.stringify(palettes)};
  s.palettes = palettes.map(({ css, ...p }) => p);
  s.theme = { palette: 'onyx', css: palettes[0].css, wallpaper: 0 };
  window.__wallUrl = '';
  if (location.hash === '#blocked') for (const c of cats) if (s.distractions.includes(c.id)) for (const v of c.services) for (const d of v.domains) s.blocks[d] = { since: Date.now() - H, until: midnight() };
  if (location.hash === '#empty') s.desktop = [];
  let recent = null, next = 500;
  const until = (id) => ({ '1h': Date.now() + H, '4h': Date.now() + 4 * H, today: midnight(), week: Date.now() + 7 * D, month: Date.now() + 30 * D, forever: null })[id];
  const outlasts = (a, b) => a === null || (b !== null && a >= b);
  const reply = (result = {}) => setTimeout(() => window.mastState(JSON.parse(JSON.stringify({ ...s, now: Date.now(), result }))), 30);
  const domains = () => cats.filter((c) => s.distractions.includes(c.id)).flatMap((c) => c.services.flatMap((v) => v.domains)).concat(s.custom);
  window.chrome = window.chrome || {};
  chrome.send = (m, a = []) => {
    if (m === 'blockDistractions') {
      const u = until(a[0]); const before = {}; let changed = 0;
      for (const d of domains()) { const cur = s.blocks[d]; if (cur && outlasts(cur.until, u)) continue; before[d] = cur ?? null; s.blocks[d] = { since: cur?.since ?? Date.now(), until: u }; changed++; }
      recent = { before }; s.lastDuration = a[0];
      return reply({ changed, until: u });
    }
    if (m === 'block') {
      const u = until(a[1]); const before = {}; let changed = 0;
      for (const d of a[0]) { const cur = s.blocks[d]; if (cur && outlasts(cur.until, u)) continue; before[d] = cur ?? null; s.blocks[d] = { since: cur?.since ?? Date.now(), until: u }; changed++; }
      recent = { before }; s.lastDuration = a[1];
      return reply({ changed, until: u });
    }
    if (m === 'setPalette') { const p = palettes.find((x) => x.id === a[0]); if (p) s.theme = { ...s.theme, palette: p.id, css: p.css }; }
    // The browser writes the image to the profile folder; here it stays a data URL.
    if (m === 'setWallpaper') { window.__wallUrl = 'data:image/jpeg;base64,' + a[0]; s.theme = { ...s.theme, wallpaper: Math.abs(s.theme.wallpaper) + 1 }; }
    if (m === 'clearWallpaper') s.theme = { ...s.theme, wallpaper: -Math.abs(s.theme.wallpaper) };
    if (m === 'addSite' && !s.custom.includes(a[0])) s.custom.push(a[0]);
    if (m === 'removeSite') s.custom = s.custom.filter((d) => d !== a[0]);
    if (m === 'undo' && recent) { for (const [d, b] of Object.entries(recent.before)) b ? (s.blocks[d] = b) : delete s.blocks[d]; recent = null; return reply({ undone: true }); }
    if (m === 'setDistraction') { s.distractions = s.distractions.filter((x) => x !== a[0]); if (a[1]) s.distractions.push(a[0]); }
    if (m === 'desktopAdd') { let u = a[0]; if (!u.includes('://')) u = 'https://' + u; try { new URL(u); } catch { return reply({ invalidSite: true }); }
      s.desktop.push({ id: String(next++), title: a[1] || new URL(u).hostname.replace(/^www\\./, ''), url: u }); }
    if (m === 'desktopRemove') s.desktop = s.desktop.filter((x) => x.id !== a[0]);
    if (m === 'desktopMove') { const i = s.desktop.findIndex((x) => x.id === a[0]); const [n] = s.desktop.splice(i, 1); s.desktop.splice(a[1], 0, n); }
    reply();
  };
})();
</script>`;

let html = fs.readFileSync(path.join(res, 'desktop.html'), 'utf8');
const letterIcon = `const icon = (url) => { const h = new URL(url).hostname.replace(/^www\\./, ''); return 'data:image/svg+xml,' + encodeURIComponent('<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"><rect width="32" height="32" rx="7" fill="#3b3a38"/><text x="16" y="22" font-family="Inter,sans-serif" font-size="17" font-weight="600" text-anchor="middle" fill="#e9e7e3">' + h[0].toUpperCase() + '</text></svg>'); };`;
if (!/const icon = \(url\) => [^\n]*;/.test(html)) throw new Error('icon() not found');
const blocklist = fs.readFileSync(path.join(res, 'blocklist.html'), 'utf8');
html = html.replace('{{STYLE}}', style).replace('{{ICON}}', '').replace('{{BLOCKLIST}}', () => blocklist).replace(/const icon = \(url\) => [^\n]*;/, letterIcon).replace('<script>', mock + '\n<script>')
  // The page serves its image as wallpaper-<version>.jpg; here it's a data URL.
  .replace('{{WALLPAPER}}', '0').replace('{{WALLPAPER_TOP}}', '57').replaceAll(/wallpaper-\$\{\w+\}\.jpg/g, () => '${window.__wallUrl}');
if (html.includes('wallpaper-$')) throw new Error('a wallpaper URL was not mocked');
// #bl opens the blocklist panel, #cz the Customize panel, for screenshots.
html = html.replace('</body>', '<script>if (location.hash === "#cz") setTimeout(() => document.getElementById("customize").click(), 200);</script></body>');
html = html.replace('</body>', '<script>if (location.hash === "#bl") setTimeout(() => document.getElementById("editList").click(), 200);</script></body>');
fs.writeFileSync(out, html);
console.log(out);
