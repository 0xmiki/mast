# Making Mast look like Mast (not Chrome)

Plan, 2026-10-28. Sources: our own SvelteKit demo (`../browser`, the design target), Helium's
patch series and Brave's `brave-core` (both pinned to Chromium 156, same as us), read over the
web, not cloned. Full research notes with links are summarised at the end.

## 1. Where we are

Working: the session engine, the block page, the Mast new tab page, the Block/countdown toolbar
button, Mast product strings, and the Mast app icon.

Still Chrome:
- **Colours.** These are Material You tones seeded from our ember, so "Chrome with an orange
  tint" rather than our palette.
- **Shapes and density.** Chrome's tab strip, toolbar, omnibox, menus, dialogs and ripple
  effects are unchanged.
- **Icons.** Chrome's back, forward, reload, menu and other toolbar icons.
- **Fonts.** Native UI uses the system font, and internal pages use Roboto.
- **Logos and names still visible:**
  - the Chromium logo on `chrome://version`
  - the `chrome-product` glyph in the shared WebUI toolbar and drawer
  - product glyphs in the omnibox
  - favicons of `chrome://` pages
  - "Chromium" in strings outside `chromium_strings.grd` and in every translation
  - the Linux desktop name and WM_CLASS
  - "The Chromium Authors" on the About page

## 2. Design target (from the demo, as numbers)

| Token | Value | Used for |
|---|---|---|
| bg | `#0c0d0f` | window frame, tab strip |
| chrome | `#141518` | toolbar, active tab, panels, menus |
| chrome-2 | `#1b1c20` | omnibox resting |
| raised | `#22242a` / `#2a2c33` | hover, buttons, chips |
| line | `rgb(255 255 255 / .07)` / `.12` | separators, borders |
| text | `#ececef` / muted `#9a9ca5` / faint `#6b6e78` | |
| ember | `#ff6a3d` (hover `#ff8f5e`, soft `rgb(255 106 61 / .14)`) | accent, focus ring, session state |
| font | Inter (UI), JetBrains Mono (countdowns) | |
| tab strip | 42 px; tab 34 px; top radius 10 px; flared bottom corners; separators only between inactive tabs | |
| toolbar | 46 px; round 32 px buttons; no dividers | |
| omnibox | 34 px pill; focus = ember 1 px border + 3 px soft ember ring | |
| menus | radius 14 px, 6 px padding, 34 px items with radius 8 px, big soft shadow | |
| motion | no ripples; 150 ms fades | |

## 3. Principles

1. **Brave's maintenance rule.** Mast code lives in `src/mast/`. Upstream is touched only by
   one-to-three-line hooks that call into it. In order of preference: our own code, then a
   small hook, then a function wrapper, then a hand patch as a last resort.
2. **Write our own changes; don't copy Helium's patches.** Helium's patches are GPL-3.0, and
   copying them would make Mast GPL. We learn their techniques (which file, which constant)
   and write our own code. Brave's code is MPL-2.0; same approach.
3. **Copy assets, don't patch them.** Icons, logos and fonts are whole-file overrides from
   `src/overrides/` (already in place), driven by a list, as both Helium and Brave do.
4. **One design source.** A single `mast_colors.h` (and matching CSS variables) mirrors the
   demo tokens, so native UI, WebUI pages and our own pages can't drift apart.

## 4. The work, by layer

### A. Colour: the biggest visual win for the fewest lines
- **Reference palette** (`mast_ref_color_mixer.cc`). Replace the Material baseline tones:
  Neutral is the demo's near-blacks, Primary is ember. Every `kColorSys*` value derives from
  these, so frame, menus, dialogs, bubbles and WebUI follow at once.
  Hook: one call after `AddRefColorMixer` in `ui/color/color_mixers.cc`.
- **Surface mixer** (`mast_color_mixer.cc`). A postprocessing mixer that pins exact surfaces
  regardless of mixer order:
  - frame and inactive frame = bg
  - toolbar and active tab = chrome
  - inactive tab = transparent, with hover = raised
  - omnibox = chrome-2, with hover and focus variants
  - menu, bubble and dialog backgrounds, separators, focus ring = ember

  Hook: one call in `chrome/browser/ui/color/chrome_color_mixers.cc` plus a BUILD dep. Also
  make `ShouldApplyChromeMaterialOverrides()` return false.
- **Theme defaults.** Dark, our palette, no GTK and no Material You seeding (replaces today's
  "user colour = ember" trick). Hide the "Customize Chrome" colour pickers that would fight it.

