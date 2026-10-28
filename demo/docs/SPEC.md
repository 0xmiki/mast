# Mast — a browser where commitments are irreversible

> Working name. From the Ulysses pact: Ulysses had his crew tie him to the mast so he
> could hear the Sirens without being able to steer toward them.

Blockers built as extensions fail at the moment they matter most: the user who made the
commitment can open `chrome://extensions` and switch it off. Mast moves the commitment
into the browser itself. Once a commitment (a **pact**) is sealed, nothing inside the
browser can weaken it until it ends.

This repo is the **SvelteKit demo**: a full-viewport mock of the browser (tabs, omnibox,
menus, internal pages, simulated websites) used to settle the look, feel and behaviour
before the Chromium fork.

## Core rules

1. **Sealed means sealed.** After sealing, a pact cannot be deleted, paused, shortened or
   loosened from anywhere in the browser.
2. **One-way ratchet.** A sealed pact can always be made *stricter*: add sites, extend the
   end date, widen block hours, lower a budget, lengthen a wait, drop the escape clause.
   Every tightening is written to the pact's amendment log.
3. **Regret window.** For 2 minutes after sealing, the pact can be cancelled outright.
   This protects against typos, not against temptation.
4. **Strictest wins.** When pacts overlap on a site: block > budget used up > speed bump >
   allow (with cleaning applied).
5. **Your own words.** Every pact carries a sentence the user wrote ("I want my mornings
   back"). The block screen shows it back to them.
6. **No hidden bypass.** The block screen has no "continue anyway" button. The only
   optional exit is the *cool-down escape clause*, chosen at creation time: requesting
   release starts a 24-hour wait, enforcement continues during it, and the pact is
   recorded as broken.

## Pact types

| Type | What it does | Ratchet (allowed after sealing) |
| --- | --- | --- |
| **Block** | Sites unreachable during a schedule (always, or days + hours) | add sites, add days, widen hours, extend end |
| **Budget** | N minutes per day across a group of sites, then blocked until midnight | add sites, lower minutes, extend end |
| **Focus** | For a fixed session, *only* the allow-listed sites open | remove allowed sites, extend session |
| **Speed bump** | A forced wait (breathing screen) before a site opens | add sites, longer wait, extend end |
| **Clean** | Strips distracting parts of sites: feeds, Shorts, recommendations, comments | add sites, add features, extend end |

## Browser surfaces (demo → Chromium)

| Demo surface | What the user sees | Chromium equivalent for the fork |
| --- | --- | --- |
| Omnibox shield | Per-site status: sealed, budget left, speed bump, cleaned | Page action / location bar icon view |
| Pacts side panel | Live countdowns, budgets left, quick "new pact" | Side panel (`SidePanelEntry`) |
| `mast://pacts`, `mast://pacts/new`, `mast://pacts/<id>` | Ledger, creation wizard, pact detail + tighten | WebUI pages (`chrome://` style) |
| Block / speed bump / budget screens | Interstitial replacing the page | Security-interstitial style page via a `NavigationThrottle` |
| Clean mode | Feeds, Shorts, comments hidden in-page | Built-in content script / CSS injection, not an extension |
| `mast://settings` | Locked toggles while pacts run | Settings WebUI + enterprise-policy-like locks |
| Menu: incognito, extensions, DevTools | Disabled while pacts run | `IncognitoModePrefs`, `DeveloperToolsAvailability` policies |

## Tamper resistance (what the fork must handle)

- **Incognito / guest / new profile** — disabled or covered: pacts are browser-wide, not per profile.
- **Extensions** — pacts are not an extension, so there is nothing to disable.
- **DevTools** — disabled on sealed sites.
- **Clock rollback** — enforcement uses monotonic time plus network time; local clock changes are ignored.
- **Clear data / delete profile** — the pact store lives outside the profile directory and is signed.
- **Uninstall** — OS-level: the installer/service refuses removal while a pact runs (platform specific).
- **Another browser** — *cannot* be solved inside a browser. Options: an optional OS companion
  ("Mast Guard") that blocks other browsers during pacts, or OS parental controls. Open question.

## Demo-only controls

A floating **Demo** drawer (bottom-left) provides: a time machine (+1h, +1 day, jump to
next 09:00, back to real time), example scenarios, "try to escape" actions that show how
the browser responds, and a reset. None of these exist in the real product.

## Code map

- `src/lib/mast/` — clock, types, pure enforcement engine, pact + browser state.
- `src/lib/components/chrome/` — tab strip, toolbar, omnibox, menu, side panel, toasts.
- `src/lib/components/pages/` — `mast://` internal pages and interstitials.
- `src/lib/sites/` — simulated websites (see contract below).

## Simulated site contract

Each site is a Svelte 5 (runes, TypeScript) component in `src/lib/sites/` that receives
`SiteProps` from `src/lib/sites/types.ts`:

```ts
interface SiteProps {
  url: URL;               // the page URL the tab is showing (read pathname / searchParams)
  clean: CleanFeature[];  // parts the user's Clean pacts strip from this site
  navigate: (href: string) => void; // for forms; plain <a href> clicks are intercepted by the shell
}
```

- Links are ordinary `<a href="...">` (absolute `https://…` or site-relative `/path`). The
  shell intercepts the click and navigates the tab.
- Everything is local: no network, no images, no fonts. Thumbnails/avatars are CSS
  gradients or initials derived deterministically from ids.
- Each component has one root element that sets its own background, colour and font and
  fills the content area (`min-height: 100%`). Styles are scoped; no `:global`.
- Cleaned parts render `<CleanedNotice label="…" />` in their place.
