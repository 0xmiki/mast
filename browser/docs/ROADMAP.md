# Mast roadmap: the next challenges, solved from first principles

Written 2026-11-06 against Chromium **156.0.8078.79** (the tree we build). Every claim below was
read from that tag's source (fetched file by file from chromium.googlesource.com, nothing
cloned) unless marked *(inferred)*. Designs referenced live in `../../design/mast-browser.pen`.

## 0. First principles

A fork costs three things. Everything in this plan is about keeping them small.

| Cost | What drives it | Rule that minimises it |
|---|---|---|
| **Rebase time** | Lines of *upstream* text we change. Context lines go stale every release. | Logic lives in `src/mast/`. An upstream file gets a hook of **1–3 lines**, never logic. |
| **Money** | Full rebuilds (~2.5 h × $0.245/h ≈ **$0.61** on cpx62). Incremental builds cost cents. | Batch work into one server session. Never touch a widely-included header without need. |
| **Uncertainty** | Code we haven't read. | Read the upstream code before writing ours. Every step has a check (screenshot, hit test, or `file.cc^` compile) that proves it. |

Order of preference for any change (Brave's rule, and it holds up):

1. **Our own code** using an existing extension point (no upstream patch at all).
2. **Subclass + one factory line** (upstream keeps working, we override virtuals).
3. **A one-line hook call** into `mast::` from an upstream function.
4. A real patch, only when 1–3 are impossible, and kept to one hunk.

Chromium is deterministic. When something surprises us, the answer is in a file we haven't
read yet. So the first move on any problem is to find and read that file.

## 1. The extension points we have (verified)

| Point | Where | Use it for |
|---|---|---|
| `EmbedderBrowserWindowFeatures` | `chrome/browser/ui/browser_window/internal/embedder_browser_window_features.cc`. Four empty stubs: `Init`, `InitPostWindowConstruction`, `InitPostBrowserViewConstruction`, `TearDownPreBrowserWindowDestruction`, each saying "Embedder implementation goes here". | Per-window Mast controllers: toolbar customisation, one-row layout switch, block state in the window. One patch file replaces the stub bodies with calls into `mast::`. |
| `BrowserViewLayout::CreateLayout()` | `chrome/browser/ui/views/frame/layout/browser_view_layout.cc`. Picks `BrowserViewTabbedLayoutImpl` / `AppLayoutImpl` / `PopupLayoutImpl`. Layout methods are virtual. | The one-row layout as a subclass (§3.C). |
| `SuggestedSaveLocationProvider` | `components/power_bookmarks`, registered via `BookmarkClientBase::AddSuggestedSaveLocationProvider` (Chrome's shopping collections use it). | The star saves to the Desktop folder (§3.A). |
| `FaviconSource` | `content::URLDataSource::Add(profile, make_unique<FaviconSource>(profile, kFavicon2))`. Local favicon DB only, unless `allowGoogleServerFallback=1` (we never pass it). | Real icons on the desktop NTP. No CSP change for chrome:// pages. |
| `NavigationThrottle` registration | already used (`mast-hooks.patch`). | Blocking. |
| `WebUIConfig` registration | already used. | NTP, block page. |
| `ChromeBrowserFieldTrials::RegisterFeatureOverrides` | one hook gives us a single Mast list of feature defaults. | Turning upstream features on/off in one place instead of scattered patches. (`fieldtrial_testing_config` is disabled by ungoogled, so this is the way.) |

## 2. Release strategy: ride ungoogled-chromium's tags

Chromium now ships a major **every 2 weeks** (verified on chromiumdash: 152 on Aug 25, then
153 Sep 8, 154 Sep 22, 155 Oct 6, 156 Oct 20, 157 Nov 3).

ungoogled-chromium follows **Stable**, not Extended: about three tags per major (156.0.8078.47,
.73, .79), then the next major (157.0.8103.38-1 is already out). It doesn't tag Extended-only
point releases (no 156.0.8078.88). Verified on the GitHub tags list.

- **While testing:** stay on 156. The $5 budget covers test builds only.
- **After launch:** follow ungoogled's tags, which means the current Stable with their patches
  already rebased. Cost isn't the constraint then; rebase *time* is. That's why the hook rules
  in §0 matter more than the build bill.
- **Each rebase:**
  1. Free check **before renting the server**: run ungoogled's `devutils/validate_patches.py -r`
     with our series appended against the new tag. It fetches only the files the patches touch.
  2. On the server, compile just the hooked files first (`autoninja -C out/Mast ../../path/file.cc^`).
  3. Only then the full build.
- Keep `docs/REBASE-LOG.md`: per release, which patch broke, why, and how long it took. After
  three releases this tells us which hooks to redesign.
- Overlap to watch: ungoogled's core patches also edit `browser_prefs.cc` (`disable-ai`,
  `remove-unused-preferences-fields`) and `settings_ui.cc` (2 hunks in
  `remove-unused-preferences-fields`). Our hooks there must sit away from their hunks. Our other
  planned hook files aren't touched by any ungoogled patch we apply (checked against the 110
  patches of 156.0.8078.79-1; we apply only one "extra" patch).

