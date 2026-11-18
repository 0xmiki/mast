// Copyright 2026 The Mast Authors
// YouTube, Music only (design C24) and Shorts (C32). Run by Mast
// (mast_youtube.cc) in its own isolated world on youtube.com while either
// block is in force: the same place an extension's content script runs, but
// built in, so there is nothing to switch off. Embedded at build time;
// {{MUSIC_UNTIL}} and {{SHORTS_UNTIL}} are each block's end (ms since the
// epoch), null for forever, or 0 when it isn't on.
//
// Shorts: every Shorts shelf, item, chip and sidebar entry is hidden, and a
// Short opened anyway goes to Mast's blocked page (the navigation throttle
// shows it for /shorts/ addresses). Nothing else changes.
//
// Music only does that too, and keeps only music, as YouTube itself marks it:
//   - in lists (search, home, the sidebar): the link starts a Mix
//     ("list=RD…"), or the duration badge carries the ♪ icon, or the badge
//     says "Mix". Everything else is hidden;
//   - on a watch page: the page's own data for that video says genre "Music",
//     or it plays from a Mix. Anything else goes to the blocked page.
(() => {
  if (window.__mastYouTube) return;
  window.__mastYouTube = true;
  const MUSIC = {{MUSIC_UNTIL}}, SHORTS = {{SHORTS_UNTIL}};
  const on = (until) => until === null || Date.now() < until;
  const musicOn = () => on(MUSIC);
  const shortsOn = () => musicOn() || on(SHORTS);
  if (!shortsOn()) return;

  const ITEMS = [
    'ytd-rich-item-renderer', 'ytd-video-renderer', 'ytd-compact-video-renderer',
    'ytd-grid-video-renderer', 'ytd-playlist-renderer', 'ytd-radio-renderer',
    'ytd-compact-radio-renderer', 'ytd-compact-playlist-renderer', 'ytd-movie-renderer',
    'ytd-reel-item-renderer', 'yt-lockup-view-model', 'ytm-shorts-lockup-view-model',
    'ytm-shorts-lockup-view-model-v2', 'ytd-playlist-video-renderer',
  ].join(',');
  // Shorts, wherever they show: shelves, single items linking to one, the
  // sidebar entry and the filter chip (marked by sweep()).
  const SHORTS_CSS = `
    ytd-reel-shelf-renderer, ytm-reel-shelf-renderer, ytd-rich-shelf-renderer[is-shorts],
    ytd-rich-section-renderer:has(ytd-rich-shelf-renderer[is-shorts]),
    grid-shelf-view-model:has(a[href^="/shorts"]), ytm-shorts-lockup-view-model, ytm-shorts-lockup-view-model-v2,
    :is(${ITEMS}):has(a[href^="/shorts"]),
    ytd-guide-entry-renderer:has(a[title="Shorts"]), ytd-mini-guide-entry-renderer[aria-label="Shorts"],
    [data-mast-chip="shorts"] { display: none !important; }`;
  // Music only: undecided items wait invisible (keeping their place, so
  // YouTube still loads their badges); decided non-music ones collapse.
  const MUSIC_CSS = `
    :is(${ITEMS}):not([data-mast]) { visibility: hidden !important; }
    [data-mast="other"], grid-shelf-view-model { display: none !important; }
    html[data-mast-checking] #movie_player, html[data-mast-checking] ytd-watch-flexy #player { visibility: hidden !important; }`;
  const style = (css) => { const s = document.createElement('style'); s.textContent = css; return s; };
  const shortsStyle = style(SHORTS_CSS), musicStyle = style(MUSIC_CSS);
  const addStyles = () => {
    if (!document.documentElement) return;
    if (!shortsStyle.isConnected) document.documentElement.appendChild(shortsStyle);
    if (musicOn() && !musicStyle.isConnected) document.documentElement.appendChild(musicStyle);
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
  // Videos shown as music in a list, remembered for this tab's session: opening
  // one is music, whatever its watch page says.
  const KNOWN = 'mastMusicIds';
  const known = new Set(JSON.parse(sessionStorage.getItem(KNOWN) || '[]'));
  function remember(el) {
    for (const a of el.querySelectorAll('a[href*="/watch?v="]')) {
      const v = new URLSearchParams(a.getAttribute('href').split('?')[1] || '').get('v');
      if (v && !known.has(v)) {
        known.add(v);
        try { sessionStorage.setItem(KNOWN, JSON.stringify([...known].slice(-500))); } catch (e) {}
      }
    }
  }
  function sweep() {
    for (const chip of document.querySelectorAll('chip-view-model, yt-chip-cloud-chip-renderer')) {
      if (chip.textContent.trim() === 'Shorts') chip.dataset.mastChip = 'shorts';
    }
    if (!musicOn()) return;
    for (const el of document.querySelectorAll(ITEMS)) {
      if (el.parentElement && el.parentElement.closest(ITEMS)) continue;  // judged with its outer item
      const verdict = musicItem(el);
      if (!verdict) { el.dataset.mastSeen = (+el.dataset.mastSeen || 0) + 1; continue; }
      for (const e of [el, ...el.querySelectorAll(ITEMS)]) if (e.dataset.mast !== verdict) e.dataset.mast = verdict;
      if (verdict === 'music') remember(el);
    }
  }
  // YouTube reuses item elements for new videos: re-judge when a link changes.
  function forget(node) {
    const item = node.closest && node.closest(ITEMS);
    if (item) for (const e of [item, ...item.querySelectorAll(ITEMS)]) { delete e.dataset.mast; delete e.dataset.mastSeen; }
  }
  // Music only ended while Shorts goes on: everything but Shorts comes back.
  function endMusic() {
    musicStyle.remove();
    for (const e of document.querySelectorAll('[data-mast], [data-mast-seen]')) { delete e.dataset.mast; delete e.dataset.mastSeen; }
    release();
  }

  // --- watch pages -------------------------------------------------------
  let checkedFor = '';
  let pausedHere = false;
  let heldSince = 0;  // when the current video's check began
  const leave = (url) => { stop(); location.replace(url); };
  function judgePage() {
    const url = new URL(location.href);
    if (url.pathname.startsWith('/shorts')) return leave(location.href);
    if (!musicOn()) return;
    if (url.pathname !== '/watch') { release(); return; }
    const v = url.searchParams.get('v');
    if (!v || checkedFor === v) return;
    if ((url.searchParams.get('list') || '').startsWith('RD') || known.has(v)) { checkedFor = v; release(); return; }
    hold();
    if (document.querySelector('ytd-video-description-music-section-renderer')) { checkedFor = v; return release(); }
    const genre = genreOf(v);
    // YouTube's cookie consent (in some countries) holds the page back: wait
    // for it, however long. Otherwise, a few seconds without an answer: strict.
    if (genre === null && (consentShowing() || Date.now() - heldSince < 8000)) return;
    checkedFor = v;
    if (genre === 'Music') return release();
    url.searchParams.set('mast_music_only', '1');
    url.searchParams.set('mast_why', genre === null ? 'unknown' : 'genre:' + genre);
    leave(url.toString());
  }
  // The video's genre, from YouTube's own data for exactly this video: the
  // player's microformat (kept current on in-page navigation) or, on a full
  // load, the page's meta tags. null: not known yet.
  function genreOf(v) {
    const data = document.querySelector('player-microformat-renderer script');
    try {
      const info = data && JSON.parse(data.textContent);
      if (info && info.genre && String(info['@id'] || info.url || '').includes(v)) return info.genre;
    } catch (e) {}
    const id = document.querySelector('meta[itemprop="identifier"], meta[itemprop="videoId"]');
    const genre = document.querySelector('meta[itemprop="genre"]');
    if (id && id.content === v && genre) return genre.content;
    return null;
  }
  const consentShowing = () => !!document.querySelector(
      'ytd-consent-bump-v2-lightbox, tp-yt-paper-dialog #consent-bump, form[action*="consent"]');
  // While a video is being checked, its player is hidden and kept silent.
  function hold() {
    if (!document.documentElement) return;
    if (!('mastChecking' in document.documentElement.dataset)) heldSince = Date.now();
    document.documentElement.dataset.mastChecking = '';
    for (const video of document.querySelectorAll('video')) {
      if (!video.paused) { video.pause(); pausedHere = true; }
    }
  }
  function release() {
    if (!document.documentElement || !('mastChecking' in document.documentElement.dataset)) return;
    delete document.documentElement.dataset.mastChecking;
    if (pausedHere) { pausedHere = false; const v = document.querySelector('video'); if (v) v.play().catch(() => {}); }
  }

  // --- wiring ------------------------------------------------------------
  let queued = false;
  let music = musicOn();
  const pass = () => {
    queued = false;
    if (!shortsOn()) return stop();
    if (music && !musicOn()) { music = false; endMusic(); }
    addStyles(); sweep(); judgePage();
  };
  const observer = new MutationObserver((records) => {
    if (musicOn()) for (const r of records) if (r.type === 'attributes') forget(r.target);
    if (!queued) { queued = true; queueMicrotask(pass); }
  });
  observer.observe(document, { childList: true, subtree: true, attributes: true, attributeFilter: ['href'] });
  const onNav = () => { checkedFor = ''; pass(); };
  document.addEventListener('yt-navigate-start', () => { checkedFor = ''; heldSince = Date.now(); });
  document.addEventListener('yt-navigate-finish', onNav);
  document.addEventListener('yt-page-data-updated', onNav);
  // Late badges, anything the events miss, and a mode running out.
  const timer = setInterval(pass, 500);
  function stop() {
    observer.disconnect();
    clearInterval(timer);
    shortsStyle.remove();
    endMusic();
    for (const e of document.querySelectorAll('[data-mast-chip]')) delete e.dataset.mastChip;
  }
  pass();
})();
