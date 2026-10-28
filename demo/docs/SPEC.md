# Mast — one button, no distractions

> From the Ulysses pact: Ulysses had his crew tie him to the mast so he could hear the
> Sirens without being able to steer toward them.

Blockers built as extensions fail at the moment they matter most: you can open
`chrome://extensions` and switch them off. Mast builds blocking into the browser itself.
Once a session starts, nothing inside the browser can end it early.

This repo is the **SvelteKit demo**: a full-viewport mock of the browser (tabs, omnibox,
menus, internal pages, simulated websites) used to settle the look, feel and behaviour
before the Chromium fork.

## The product

1. **One button.** Pick a length (25m, 50m, 1h 30m, 2h, 3h, 4h) and press Start. The last
   length is preselected, so starting is one click.
2. **All or nothing.** During a session, every site on the blocklist is blocked. No
   allowances, no "5 more minutes", no speed bumps, no continue-anyway button.
3. **No stop button.** A session can be extended (+30 min) but never shortened or
   stopped. The only exception is a 10-second Undo right after Start, for mis-clicks.
4. **The blocklist is already filled in.** Distractions are well known: social, video,
   news, shopping, games. Users can add their own sites. Additions work mid-session
   (stricter is always allowed); removals only between sessions.

## Browser surfaces (demo → Chromium)

| Demo surface | What the user sees | Chromium equivalent for the fork |
| --- | --- | --- |
| New tab page | Length picker + Start; big countdown while running | NTP WebUI |
| Toolbar "Block" button / countdown | Same control in a popover from any page | Toolbar action button + bubble |
| Block screen | "x.com is blocked", countdown, Back to work | Interstitial via a `NavigationThrottle` |
| `mast://blocklist` | Preset list + your additions | WebUI page |
| `mast://settings` | Incognito, guest, DevTools, reset, uninstall locked during a session | Settings WebUI + policy-like locks |
| Omnibox status icon | Lock on blocked sites; "Add to blocklist" on others | Location bar icon view |

## Tamper resistance (what the fork must handle)

- **Incognito / guest / new profile:** off during a session; blocking is browser-wide.
- **Extensions:** blocking is not an extension, so there is nothing to disable.
- **DevTools:** off on blocked sites.
- **Clock changes:** sessions run on monotonic time checked against network time.
- **Clear data / delete profile:** the running session lives outside the profile.
- **Uninstall:** an OS service declines to uninstall during a session (platform specific).
- **Another browser:** can't be solved inside a browser. Option: an optional OS companion
  ("Mast Guard") that closes other browsers during a session. Open question.

## Demo-only controls

A floating **Demo** drawer (bottom-left): time machine (+10 min, +30 min, +1 hour, back to
real time), "try to escape" actions showing how the browser responds, and a reset.

## Code map

- `src/lib/mast/focus.svelte.ts` — the store: blocklist additions, sessions, attempts.
- `src/lib/mast/blocklist.ts` — preset distraction list.
- `src/lib/mast/engine.ts` — pure helpers (matching, time formatting).
- `src/lib/components/` — browser chrome, `StartControl`, internal pages.
- `src/lib/sites/` — simulated websites. Each takes `{ url, clean, navigate }`; `clean`
  is unused for now (always `[]`). Plain `<a href>` clicks are intercepted by the shell.
