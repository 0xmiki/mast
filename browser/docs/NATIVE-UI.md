# Styling Chromium's native UI (Views) like we style the web

Chromium's tabs, toolbar, omnibox, menus and bubbles are **not HTML**. They're C++ "Views",
drawn with Skia. This doc maps CSS habits onto Views, explains why our first restyle left gaps,
and gives the workbench and the exact knobs to close them. Read against tag
**156.0.8078.79**. Anything not read from source is marked *(inferred)*.

## 1. CSS → Views, concept by concept

| CSS idea | Views equivalent | Where | Gotcha |
|---|---|---|---|
| DOM tree | View tree (Widget → View → children) | `ui/views/view.h` | Inspect it live with UI DevTools (§3). |
| `color`, `background` | `ui::ColorId` resolved by a **mixer pipeline** | `ui/color/`, `chrome/browser/ui/color/` | Hundreds of IDs, derived in layers. Change the source token and every derivative moves too. |
| Cascade order / `!important` | **Mixer order**: ui chain (Ref → Sys → … → NativePostprocessing), then chrome chain (Chrome → … → NativeChrome → custom theme) | `ui/color/color_mixers.cc`, `chrome_color_mixers.cc` | A value set in a late mixer wins. Recipes read the *final* provider, so overriding a sys token late propagates. |
| Inheritance | Almost none. Only "cascading properties" (e.g. `kCascadingBackgroundColor`) | `ui/views/cascading_property.cc` | Each view picks its own colours. |
| `display:flex`, `flex-grow`, `gap` | `FlexLayout` / `BoxLayout`, `kFlexBehaviorKey`, `kMarginsKey`, `SetBetweenChildSpacing` | `ui/views/layout/` | Defaults differ from CSS (next row). |
| `text-align` | `Label::SetHorizontalAlignment` | `ui/views/controls/label.cc` | **Default is `ALIGN_CENTER`** (`label.cc:1348`). In a stretching `BoxLayout` a label spans the width and centres its text. That's our bubble's centred body. |
| `padding` | `Border` insets / `SetInteriorMargin` | | |
| `border-radius` | `LayoutProvider::GetCornerRadiusMetric(ShapeContextTokens / Emphasis)`, plus per-view highlight path generators | `ui/views/layout/layout_provider.cc:233-244` | Central for dialogs, menus, combobox, textfields and buttons. Hard-coded in some views (omnibox pill, omnibox rows, tabs). |
| `outline` / `:focus-visible` | `views::FocusRing` (halo 2 px, `focus_ring.h:44`) | `ui/views/controls/focus_ring.cc` | **Colour is forced to ≥3:1 contrast** against the cascading background (`cascading_property.cc:78`, `BlendForMinContrast(..., kMinimumVisibleContrastRatio)`). You cannot make a ring quiet with colour alone. Give it its own ColorId (`FocusRing::SetColorId`), which bypasses the clamp. |
| `:hover` / `:active` | State ColorIds (`…Hovered`, `…Selected`), ink drops | per control | |
| `font-family` | `gfx::FontList` default, `ChromeTypographyProvider` | `ui/gfx/platform_font_skia.cc:150` | On Linux the family comes from GTK (`gtk-font-name`) or Qt, else fontconfig "sans". There's no switch. |
| Button variants | `MdTextButton` styles: default button = `kProminent`, others `kTonal` (`dialog_delegate.cc:220`) | `ui/views/controls/button/md_text_button.cc` | Prominent = `kColorSysPrimary` on `kColorSysOnPrimary`. Radius is a pill (`kFull`). |
| DevTools Elements panel | **UI DevTools**: `--enable-ui-devtools=9223` | `components/ui_devtools/` | See §3. |
| Outline every box | `--draw-view-bounds-rects` | `ui/views/views_switches.h` | Works in release builds. |

## 2. Why our first restyle left gaps (honest post-mortem)

1. **We changed only the colour layer.** Shapes, radii, heights, button variants and fonts live
   elsewhere, so they stayed Chrome's.