## 3. The work, solved

### A. Desktop new tab page (saved sites as apps + widgets) · easy

Design: screen **C7** in the Pencil file.

- **Storage = a bookmarks folder** "Desktop" under Other bookmarks, found by meta-info key
  `mast_desktop=1`, not by its title (users can rename it). Gets us for free:
  - the star lights up for saved pages;
  - the bookmark manager edits them;
  - Chrome import brings bookmarks in;
  - **favicons of bookmarked pages survive history expiry** (`ChromeHistoryBackendClient::IsPinnedURL`);
  - sync later.
- API: `BookmarkModelFactory::GetForBrowserContext(profile)` (wait for `loaded()`), then:
  - `AddFolder` / `AddNewURL` / `Move` / `Remove` / `SetNodeMetaInfo` to edit;
  - a `BookmarkModelObserver` that pushes changes to the page with `FireWebUIListener`.
- **Star → Desktop:** a `MastDesktopSaveLocationProvider` registered in `ChromeBookmarkClient::Init`
  (~3 lines in `chrome_bookmark_client.{h,cc}`). `GetBackoffTime()` must return zero, otherwise
  moving one bookmark out of Desktop silently disables the folder for a while.
- **Favicons:** desktop Chromium stores only 16/32 px icons (the large-icon handler is
  Android-only). So the tile is 64 px with a **32 px favicon centred**, and a monogram fallback:
  `chrome://favicon2/?size=32&scaleFactor=2x&showFallbackMonogram=&pageUrl=…`.
  The Pencil tiles should be updated to match.
- **Widgets:** plain HTML in `start.html`. Widget layout goes in a profile pref (`mast.desktop.widgets`).
  The Distractions widget calls the existing `MastBlocks`. It only needs a stored set of
  "distraction" categories.
- **Page ↔ browser:** keep `chrome.send` / `WebUIMessageHandler`. It's deprecated for *new upstream*
  pages, but still fully supported. Mojo adds mojom + BUILD files for no user benefit.
- Upstream patches: `chrome_bookmark_client.{h,cc}` (~3 lines). Everything else is in `src/mast`.
- Check:
  - screenshot of the NTP with real favicons;
  - ☆ on a page → it appears on the desktop;
  - delete in the bookmark manager → it disappears from the desktop.

### B. "Block this site" in the page-info bubble · removed 2026-11-09

- Removed: few would find it in the page-info panel. The bubble (`mast/browser/ui/mast_block_bubble.cc`) is parked, not compiled; a better entry point is still to be found.

Replaces the toolbar Block pill (now removed in the design).

