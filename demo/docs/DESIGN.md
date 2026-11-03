# Mast & Sail: the design language

Mast should not look like Chrome with a new colour. Its look comes from the logo: a straight
mast and a curved sail.

## Rules

1. **Structure is straight.** Right angles, 1px hairlines, radii of 2–5px. No pills and no
   round buttons.
2. **Only what is alive curves.** Two things curve: the active tab, shaped like a sail
   (straight luff on the left, curved leech on the right), and the glowing tip of a burning
   block.
3. **Ember means a promise.** Mast is graphite until something is blocked. Ember is only
   used for:
   - blocks and their status
   - struck-through blocked sites
   - the Block button
   - the forever end of the duration ruler

   Focus rings and selection are white hairlines.
4. **Read like an instrument.**
   - Numbers and statuses are monospace.
   - Durations are chosen on a tick-marked ruler: 1h, 4h, today, 1 wk, 1 mo, ∞.
   - Labels are small, mono and letter-spaced.
5. **A block is a line.**
   - A timed block is a fuse: an ember line burning from right to left, with a glowing tip.
   - A forever block is a solid ember line that never burns.
   - The same line appears next to each blocked category and site, under the toolbar on a
     blocked site, and through the site's name on the block page.

## Pieces

| Piece | Look |
|---|---|
| Tab strip | Inactive tabs on a "ruler": tick marks rising from the baseline. Blocked tabs struck through in ember |
| Active tab | Sail: square left edge, S-curve right edge flaring into the toolbar |
| Toolbar | Three cells split by hairlines; address centred (max 820px), recessed and darker than the toolbar. Right cell: **Block** for the current site, or its status ("forever", "midnight") if blocked |
| Address | Site bright, path dim; blocked site struck through |
| Horizon (below the toolbar) | Hairline; on a blocked site it carries that block's fuse or solid line |
| New tab | A ruled ledger of categories with their status lines, expandable to sites; a sticky bar with the duration ruler and Block button; an etched sea with a lone sail low on the page; a 7-bar log of distractions stopped |
| Block page | The site's name struck through by its block: the fuse shortens as time passes; forever is solid |

## Porting to Chromium (Views)

| Piece | Where | Cost |
|---|---|---|
| Radii, square buttons | `GetLayoutConstant` (toolbar/location bar corner radius), `MenuConfig`, `ToolbarButton` ink drop shape | Low |
| Sail tab path | `TabStyleViews::GetPath` (Brave overrides the same function) | Medium |
| Ruler ticks between tabs | `TabStyleViews` separator painting: short ticks at the baseline instead of full-height separators | Low |
| Toolbar cells, centred address | `ToolbarView` layout: flexible spacers around `LocationBarView` with a max width; 1px separators | Medium |
| Recessed address, site/path emphasis | Colour mixer for the omnibox background; the dimmed path already exists upstream (simplified domain display) | Low |
| Horizon line | A thin custom View at the top of the contents container, painted from the current tab's block | Medium |
| Struck-through blocked tab titles | `TabStyleViews`/`Tab` title rendering when the URL is blocked | Medium |
| New tab and blocked pages | Our own WebUI HTML: a direct port | Low |