2. **One token choice cascaded unseen.** We made `primary` neutral, so dark tone 80 =
   `#E9E7E3`. Everything derived from it turned near-white:
   - focus rings (`kColorSysStateFocusRing` → Primary80);
   - prominent buttons (`kColorSysPrimary`).

   We also set `kColorFocusableBorderFocused = muted #a29f99`, a light grey. Even a dark value
   would have been lightened by the 3:1 clamp.
3. **No inspector.** We guessed which ColorId painted a pixel from screenshots, with a rebuild
   between guesses. Chromium ships a native-UI inspector; we didn't use it.
4. **CSS defaults don't transfer.** `Label` centres text by default. A `BoxLayout` stretches
   children by default.

The fix isn't "try harder". It's: inspect first, change the right layer, check against the design.

## 3. The native-UI workbench

On the builder (Xvfb):
```sh
out/Mast/chrome --user-data-dir=/tmp/mast-prof \
  --enable-ui-devtools=9223 --remote-debugging-port=9222 \
  --force-device-scale-factor=1 --force-color-profile=srgb --animation-duration-scale=0 \
  [--draw-view-bounds-rects]
```

- UI DevTools listens on **localhost only** (`devtools_server.cc:134`). It has one client at a
  time and no `/json` endpoint; the socket is `ws://127.0.0.1:9223/0`.
- From the laptop: `ssh -N -L 9223:127.0.0.1:9223 -L 9222:127.0.0.1:9222 <builder>`, then open
  `devtools://devtools/bundled/devtools_app.html?uiDevTools=true&ws=127.0.0.1:9223/0` in a
  local Chrome.
  - `chrome://inspect#native-ui` also has a "Bubble Locking" checkbox that keeps bubbles open.

What it gives us:
- **The tree:** class and name per view; `DOM.getNodeBoundsInScreen` gives screen bounds.
- **Properties:** one group per metadata class. View `Background`/`Border` show their **ColorId
  name** (`ColorVariant::ToString` → `ColorIdName`).
- **Live edits:** writable serialisable properties (`Visible`, `X/Y/Width/Height`, a label's
  `EnabledColor`, …) via `CSS.setStyleTexts`.
  - Values are lower-cased first, so text edits get mangled.
  - Background and Border are read-only.

Scripted: `remote/dump-views.mjs` (to write; sketch in the research notes) walks
`DOM.getDocument`. It records class, name, screen bounds and properties to JSON. We pair that
with a full-screen Xvfb grab, cropped per view, so every region of a screenshot names its class
and ColorIds. That replaces guessing.

Colour tracing:
- **A dump behind a Mast switch:** loop `ui::kUiColorsStart … kChromeColorsEnd` and log
  `ColorIdName(id) = SkColorName(GetColor(id))` (`ui/color/color_provider_utils.h`). Then grep for
  a pixel's value.
- **Per-lookup logs:** with a dcheck build, `--vmodule=color_provider=2` logs every lookup.

Per-bubble screenshots without clicking around: `browser_tests` `TestBrowserUi` with
`--test-launcher-interactive --ui=…` (and `--browser-ui-tests-verify-pixels
--bypass-skia-gold-functionality --skia-gold-local-png-write-directory=…`). This uses our real
mixers, but costs a `browser_tests` build, so it's for later.

## 4. Styling map (where each visible property lives)

**Omnibox**
- The focus ring is installed with no ColorId (`location_bar_view.cc:239`), so it takes the
  clamped accent.
- Field colours:
  - focused: `kColorOmniboxResultsBackground`;
  - unfocused: `kColorLocationBarBackground`;
  - stroke: `kColorLocationBarBorder`.
- Shape: radius `Emphasis::kMaximum` (a pill, `:533`), height `kLocationBarHeight` 34.
- Popup:
  - background `kColorOmniboxResultsBackground`, rows `…Hovered/…Selected`;
  - focus bar `kColorOmniboxResultsFocusIndicator`;
  - frame radius `kOmniboxExpandedRadius` 16;
  - row height 40, right-rounded rows (hard-coded, `omnibox_result_view.cc:346`).