- `PageInfoMainView`'s constructor adds `RichHoverButton` rows to `site_settings_view_`. There's no
  delegate for extra rows, so it's a ~5-line hook calling `mast::CreateBlockSiteRow(site_url,
  profile, close_callback)`. The row and its duration bubble live in
  `mast/browser/ui/mast_page_info_row.cc`.
- chrome:// pages get `InternalPageInfoBubbleView`, so the row never shows there, which is correct.
- **Removing the toolbar pill deletes our `toolbar_view.cc` hook.** That's a net reduction in patches.
- Check:
  - click the 🔒/ⓘ icon → "Block this site" → the duration bubble → the block page;
  - screenshot of each step.

### C. One-row window (tabs · ← address → · menu) · medium, no longer hard

Design: screens **C1–C7**. Key finding: **upstream already has both halves.**

1. **Tabs and toolbar in one row already exist for tabbed web apps.**
   `BrowserViewAppLayoutImpl::CalculateTitlebarLayout()` builds one titlebar rect, puts the toolbar
   at its preferred width on the right, and the horizontal tab strip in the rest (read directly).
2. **"Toolbar is the titlebar" already exists for vertical tabs** (`TabStripType::kVertical`):
   - the toolbar goes into the caption-button exclusion area ("If tabstrip is completely absent
     (or vertical), this can go in the top exclusion area");
   - `ToolbarView::IsPositionInWindowCaption()` makes empty toolbar space drag the window;
   - `GetFrameElementInfo()` reports the toolbar height so every platform's frame sizes the
     titlebar from it.
3. **Caption buttons are already abstracted.** Every frame reports them as
   `leading_exclusion`/`trailing_exclusion` in `BrowserLayoutParams` (Windows caption container,
   Linux opaque frame, macOS traffic-light rect). Layouts place views around them.

Plan:

- `mast/browser/ui/mast_one_row_layout.{h,cc}`: `MastOneRowLayout : BrowserViewTabbedLayoutImpl`.
  It overrides the top-container calculation using the web-app titlebar recipe: one 44 px row
  between the exclusions, toolbar at preferred width on the right, tab strip in the remainder.
  It never calls `SetTop()` after the tab strip.
  - Upstream: 1 line in `BrowserViewLayout::CreateLayout()` (`if (mast::UseOneRow()) return
    make_unique<MastOneRowLayout>(...)`).
- `browser_view.cc`, two small hooks:
  - `GetFrameElementInfo()` reports tab strip height 0 and toolbar height 44 in one-row mode, the
    same as vertical tabs does;
  - `NonClientHitTest()` runs the existing `IsPositionInWindowCaption` check in the horizontal
    branch too, so gaps in the row drag the window.
- **Toolbar arrangement without patching `toolbar_view.cc`** (order verified):
  - `browser.cc` creates the window, and `browser_window_factory.cc` runs
    `InitBrowserWidget()`. That adds the view to the widget, and `BrowserView::AddedToWidget()`
    calls `toolbar_->Init()` (`browser_view.cc:5031`).
  - Only then does `browser.cc:331` call `InitPostWindowConstruction`.
  - So `EmbedderBrowserWindowFeatures::InitPostWindowConstruction` sees a fully built toolbar
    and can:
    - reorder its children (forward after the omnibox);
    - give the location bar a fixed 228 px `kFlexBehaviorKey`;
    - hide extensions/profile buttons.
  - `InitPostBrowserViewConstruction` is too early: it runs inside BrowserView's constructor
    (`browser_view.cc:1064`), before the toolbar exists.
  - *(inferred)* Watch for upstream code re-adding toolbar children later, e.g. on theme
    change. Check by toggling the theme in a test.
- **Omnibox dropdown wider than the 228 px bar:** a hook in
  `OmniboxPopupPresenterBase::SynchronizePopupBounds()` (WebUI popup is the default in M156).
  Sets `max(location bar width, 560)`, clamped to the window, aligned so it doesn't leave the
  screen.
  - *Risk:* the location-bar "cutout" assumes the popup is exactly as wide as the bar
    (`RoundedOmniboxResultsFrame::Layout()`, and the WebUI page draws its own). Simplest
    first answer: drop the cutout and use a detached dropdown 6 px below the bar. That
    matches the design and avoids the problem.
- **Tab shape:** tabs are drawn to merge into the toolbar below (`kTabstripToolbarOverlap`, the
  swoop corners). In one row there is no toolbar below, so we want the pill tabs from the design.
  That's `TabStyle`/layout constants, which we already override for the Rams theme.
- **Order of work** (Linux first, because the opaque frame is the simplest):
  1. Layout subclass behind a pref `mast.one_row`, so we can flip back without rebuilding.
  2. Hit testing.
  3. Toolbar arrangement.
  4. Tab style.
  5. Dropdown.
  6. macOS traffic-light y (hard-coded rect in `browser_frame_view_mac.mm`) and Windows when we
     build those.
- Check every step with the built-in `--show-browser-frame-regions` switch (draws the exclusion
  areas) plus screenshots, and click-to-drag tests in the gaps.
- *Known edges to test:*
  - immersive fullscreen (the tab strip moves into `top_container`);
  - side panel height;
  - Windows `GetTopInset()`, which reads `GetTabStripVisible()`;
  - many tabs (overflow in a narrower strip).

### D. Settings without Google · easy, do with A/B

- C++: one hook at the end of the `SettingsUI` constructor (`settings_ui.cc`) re-sets loadTimeData
  booleans: `showAiPage`, `showGlicSettings`, `signinAllowed`, `showPrivacyGuide`, …
- TS: ~3 lines in `page_visibility.ts` so `createPageVisibility()` hides people/AI/safety-hub.
  The menu and router already obey `pageVisibility`.
- Decision needed: hiding the whole "You and Google" page also hides "Customize profile" and
  "Import bookmarks". Import should move to our own onboarding (`IMPORT-PLAN.md`).

### E. Remove the tab-search button (the ⌄ at the left of the tab strip) · trivial

- It's the end button of `TabStripComboButton`, which `HorizontalTabStripRegionView` creates
  for every normal window (`horizontal_tab_strip_region_view.cc:173`, and `:781` in the "New"
  variant).
- Its visibility already follows a profile pref: `TabStripComboButton::UpdateButtonsVisibility()`
  shows it only while `prefs::kTabSearchPinnedToTabstrip` is true (or while it's shown
  ephemerally for the shortcut). The default `true` is registered in
  `chrome/browser/ui/tabs/tab_strip_prefs.cc:23`.
- So **no UI code changes.** Add `mast::RegisterProfilePrefs(registry)` as the last line of
  `RegisterProfilePrefs()` in `browser_prefs.cc` (it ends at line 2386, after
  `tabs::RegisterProfilePrefs` at 2311). It calls
  `registry->SetDefaultPrefValue(prefs::kTabSearchPinnedToTabstrip, base::Value(false))`.
  - That's one more line in a file we already hook.
  - The same function registers Mast's own profile prefs (desktop widgets, §3.A).
- Ctrl+Shift+A still opens tab search (the button shows briefly while it's open). Profiles where
  the user explicitly pinned it keep it, which is correct.
- Check: fresh profile → no ⌄ button; screenshot.

### F. Mast icon on New Tab tabs · trivial

- A tab's icon is the page's favicon, or the default globe when there is none
  (`TabUIHelper::GetFavicon()`, `tab_ui_helper.cc:199`).
- `chrome://` pages get theirs from `ChromeWebUIControllerFactory::GetFaviconResourceBytes()`.
  For the `newtab` host (ours) it returns `NewTabPageUI::GetFaviconResourceBytes()` =
  `IDR_NTP_FAVICON`.
