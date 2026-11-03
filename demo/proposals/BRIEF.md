# Design proposals: shared brief (round 2)

Each proposal is one self-contained `index.html` in its own folder (`proposals/<nn>-<slug>/index.html`).

Round 1 (01–05) leaned on ship and sea metaphors. That made the work feel themed and gimmicky
without improving the UI or UX. Round 2 is about **design languages**: a coherent system of
shape, type, colour, density, spacing, states and motion that someone could write down as tokens
and components, and that holds up everywhere in the product. Proposal 03 (Console) is the
favourite so far, because it is an actual design language rather than a theme. Match that bar
with a *different* language.

## Hard rules

- **No ship, sea, sail, mast, rope, horizon, chart or nautical imagery, wording or metaphors.**
  - The logo itself is the only exception.
  - Copy is plain product language: "Blocked until midnight", "Blocked forever", "Block".
- **Radical in language, sane in structure.**
  - The look should make people ask "what browser is this?"
  - The skeleton must stay a normal browser: tabs at the top, a toolbar with the address, page
    content below.
  - It must adapt to macOS and Windows without a redesign.
- **No themed decoration.**
  - Every visual choice should be part of the system and reusable for any component, not an
    illustration.

## Cross-platform adaptability (required)

Include a platform switch (Linux / macOS / Windows) in the caption. It must re-lay out the window
chrome in place:

| Platform | Title bar and window controls |
|---|---|
| macOS | Traffic lights at the top left, tabs starting after them |
| Windows | Minimise / maximise / close at the top right, Windows-sized hit targets; the tab strip leaves room for the system's snap affordances |
| Linux | GNOME-style controls on the right |

Your language must look right in all three. Don't fight native conventions: keyboard shortcuts,
menu placement and scrollbar behaviour stay conventional.

## The product (must be shown)

Mast is a Chromium-based browser whose one job is blocking distractions. It's built into the
browser so it can't be switched off like an extension.

- **A categorised blocklist, already filled in.** Show it on the new tab page:
  - **Social:** x.com, twitter.com, facebook.com, instagram.com, tiktok.com, reddit.com,
    snapchat.com, threads.net, bsky.app, pinterest.com, tumblr.com, discord.com
  - **Video & streaming:** youtube.com, netflix.com, twitch.tv, kick.com, primevideo.com,
    disneyplus.com, hulu.com, max.com
  - **News & forums:** news.ycombinator.com, cnn.com, foxnews.com, bbc.com, nytimes.com,
    buzzfeed.com, 9gag.com
  - **Shopping:** amazon.com, ebay.com, aliexpress.com, temu.com, etsy.com, shein.com
  - **Games:** chess.com, lichess.org, roblox.com, store.steampowered.com, poki.com
  - **Your sites:** the user's own additions, e.g. espn.com
- **Pick, then block.** Pick categories or single sites, choose how long (1 hour, 4 hours,
  until midnight, 1 week, 1 month, **forever**), then press Block.
- **Forever is truly forever.**
  - Blocks can be lengthened, never shortened or lifted.
  - The only way back is a 10-second Undo right after pressing Block.
  - No confirm dialogs, no hold-to-confirm, no friction.
- **Toolbar "Block" action** blocks the site you're currently on.
- **Blocked page:** shown when you open a blocked site. It says the site and until when (or
  "forever"), and has a Back button.
- **Sample state to show:**
  - Social blocked forever (since Oct 18)
  - Video & streaming blocked until midnight
  - news.ycombinator.com blocked for 5 more days
  - the rest open
  - 4 distractions stopped today, 21 sites blocked
- **No:** focus timer, sessions, countdown hero, quotes or slogans, dashboards, app/shortcut
  tiles.
- **Search:** Google is the default search engine.

## Brand

- **Logo:** use as-is (32×32 viewBox). You may render it monochrome if your language demands
  it, but keep the shape.

```html
<svg viewBox="0 0 32 32"><defs><linearGradient id="g" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ff6a3d"/><stop offset="1" stop-color="#c2300f"/></linearGradient></defs>
<rect width="32" height="32" rx="8" fill="url(#g)"/>
<path d="M14 10.5V24.5H6.6Q7.4 16 14 10.5Z" fill="#fff" stroke="#fff" stroke-width="1.2" stroke-linejoin="round"/>
<path d="M17 6V24.5H25.4Q24.7 13 17 6Z" fill="#fff" stroke="#fff" stroke-width="1.2" stroke-linejoin="round"/></svg>
```

- **Accent:** ember `#ff6a3d` is the brand accent. Your language decides how it is used.
- **Theme:** dark-first for v1.

## What the file must contain

1. **A full browser window mock**, filling the viewport (designed for 1440×900, must still work
   at 1280×800):
   - 4–6 tabs (New Tab, GitHub, Wikipedia, YouTube [blocked], X [blocked])
   - back/forward/reload, address, the Block action, menu
2. **Views, switched by clicking the tabs:**
   - **New tab page:** the categorised blocklist with status, selection, a duration choice
     including forever, and a Block button.
   - **An ordinary site:** a quick fake GitHub or Wikipedia page.
   - **The blocked page:** YouTube (until midnight) and X (forever) should differ visibly.
3. **A "System" sheet**, opened from the caption: the language written down.
   - colour tokens
   - type scale
   - spacing/radius/stroke rules
   - core components in their states (button default/hover/pressed/disabled, input, toggle or
     checkbox, tab active/inactive/blocked, toast, menu)
   - 3–5 one-line principles
4. **Light interactivity in vanilla JS:**
   - select categories
   - choose a duration
   - Block updates the status and shows a toast with Undo, and Undo really reverts
   - the toolbar Block popover
   - the menu
   - the platform switch
5. **Caption:** a small collapsible caption at a corner: "Proposal N · Name", 1–2 lines on the
   language, the platform switch, and the System button.
6. **Self-contained:** inline CSS and JS. Google Fonts `<link>`s are allowed. No frameworks, no
   external images.

## Constraint: it has to become a Chromium fork

The browser chrome is native Chromium Views, on Linux first, then macOS and Windows. The new tab
and blocked pages are HTML (WebUI). Avoid:
- blurring web content behind the chrome
- per-pixel full-window animation
- fonts that can't be bundled (Google Fonts are fine)

Put an HTML comment at the top of the file listing the 3–5 riskiest pieces to port.

## Quality bar

- A pitch to a design-literate founder: every pixel intentional.
- Consistent spacing, a real typographic hierarchy, considered empty, hover and focus states.
- Holds up in a screenshot.
- Usable at speed: picking and blocking should take two clicks.

## Verifying

The chrome-devtools MCP browser is often locked by other agents, so don't rely on it. Use your
own headless Chromium over CDP instead.
- `/tmp/mast-shots/tool/cdp.mjs` is a working example:
  `node cdp.mjs <file-url> <W> <H> <out.png> "<js>" [...]`
- Put your own scripts in your own `/tmp/<slug>/` folder.
- Check 1440×900 and 1280×800, every view, the System sheet, all three platforms, Block/Undo and
  the console.