### B. Shape and density
- **Layout constants.** An early-return hook in `GetLayoutConstant`/`GetLayoutInsets`
  (`chrome/browser/ui/layout_constants.cc`) that calls `mast::GetLayoutConstant()`. It sets
  tab height 34, toolbar 46, buttons 32, omnibox 34, tab/toolbar overlap and the interior
  margins. The omnibox stays a pill for free (upstream uses `kMaximum`).
- **Tabs** (`MastTabStyle`, one-token swap in `TabStyle::Get()`). Top radius 10, our widths,
  separator style, no tab strokes. We keep the demo's Chrome-like flared tabs rather than
  Helium/Brave's detached pills (decision 1).
- **Menus and dialogs.** `MenuConfig` (radius 14, item padding) and `LayoutProvider` radii.
- **No ripples.** `PlatformStyle::kUseRipples = false`, plus `noink` in WebUI `cr-ripple`
  (about 4 lines).
- **Toolbar** (`MastToolbarView` or targeted hooks):
  - remove the divider
  - hide the avatar with one profile
  - hide Labs, Lens, Glic/AI and reading-list entry points (feature-default flips)
  - keep back, forward, reload, the omnibox, Block and the menu
- **Omnibox.**
  - keep the search-engine favicon (Google stays the default; decision 2)
  - hide the AI/Lens/payments page actions
  - ember focus ring
  - restyle the popup: radius 14 and our colours

### C. Icons
- **Vector icon overrides.** A JSON map from upstream `.icon` paths to Mast `.icon` files,
  applied by a two-line hook in `components/vector_icons/aggregate_vector_icons.py` (Brave's
  approach). We'll draw Lucide-style 2 px-stroke versions of the 15–25 icons people actually
  see: back, forward, reload, stop, menu, extensions, downloads, the omnibox lock/info/star,
  close tab, new tab.
- **Product glyphs.** Replace `browser_logo`, `chrome_product`, omnibox `product*` and
  `components/vector_icons/chromium/product*` with the sails.

### D. Typography
- **Native UI.** Inter as the default UI font. This is a one-line hook at
  `ResourceBundle`'s `SetDefaultFontDescription` (an idea from the Brave research that Brave
  itself doesn't use). It needs Inter shipped with the browser package (OFL licence).
- **WebUI.** Inter in `text_defaults_md.css` (Brave copies a replacement file). Our own pages
  load the same font from `chrome://resources`.

### E. Branding, every trace
- **Name substitution.** Run "Chromium/Google Chrome → Mast" across **all** `.grd`/`.grdp`
  files and remap the `.xtb` translation fingerprints, so every language keeps working.
  Helium's approach; we write our own script.
- **Asset copy list**, extending `src/overrides`:
  - `components/resources/*/chromium/product_logo*.png` (`chrome://version`)
  - `chrome_logo_dark.svg`
  - `chrome://` page favicons
  - the `chrome-product` glyph in `cr_elements/icons.html.ts`
- **Linux identity.** Desktop name `mast.desktop`, WM_CLASS `Mast`, icon name, user-data dir
  (done).
- **About and credits.** "The Mast Authors" plus "Built on Chromium" with its licence link
  (required by the licence).
- **Default search engine.** Stays Google (decision 2).
- **Remove Google promos and intros** (resource and feature flips).

### F. Internal pages (WebUI)
- Palette variables in `cr_shared_vars.css`, plus `cr_button`/`cr_icon_button` radii to match
  the demo.
