// Build a local preview of resources/start.html (the new tab page) with a fake chrome.send,
// for design work without the builder.   bun tools/preview-start.ts [out.html]
const root = import.meta.dir + '/../src/mast/browser/resources/';
const out = process.argv[2] ?? '/tmp/mast-start-preview.html';

// The shared tokens live in mast_pages.cc; lift them out so the preview matches.
const pages = await Bun.file(import.meta.dir + '/../src/mast/browser/mast_pages.cc').text();
const style = pages.match(/kStyle\[\] = R"MAST\(([\s\S]*?)\)MAST"/)![1];
const list = await Bun.file(import.meta.dir + '/../../blocklist/blocklist.json').json();

// Fake browser side: same messages and state shape as mast_ui.cc.
const mock = `<script>
(() => {
  const now = Date.now(), H = 36e5, D = 864e5;
  const midnight = () => { const d = new Date(); d.setHours(24, 0, 0, 0); return d.getTime(); };
  const cats = ${JSON.stringify(list.categories.map((c: any) => ({ id: c.id, name: c.name, services: c.services.map((s: any) => ({ name: s.name, domains: s.domains })) })))};
  const blocks = {};
  for (const s of cats[0].services) for (const d of s.domains) blocks[d] = { since: now - 12 * D, until: null };
  for (const s of cats[1].services) for (const d of s.domains) blocks[d] = { since: now - 3 * H, until: midnight() };
  blocks['news.ycombinator.com'] = { since: now - 2 * D, until: now + 5 * D };
  const s = { categories: cats, custom: ['espn.com'], blocks, lastDuration: 'today', canUndo: false, stopped: 4 };
  let recent = null;
  const until = (id) => ({ '1h': Date.now() + H, '4h': Date.now() + 4 * H, today: midnight(), week: Date.now() + 7 * D, month: Date.now() + 30 * D, forever: null })[id];
  const outlasts = (a, b) => a === null || (b !== null && a >= b);
  const reply = (result = {}) => setTimeout(() => window.mastState(JSON.parse(JSON.stringify({ ...s, now: Date.now(), result }))), 30);
  window.chrome = window.chrome || {};
  chrome.send = (m, a = []) => {
    if (m === 'block') {
      const u = until(a[1]); const before = {}; let changed = 0;
      for (const d of a[0]) { const cur = s.blocks[d]; if (cur && outlasts(cur.until, u)) continue; before[d] = cur ?? null; s.blocks[d] = { since: cur?.since ?? Date.now(), until: u }; changed++; }
      recent = { at: Date.now(), before }; s.lastDuration = a[1];
      return reply({ changed, until: u });
    }
    if (m === 'undo' && recent) { for (const [d, b] of Object.entries(recent.before)) b ? (s.blocks[d] = b) : delete s.blocks[d]; recent = null; return reply({ undone: true }); }
    if (m === 'addSite' && !s.custom.includes(a[0])) s.custom.push(a[0]);
    if (m === 'removeSite') s.custom = s.custom.filter((d) => d !== a[0]);
    reply();
  };
})();
</script>`;

let html = await Bun.file(root + 'start.html').text();
html = html.replace('{{TITLE}}', 'New Tab').replace('{{STYLE}}', style).replace('<script>', mock + '\n<script>');
await Bun.write(out, html);
console.log(out);