- That resource is `chrome/app/theme/default_{100,200}_percent/common/favicon_ntp.png` (16×16
  and 32×32, `theme_resources.grd:122`). Today it's the Chromium logo glyph.
- So **no patch:** add Mast-logo PNGs at those two paths under `src/overrides/`, the same
  mechanism as our product logos. `rsync --checksum` keeps rebuilds incremental.
- *Check:* chrome:// favicons may be themified (recoloured for contrast,
  `ShouldThemifyFaviconForEntry`). Look at the result on light and dark tabs. If the ember
  gradient gets flattened, use a mark that survives it.
- Related, *(unverified)*: `chrome://mast` and the block page show the globe. Try a
  `<link rel="icon">` with a `data:` URI in their HTML first (no C++). If that's ignored, it's 2
  lines in `GetFaviconResourceBytes()` for our host.

### G. Native-UI polish (see NATIVE-UI.md)

The colour-only restyle left Chrome's shapes, rings, button styles and font. `NATIVE-UI.md` has
the CSS-to-Views map, the inspector workbench (`--enable-ui-devtools`), and ten fixes, smallest
first (4 are colour mixer only; the rest are 1–3 line hooks).

### I. Customize: wallpaper and colours (design C13)

- A Customize button on the new tab page opens a panel:
  - **Background:** none, or an uploaded image (Replace once one is set). No presets: the
    design's wallpapers are third-party art and can't ship.
  - **Colours:** onyx (default), graphite, ink, espresso, moss. Ember never changes.
- **Style:** the wallpaper is clear, with no frosted glass (dropped 2026-11-06, cross-platform
  risk). A light dark wash keeps text legible; widgets and tiles are plain dark tints.
