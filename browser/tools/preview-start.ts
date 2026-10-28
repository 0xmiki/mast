// Build a local preview of resources/start.html with a fake chrome.send, for design work
// without the builder.   bun tools/preview-start.ts [out.html]
const root = import.meta.dir + '/../src/mast/browser/resources/';
const out = process.argv[2] ?? '/tmp/mast-start-preview.html';

const logo = `<svg width="26" height="26" viewBox="0 0 32 32"><defs><linearGradient id="lg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ff6a3d"/><stop offset="1" stop-color="#c2300f"/></linearGradient></defs><rect width="32" height="32" rx="8" fill="url(#lg)"/><path d="M14 10.5V24.5H6.6Q7.4 16 14 10.5Z" fill="#fff"/><path d="M17 6V24.5H25.4Q24.7 13 17 6Z" fill="#fff"/></svg>`;

// Fake browser side: same messages and state shape as mast_ui.cc.
const mock = `<script>
(() => {
  const presets = ['x.com','twitter.com','facebook.com','instagram.com','tiktok.com','reddit.com',
    'youtube.com','netflix.com','twitch.tv','news.ycombinator.com','amazon.com','chess.com'];
  const s = { active: false, startedAt: 0, endsAt: 0, lastMinutes: 50, presets, custom: ['espn.com'],
    stats: { focusMs: 83 * 60000, blocked: 12, sessions: 3,
      week: ['Fri','Sat','Sun','Mon','Tue','Wed','Thu'].map((label, i) =>
        ({ label, ms: [95, 0, 30, 140, 75, 160, 83][i] * 60000 })) } };
  const reply = () => setTimeout(() => window.mastState(
      JSON.parse(JSON.stringify({ ...s, stats: { ...s.stats, at: Date.now() } }))), 30);
  window.chrome = window.chrome || {};
  chrome.send = (m, a = []) => {
    if (m === 'start' && !s.active) {
      Object.assign(s, { active: true, startedAt: Date.now(), endsAt: Date.now() + a[0] * 60000, lastMinutes: a[0] });
      s.stats.sessions++;
    }
    if (m === 'extend') s.endsAt += a[0] * 60000;
    if (m === 'undo') { s.active = false; s.stats.sessions--; }
    if (m === 'addSite') s.custom.push(a[0]);
    if (m === 'removeSite') s.custom = s.custom.filter((d) => d !== a[0]);
    reply();
  };
})();
</script>`;

let html = await Bun.file(root + 'start.html').text();
html = html.replace('{{TITLE}}', 'New Tab').replace('{{LOGO}}', logo).replace('<script>', mock + '\n<script>');
await Bun.write(out, html);
console.log(out);