**Tabs**
- Path: `HorizontalTabStyleViews::GetPath` (`horizontal_tab_style_views.cc:65`). Its
  highlight/hit-test branch (`:92`) already draws a detached rounded rect.
- Radii: top 10, bottom 12 (the swoop) (`tab_style.cc`).
- Heights and spacing: `kTabHeight` 35, `kTabStripPadding` 6.
- Tab colours: `kColorTabBackground*`, `kColorTabDividerFrameActive`,
  `kColorTabStrokeFrameActive`.
- Control buttons (tab search, new tab): 28×28 pills, `kColorNewTabButtonBackgroundFrameActive`.

**Dialogs and bubbles**
- Radius: `set_corner_radius()` per bubble, else `kDialogRadius` 12.
- Colours: `kColorBubbleBackground` / `kColorBubbleBorder`.
- Prominent buttons: `kColorButtonBackgroundProminent` (+`Focused`, `Disabled`) and
  `kColorButtonForegroundProminent`. Tonal: `kColorButtonBackgroundTonal` /
  `kColorButtonForegroundTonal`.
- Combobox: `kColorComboboxBackground`, `kColorComboboxContainerOutline`, radius 8.

**Menus**
- `MenuConfig` (`menu_config.cc`): item vertical margin 6, separator height 17, corner radius
  12 on Linux.
- Colours: highlight `kColorMenuItemBackgroundSelected`; app-menu profile row 12 px
  (`app_menu.cc:1583`).

**Font**
- `PlatformFontSkia::EnsuresDefaultFontIsInitialized` takes the family from `LinuxUi`.
  Setting the family there is global.

## 5. The fixes, smallest first

Mixer-only (our files, no upstream patch):
1. `kColorSysStateFocusRing` → a quiet value. Rings still pass through the clamp, so this
   mainly fixes buttons and checkboxes.
2. Prominent button = ember: `kColorButtonBackgroundProminent` / `…Focused` / `…Disabled` →
   `#ff6a3d`, `kColorButtonForegroundProminent` → `#141414`. Tonal → raised grey.
3. Omnibox:
   - `kColorOmniboxResultsBackground` / `kColorLocationBarBackground` → `#262626`;
   - hovered/selected rows → `#2c2c2c`;
   - `kColorOmniboxResultsFocusIndicator` → transparent or muted.
4. Tabs: divider and stroke → transparent, `kColorNewTabButtonBackgroundFrameActive` → frame;
   menu highlight → `#2c2c2c`.

Our own code:

5. Block bubble (replaced by the page-info row in the roadmap, but the same rules apply to
   every Mast bubble): `SetHorizontalAlignment(gfx::ALIGN_LEFT)` on labels and
   `set_corner_radius(8)`.

Small hooks (1–3 lines each):

6. `location_bar_view.cc`, after `FocusRing::Install`:
   `SetColorId(kColorLocationBarBorder)` and `SetHaloThickness(1.f)`. This **bypasses the clamp**;
   then the mixer sets the ring to `#2c2c2c` or transparent.
7. `layout_provider.cc` radius table: kSmall 8→6, kMediumSmall 12→8, kMedium 16→10,
   `kButtonRadius` → 6. One switch covers dialogs, menus, comboboxes, textfields and the popup.
8. `platform_font_skia.cc:177`: family "Inter" (and ship the font). Alternative with no patch:
   GTK `gtk-font-name`, but that's per user and not ours to set.
9. Tabs: `horizontal_tab_style_views.cc:92`, the detached rect for all path types;
   `tab_style.cc` top radius 6, bottom ~3 (keeps the overlap maths non-negative *(inferred)*).
10. Omnibox shape: `location_bar_view.cc:533`, radius 6 instead of the pill, and
    `InstallRoundRectHighlightPathGenerator(this, {}, 6)`.

Checks, each against the Pencil design:
1. UI DevTools dump: every visible view's Background/Border ColorId is one of ours.
2. Screenshot crops per view next to the matching Pencil component exported at the same size.
3. Rings: focus the omnibox, tab through the bubble, and confirm no ring above `#2c2c2c`
   contrast.
