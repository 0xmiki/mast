# Mast

A deep-work browser: one button blocks every known distraction for a set time, and nothing
inside the browser can stop it early. Built on Chromium.

| Folder | What's in it |
|---|---|
| [`demo/`](demo) | SvelteKit prototype of the browser (look, feel and controls). `bun install && bun run dev`. |
| [`browser/`](browser) | The Chromium fork: Mast's own code (`src/mast/`), whole-file overrides (`src/overrides/`), small upstream patches (`patches/`), and the build tooling (`mast` CLI, Hetzner builder). |

Docs:
- product rules: [`demo/docs/SPEC.md`](demo/docs/SPEC.md)
- how Mast wires into Chromium: [`demo/docs/CHROMIUM.md`](demo/docs/CHROMIUM.md)
- the design plan and progress: [`browser/docs/DESIGN-PLAN.md`](browser/docs/DESIGN-PLAN.md)

The browser binary and the Chromium source are not in this repo. They live on the builder
and are rebuilt from `browser/` (pinned to Chromium `browser/chromium_version.txt`).

## Licence

Mast is free software under the [GNU General Public License v3.0](LICENSE).

It builds on [Chromium](https://www.chromium.org/) and
[ungoogled-chromium](https://github.com/ungoogled-software/ungoogled-chromium), both under
BSD-style licences. The patches in `browser/patches/` and the files in `browser/src/overrides/`
change their code, which stays under their notices (Chromium's `LICENSE` and
ungoogled-chromium's). The built-in pictures are NASA's, in the public domain
([credits](browser/wallpapers/CREDITS.md)); the wordmark is drawn from Host Grotesk (SIL Open
Font License).
