# Mast — a browser that blocks for real

> From the Ulysses pact: Ulysses had his crew tie him to the mast so he could hear the
> Sirens without being able to steer toward them.

Blockers built as extensions fail at the moment they matter most: you can open
`chrome://extensions` and switch them off. Mast builds blocking into the browser itself.
Once a site is blocked, nothing inside the browser can unblock it early.

This repo is the **SvelteKit demo**: a full-viewport mock of the browser (tabs, omnibox,
menus, internal pages, simulated websites) used to settle the look, feel and behaviour
before the Chromium fork.

## The product

1. **Blocking is the product.** There's no focus timer and no session to run. You block
   sites, and they stay blocked.
2. **A categorised list.** Distractions are well known, so the list is already filled in:
   Social, Video & streaming, News & forums, Shopping, Games. Users add their own under
   "Your sites". Pick whole categories or single sites.
3. **You choose how long:** 1 hour, 4 hours, until midnight, a week, a month, or forever.
4. **Blocks only get longer.**
   - A block can be extended (including to forever) but never shortened or lifted.
   - **Forever is forever.**
   - The only way back is a 10-second Undo right after pressing Block, for mis-clicks.
5. **Block the site you're on.** The toolbar's Block button blocks the current site, with the
   same duration choices. Sites not on the list join "Your sites".
6. **No friction theatre.** No allowances, no "5 more minutes", no continue-anyway button.

## Browser surfaces (demo → Chromium)

| Demo surface | What the user sees | Chromium equivalent for the fork |
| --- | --- | --- |
| New tab page | Categorised list with block status; duration ruler + Block | NTP WebUI |
| Toolbar Block button | Block the current site; shows "forever" / end time on blocked sites | Toolbar action button + bubble |
| Block screen | The site's name struck through; time left or "blocked forever" | Interstitial via a `NavigationThrottle` |
| `mast://blocklist` | The same list as the new tab page | WebUI page |
| `mast://settings` | Incognito, guest, DevTools, reset, uninstall locked while anything is blocked | Settings WebUI + policy-like locks |
| Omnibox | Blocked site struck through; status popover | Location bar icon view |

## Tamper resistance (what the fork must handle)

- **Incognito / guest / new profile:** off while any site is blocked; blocking is
  browser-wide.
- **Extensions:** blocking is not an extension, so there is nothing to disable.
- **DevTools:** off on blocked sites.
- **Clock changes:** blocks run on monotonic time checked against network time.
- **Clear data / delete profile:** blocks live outside the profile.
- **Uninstall:** an OS service declines to uninstall while blocks are on (platform specific).
  With a forever block, that is never.
- **Another browser:** can't be solved inside a browser. Option: an optional OS companion
  ("Mast Guard") that closes other browsers. Open question.

## Demo-only controls

A floating **Demo** drawer (bottom-right) has:
- a time machine: +10 min, +1 hour, +1 day, back to real time
- "try to escape" actions showing how the browser responds
- a reset

## Code map

- `src/lib/mast/blocks.svelte.ts`: the store (categories, per-site blocks, undo, attempts).
- `src/lib/mast/blocklist.ts`: the preset categories.
- `src/lib/mast/engine.ts`: pure helpers (matching, time formatting).
- `src/lib/components/`:
  - browser chrome
  - `BlockPanel` (the list) and `BlockAction` (the ruler and Block button)
  - `Fuse` (how a block looks)
  - internal pages
- `src/lib/sites/`: simulated websites. Each takes `{ url, clean, navigate }`.
  - `clean` is unused for now (always `[]`).
  - Plain `<a href>` clicks are intercepted by the shell.