- Settings: add a Mast stylesheet through `settings_shared.css`. Lock the session-sensitive
  rows visually (the demo's locked rows), and hide sections Mast doesn't want (Autofill
  promos, Safety Hub nags).
- Our NTP, block page and `chrome://mast` move to Inter and the shared tokens (fixes today's
  monospace fallback).

## 5. How we'll check it looks right

The same loop I used for the demo:
1. Build.
2. Launch Mast headful under Xvfb **on the builder**.
3. Take a full-window screenshot of a fixed set of scenes:
   - new tab
   - a site
   - a blocked site
   - omnibox focused and typing
   - the app menu
   - settings
   - a running session
4. Compare each next to the matching demo screenshot.

Screenshots are tiny, so the slow Hetzner link doesn't matter, and nothing pops up on your
screen. The command will be `./mast shots`, with scenes scripted over the DevTools protocol
plus `xdotool` for native menus.

## 6. Phases and budget

Builder: CPX62 at $0.245/h, restored from the snapshot in about 10 minutes. Budget left: about
**$3.37** (the snapshot costs $0.31/month on top). Build times are estimates: anything that
changes widely included headers (colour IDs, vector icons) recompiles a large share of the
browser.

| Phase | Work | Builder time (est.) | Cost (est.) |
|---|---|---|---|
| 0 | Restore, `./mast shots` harness, baseline screenshots | 0.5 h | $0.12 |
| 1 | Colour (A) + theme defaults + no ripples + layout constants (B) | 1–1.5 h | $0.25–0.37 |
| 2 | Branding sweep (E): name substitution + xtb remap, asset list, Linux identity, About | 0.5–1 h | $0.12–0.25 |
| 3 | Tabs, menus, toolbar clean-up, omnibox (B) + icons (C) | 1–1.5 h | $0.25–0.37 |
| 4 | Fonts (D) + WebUI/settings polish (F) + our pages on shared tokens | 0.5–1 h | $0.12–0.25 |
| | **Total** | **3.5–5.5 h** | **≈ $0.85–1.35** |

That leaves about $2 for the monthly Chromium security update (a near-full rebuild, about
$0.50–1.00 each) and mistakes. We snapshot and shut down between phases, so idle time costs
nothing.

Upstream footprint after all phases: about 30–40 hook lines plus the string and asset sweeps,
still small enough to rebase each month in an hour or two.

## 7. Decisions (made 2026-10-28)

1. **Tab shape:** flared, like the demo. Chromium's tab path stays; we restyle colours, sizes,
   radius and separators.
2. **Default search engine:** Google. We keep its search icon in the search-engine UI. Mast
   branding removes Chrome/Chromium names and product logos, not Google as a search provider.
3. **Light mode:** dark only for v1.

## 8. Research notes (short)

**Helium** (156.0.8078.73):
- Colour: `helium-color-scheme` (ref palette swap), `helium-color-mixers` (surfaces).
- Shape and density: `layout-constants` (one base padding), `layout-provider` (radii), `tabs`
  (detached shape, no strokes), `location-bar`/`omnibox`, `toolbar` + `remove-toolbar-dividers`.
- Small flips: `disable-ink-ripple-effect`, `enable-glow-up-features` (feature flips).
- Branding and assets: `resources/helium_resources.txt` copy list, `utils/name_substitution.py`
  (grd + xtb fingerprint remap), `omnibox-default-search-icon` (drops the "G"),
  `engine-defaults` (DuckDuckGo).
- WebUI: `update-cr-components` (WebUI CSS radii, removes `<picture>` logos).
- Helium ships no custom UI font.

**Brave** (156.0.8078.74):
- Colour: `AddBrave*ColorMixer` hooks at the end of each upstream mixer, a postprocessing
  mixer, `AddBraveRefColorMixer` after `AddRefColorMixer`, Material overrides off.
- Shape and toolbar: a `GetLayoutConstant` wrapper, `BraveTabStyle` via a one-token swap,
  `BraveToolbarView` via a class swap (removes divider, hides avatar), `MenuConfig` radius.
- Icons and fonts: a vector-icon override JSON (about 270 icons), and Inter in WebUI through
  a replacement `text_defaults_md.css`.
- Branding: copy-step (no patches), plus `brave_strings.grd` generated from `chromium_strings`
  by regex.
- Maintenance rule: our own code, then subclass + tiny hook, then wrap, then patch.

## 9. Progress (2026-10-28)

**Done** (builds 1–4, about $0.70 of builder time):
- Phase 0: Xvfb screenshot loop (`./mast shots`), baseline captured.
- Phase 1: Mast ref palette (ember accent, cool-grey secondary), Chromium's dark neutrals 26
  and surface tokens, a postprocessing surface mixer, no Material You seed, a readable
  countdown pill, Inter/JetBrains Mono on Mast's pages.
- Phase 2:
  - names renamed in every string file at build time (`substitute-names.py`), with
    "The Mast Authors" and the Chromium credit kept
  - `chrome://version` wordmarks, favicons and the dark logo
  - product vector icons and the WebUI glyph replaced with the sails
  - `mast.desktop`
- Phase 3 (part): no ripples, no profile button, ember text selection, Inter for WebUI.

**Left** (next session):
- Hide the tab-search chevron and the omnibox "AI Mode" chip, and drop the Lens and AI
  entry points (feature-default flips).
- "You and Google": rename it or hide it, since Mast has no sign-in. Same for the "Your Mast"
  menu item.
- Menus: 14 px radius and roomier items (MenuConfig). Omnibox popup: radius and colours.
- Native UI font: Inter (needs the font shipped or installed).
- Vector icons for back, forward, reload and menu in the demo's stroke style (optional; the
  Chrome refresh icons are already close).
- Translated strings: `.xtb` fingerprint remap so other languages keep their translations.
