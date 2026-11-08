// Copyright 2026 The Mast Authors
// YouTube, music only (design C24). Run by Mast (mast_youtube.cc) in its own
// isolated world on youtube.com while a "Music only" block is in force: the
// same place an extension's content script runs, but built in, so there is
// nothing to switch off. Embedded at build time; {{UNTIL}} is the block's end
// (ms since the epoch) or null for forever.
//
// Music, as YouTube itself marks it:
//   - in lists (search, home, the sidebar): the link starts a Mix
//     ("list=RD…"), or the duration badge carries the ♪ icon, or the badge
//     says "Mix". Everything else is hidden, and so is every Shorts shelf;
//   - on a watch page: the page's own data for that video says genre "Music",
//     or it plays from a Mix. Anything else, and every Short, goes to Mast's
//     blocked page (the navigation throttle shows it for these addresses).
(() => {
  if (window.__mastMusicOnly) return;
  window.__mastMusicOnly = true;
  const until = {{UNTIL}};
  const active = () => until === null || Date.now() < until;
  if (!active()) return;

  const ITEMS = [
    'ytd-rich-item-renderer', 'ytd-video-renderer', 'ytd-compact-video-renderer',
    'ytd-grid-video-renderer', 'ytd-playlist-renderer', 'ytd-radio-renderer',
    'ytd-compact-radio-renderer', 'ytd-compact-playlist-renderer', 'ytd-movie-renderer',
    'ytd-reel-item-renderer', 'yt-lockup-view-model', 'ytm-shorts-lockup-view-model',
    'ytm-shorts-lockup-view-model-v2', 'ytd-playlist-video-renderer',
  ].join(',');
  // Undecided items wait invisible (keeping their place, so YouTube still
  // loads their badges); decided non-music ones collapse.
  const CSS = `
    :is(${ITEMS}):not([data-mast]) { visibility: hidden !important; }
    [data-mast="other"], grid-shelf-view-model, ytd-reel-shelf-renderer,
    ytd-rich-shelf-renderer[is-shorts], ytd-rich-section-renderer:has(ytd-rich-shelf-renderer[is-shorts]),
    ytd-guide-entry-renderer:has(a[title="Shorts"]), ytd-mini-guide-entry-renderer[aria-label="Shorts"],
    [data-mast-chip="other"] { display: none !important; }
    html[data-mast-checking] #movie_player, html[data-mast-checking] ytd-watch-flexy #player { visibility: hidden !important; }`;
  const style = document.createElement('style');
  style.textContent = CSS;
  const addStyle = () => {
    if (!style.isConnected && document.documentElement) document.documentElement.appendChild(style);
  };

  // --- lists -------------------------------------------------------------
  function musicItem(el) {
    const link = el.querySelector('a[href*="/watch"], a[href*="list="], a[href^="/shorts"]');
    const href = link ? link.getAttribute('href') || '' : '';
    if (href.startsWith('/shorts')) return 'other';
    if (/[?&]list=RD/.test(href)) return 'music';
    const badges = el.querySelectorAll('badge-shape, ytd-thumbnail-overlay-time-status-renderer');
    for (const b of badges) {
      if (b.textContent.trim() === 'Mix') return 'music';
      if (b.matches('badge-shape.ytBadgeShapeThumbnailBadge, ytd-thumbnail-overlay-time-status-renderer') &&
          /\d:\d\d/.test(b.textContent) && b.querySelector('svg')) return 'music';
    }
    // No ♪ yet: its badge may still be loading. Decide on a later pass.
    const timed = [...badges].some((b) => /\d:\d\d/.test(b.textContent));
    return timed || el.dataset.mastSeen > 2 ? 'other' : null;
  }
  function sweep() {
    if (!active()) return stop();
    for (const el of document.querySelectorAll(ITEMS)) {
      if (el.parentElement && el.parentElement.closest(ITEMS)) continue;  // judged with its outer item
      const verdict = musicItem(el);
      if (!verdict) { el.dataset.mastSeen = (+el.dataset.mastSeen || 0) + 1; continue; }
      for (const e of [el, ...el.querySelectorAll(ITEMS)]) if (e.dataset.mast !== verdict) e.dataset.mast = verdict;
    }
    for (const chip of document.querySelectorAll('chip-view-model, yt-chip-cloud-chip-renderer')) {
      if (chip.textContent.trim() === 'Shorts') chip.dataset.mastChip = 'other';
    }
  }
  // YouTube reuses item elements for new videos: re-judge when a link changes.
  function forget(node) {
    const item = node.closest && node.closest(ITEMS);
    if (item) for (const e of [item, ...item.querySelectorAll(ITEMS)]) { delete e.dataset.mast; delete e.dataset.mastSeen; }
  }

  // --- watch pages -------------------------------------------------------
  let checkedFor = '';
  let pausedHere = false;
  let heldSince = 0;  // when the current video's check began
  const leave = (url) => { stop(); location.replace(url); };
  function judgePage() {
    if (!active()) return stop();
    const url = new URL(location.href);
    if (url.pathname.startsWith('/shorts')) return leave(location.href);
    if (url.pathname !== '/watch') { release(); return; }
    const v = url.searchParams.get('v');
    if (!v || checkedFor === v) return;
    if ((url.searchParams.get('list') || '').startsWith('RD')) { checkedFor = v; release(); return; }
    hold();
    const data = document.querySelector('player-microformat-renderer script');
    let info = null;
    try { info = data && JSON.parse(data.textContent); } catch (e) {}
    const known = info && String(info['@id'] || info.url || '').includes(v);
    // Not this video's data yet; after a few seconds without it, strict.
    if (!known && Date.now() - heldSince < 6000) return;
    checkedFor = v;
    if (known && info.genre === 'Music') return release();
    url.searchParams.set('mast_music_only', '1');
    leave(url.toString());
  }
  // While a video is being checked, its player is hidden and kept silent.
  function hold() {
    if (!('mastChecking' in document.documentElement.dataset)) heldSince = Date.now();
    document.documentElement.dataset.mastChecking = '';
    for (const video of document.querySelectorAll('video')) {
      if (!video.paused) { video.pause(); pausedHere = true; }
    }
  }
  function release() {
    if (!('mastChecking' in document.documentElement.dataset)) return;
    delete document.documentElement.dataset.mastChecking;
    if (pausedHere) { pausedHere = false; const v = document.querySelector('video'); if (v) v.play().catch(() => {}); }
  }

  // --- wiring ------------------------------------------------------------
  let queued = false;
  const pass = () => { queued = false; addStyle(); sweep(); judgePage(); };
  const observer = new MutationObserver((records) => {
    for (const r of records) if (r.type === 'attributes') forget(r.target);
    if (!queued) { queued = true; queueMicrotask(pass); }
  });
  observer.observe(document, { childList: true, subtree: true, attributes: true, attributeFilter: ['href'] });
  const onNav = () => { checkedFor = ''; pass(); };
  document.addEventListener('yt-navigate-start', () => { checkedFor = ''; heldSince = Date.now(); });
  document.addEventListener('yt-navigate-finish', onNav);
  document.addEventListener('yt-page-data-updated', onNav);
  // Late badges and anything the events miss.
  const timer = setInterval(pass, 500);
  const ends = until === null ? null : setTimeout(stop, Math.max(0, until - Date.now()));
  function stop() {
    observer.disconnect();
    clearInterval(timer);
    if (ends) clearTimeout(ends);
    style.remove();
    for (const e of document.querySelectorAll('[data-mast], [data-mast-chip], [data-mast-seen]')) {
      delete e.dataset.mast; delete e.dataset.mastChip; delete e.dataset.mastSeen;
    }
    release();
  }
  pass();
})();