- **Step 1 (built):**
  - The page scales the image down (2560 px on the long side, JPEG) and sends it with
    `chrome.send('setWallpaper')`.
  - The handler writes `Mast Wallpaper.jpg` in the profile folder (thread pool, not the UI
    thread). It bumps the version pref `mast.ntp.wallpaper`: 0 is none, negative is cleared,
    and versions are never reused, so no cached image comes back.
  - The page's own data source serves the file as `wallpaper-<version>.jpg`.
  - Palettes switch the native UI too. `mast_palette.h` holds the palettes and the current
    one; the Chrome mixer reads `palette::Current()`. `mast::ApplyPalette()` resets the
    ColorProvider cache and sends `NativeTheme::NotifyOnNativeThemeUpdated()`, the path an
    OS dark/light switch takes.
  - The pick is the profile pref `mast.theme.palette`, applied when a window opens. Mast's
    pages are served with the palette's tokens and update live.
- **The tab row (built):** the image continues behind it, on websites too (C12).
  - `mast_wallpaper.cc` cover-scales the image to the screen with the page's wash. It hooks
    the two places Chromium paints a theme frame image: `BrowserFrameView::GetFrameImage`
    (every platform's frame) and `ThemedBackground` (the toolbar, which is in the row).
  - The new tab page lays out the same picture from the same origin (`wallpaperTop`), so
    the two meet without a seam. The 1px separator above the page is off in one-row.
  - Verified on Linux (Xvfb); Windows and macOS use the same frame-image path, but neither
    has been tested.
- **Settings (built):** it loads `colors.css` always (Chromium gated it behind the 2026
  redesign flags). Its page, cards and selected menu item follow the palette.
- **Tab bar on websites (built, design C23):** Picture (the image everywhere), Theme (the
  image on the new tab only) or Match site (each website's own colour). Pref `mast.tab_bar`.
  - Modelled on how Chromium recolours an installed web app's window from its
    theme-color (`AppBrowserController::UpdateThemeSupplier`). Each one-row window has a
    `MastTabBar` (`mast/browser/ui/mast_tab_bar.cc`). It watches the active tab and
    publishes the row per window (`mast/common/mast_tab_bar_state.h`).
    `BrowserWidget::GetColorProviderKey` puts that colour in the key's
    `app_background_color`, which only app windows set and no mixer reads. The mixer then
    builds the row from it, and the frame skips the picture.
  - The window stays in dark mode: pages take their light/dark look from the window, so a
    light row must not turn the site light (that would feed back into the colour).
  - The colour comes from the page's top 2px, copied like hover-card thumbnails and taken
    as the OKLab median. A strip that isn't one colour (picture headers) falls back to the
    palette. So do colours that are too loud (chroma above 0.14), near ember, or that no
    text colour reads on at 4.5:1. theme-color stands in until the first strip arrives.
  - A tab keeps its colour across switches and navigations until the new page's colour is
    known. Re-sampled every 2s; a change is adopted only once it's seen twice in a row.
  - Rows are built in OKLCH (`mast/ui/color/mast_site_color.h`): the same hue and chroma,
    lightness steps for the pill and the field. A light row also makes the address
    field's suggestions light.
  - Chromium keeps every ColorProvider it builds. The manager keeps the 24 newest Mast row
    colours and drops older ones once no window shows them.
  - Later: fade between colours (Chromium switches instantly), and check thresholds on
    more sites.
- **Step 2:**
  - Paper, the light palette, needs light-mode mixers.
  - The palette is process-wide for now; make it per profile.

### J. YouTube, music only (built, design C24)

- **The choice:** YouTube's row in the blocklist offers "Whole site" or "Music only". Music
  only is a block entry (`music:youtube.com`) with the usual rules: a duration, lengthened
  but never shortened, a 10-second undo. A whole-site block on youtube.com wins over it.
- **Where it runs:** `mast_youtube.cc` (a tab helper the navigation throttle attaches)
  runs `resources/youtube_music.js` in Mast's own isolated world on every YouTube page
  while the block is in force. That's where an extension's content script would run, but
  it's built in and can't be switched off. A built-in extension was considered, but it
  needs a private API (a permission enum and function histogram values, both widely
  included) plus a background relay just to learn whether the block is on.
- **What counts as music** (checked on live YouTube, 2026-11-08):
  - **Lists:** the item's link starts a Mix (`list=RD…`), or its duration badge carries
    the ♪ icon (non-music badges are plain text), or the badge says "Mix". Everything else
    is hidden. Undecided items wait invisible so their badges still load.
  - **Watch pages:** `player-microformat-renderer`'s JSON for that exact video (`@id`)
    says `genre: "Music"`, or it plays from a Mix. It's re-checked on YouTube's in-page
    navigation (`yt-navigate-finish`, `yt-page-data-updated`). While checking, the
    player is hidden and paused. A video that isn't music, or no answer within 6s, reloads
    with `mast_music_only=1`, and the throttle shows the blocked page ("Music only until
    …").
  - **Shorts:** shelves, the Shorts chip and the Shorts guide entry are hidden, and any
    `/shorts/` address is blocked by the throttle.
  - **music.youtube.com** stays open.
- **Known limits:** live streams have no ♪ badge, so they're hidden. Some music videos
  aren't categorised as Music by their uploaders. YouTube's markup changes, so the
  selectors will need upkeep.

### H. Leftovers from the de-Google pass

- "Your Mast" in the app menu (string). Warm greys in settings (WebUI colour tokens, `DESIGN-PLAN.md` §F).
- Our own time server for `NetworkTimeTracker` (blocks must survive clock changes).
- Adult and gambling feeds: built (both always on, outside the blocklist; one line in the panel says so). Still to do: refresh them through an update channel between releases.

## 4. Budget (testing only)

The $5 covers test builds before launch, nothing after. $3.27 is spent, and `./mast` stops
starting servers past 90%, so **$1.23 is usable**. The snapshot costs ~$0.39/month.

| Session | Contents | Estimate |
|---|---|---|
| 1 | A + B + D + E + F, remove the toolbar pill, and the native-UI fixes in `NATIVE-UI.md` §5 (all incremental, one session; start UI DevTools first and dump the views tree before changing anything) | ~2 h ≈ **$0.49** |
| 2–3 | C, the one-row window (iterative layout work, screenshots) | ~2–3 h ≈ **$0.50–0.75** |

Both fit. Rebasing onto newer Chromium waits until after launch (§2).

## 5. Resolved unknowns (read 2026-11-06)

1. **Toolbar init order:** `toolbar_->Init()` runs in `BrowserView::AddedToWidget()`, during
   `InitBrowserWidget()`, before `InitPostWindowConstruction`. So the toolbar needs **no
   patch**: rearrange it from `EmbedderBrowserWindowFeatures::InitPostWindowConstruction`
   (§3.C).
2. **ungoogled overlap:** none for `page_info_main_view.cc`, `chrome_bookmark_client.{h,cc}`,
   `page_visibility.ts`, `tab_strip_prefs.cc`, `embedder_browser_window_features.cc`,
   `browser_view_layout.cc`, the omnibox presenter or `chrome_web_ui_controller_factory.cc`.
   Overlap with `settings_ui.cc` (2 core hunks) and `browser_prefs.cc` (already hooked fine).
   `browser_view.cc` and `toolbar_view.cc` are only touched by "extra" patches we don't apply.
3. **GN cycles:** keep `//mast/browser` standalone (it depends only on small targets today).
   UI code that needs Chromium internals is compiled *inside* the owning upstream target, as
   `mast_toolbar_button.cc` already is in `ui/views/toolbar/BUILD.gn`. The owning targets are
   small, separate source sets:
   - `browser_view_tabbed_layout_impl` (`ui/views/frame/BUILD.gn:2433`);
   - `ui/views/page_info/BUILD.gn` (`page_info_main_view.cc`);
   - `bookmarks/BUILD.gn` (`chrome_bookmark_client.cc`).

   Each is one sources line.
4. **ungoogled tags:** Stable only, about three point releases per major. No Extended-only tags
   (§2).

## 6. Patch count after all of this

Removing the toolbar pill deletes the `toolbar_view.cc` hook and its BUILD.gn lines. Adding
A–F adds hooks in:
- `chrome_bookmark_client.{h,cc}`;
- `page_info_main_view.cc`;
- `settings_ui.cc` and `page_visibility.ts`;
- `embedder_browser_window_features.cc`;
- `browser_view_layout.cc` and `browser_view.cc`;
- the omnibox presenter;
- one more line in `browser_prefs.cc`;
- BUILD.gn source lines.

Each is 1–5 lines. Everything else (tab search, tab icon, favicons, widgets) is a pref default,
an override file, or our own code.
