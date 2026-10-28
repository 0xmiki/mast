# Wiring Mast into Chromium

Research as of 2026-10-28 (Chromium M156). Paths are relative to `chromium/src`; browse them at
`https://source.chromium.org/chromium/chromium/src/+/main:<path>`. Nothing was cloned for this research.

## TL;DR

- Every Mast feature maps onto an existing Chromium mechanism. The closest built-in analog is
  **Family Link (supervised users)**: it blocks navigations with a custom page, re-blocks tabs
  that are already open, and force-disables incognito from browser code. We copy its patterns.
- Keep Mast's code in its own directory (Brave-style), with ~20–40 small hooks into
  upstream. Upstream releases a new version every 4 weeks, and fewer hooks means cheaper
  rebases.
- **Start from vanilla Chromium, not Helium or ungoogled-chromium.** Helium carries 370
  patches (all GPL-3.0) and, like ungoogled-chromium, removes network time, which we want
  for clock-tamper resistance. Borrow their *tooling* (a quilt patch series) and pick
  individual patches only if needed.
- **The hard truth:** a browser can only lock itself. Another browser, a fresh
  `--user-data-dir`, or a reinstall all bypass it. Only a small admin-level **OS helper**
  closes those doors. The same helper can also lock stock Chrome/Edge/Brave via enterprise
  policies during a session.

## How the reference forks do it

| | Helium (imputnet) | Brave (brave-core) |
|---|---|---|
| Base | ungoogled-chromium lineage | vanilla Chromium |
| Feature code | Everything is a quilt patch: `patches/series` lists 370, sorted into `helium/core`, `helium/ui` and `helium/settings` | Overlay repo at `src/brave` plus `chromium_src/` overrides (`#define Foo Foo_ChromiumImpl` and wrap) plus small `patches/` |
| Repos | `helium` (shared patches) plus `helium-linux` / `-macos` / `-windows`, each pulling `helium` in as a submodule | `brave-core`, plus `brave-browser` for build orchestration |
| Rebase | `quilt push -a --refresh`; fix failures; `he validate` | `npm run init / sync / apply_patches / update_patches`; a dedicated upgrade process |
| Useful examples | `helium/core/onboarding-page.patch` (new WebUI plus prefs), `helium/ui/toolbar-button-prefs.patch`, `helium/ui/clean-new-tab-page.patch` | `BraveContentBrowserClient::CreateThrottlesForNavigation`, `BraveToolbarView`, `brave_web_ui_controller_factory.cc` (NTP override) |

- https://github.com/imputnet/helium/blob/main/patches/series
- https://github.com/brave/brave-core/blob/master/docs/patching_and_chromium_src.md

**Plan for Mast:** a vanilla Chromium checkout plus a `src/mast/` directory for our code, and
a short quilt series of hook patches, so we get Helium's lightweight tooling with Brave's
separation of our code from upstream.

## Feature → Chromium hook

| Mast feature | Hook | Model to copy |
|---|---|---|
| Block a site during a session | New `MastBlockNavigationThrottle`, added with one line in `CreateAndAddChromeThrottlesForNavigation()` (`chrome/browser/chrome_content_browser_client_navigation_throttles.cc`). Registration API: `ContentBrowserClient::CreateThrottlesForNavigation(NavigationThrottleRegistry&)`. | `supervised_user::ClassifyUrlNavigationThrottle` |
| Custom block page (site + countdown) | Cancel with `ThrottleCheckResult(CANCEL, net::ERR_BLOCKED_BY_CLIENT, html)`. The 3rd argument is our page's HTML (`content/public/browser/navigation_throttle.h`). | `SupervisedUserInterstitial` |
| Tabs already open switch to the block page | Per-tab observer that reloads tabs on the blocklist when a session starts, so the throttle catches them | `SupervisedUserNavigationObserver::OnUrlFilteringServiceChanged()` |
| Lock incognito / guest / add profile / DevTools | A `MastPrefStore` at the supervised-user layer of the pref stack. During a session it forces `incognito.mode_availability`, `kBrowserGuestModeEnabled`, `kBrowserAddPersonEnabled` and `DeveloperToolsAvailabilityBlocklist`. Settings rows then show as locked automatically. Also close incognito windows already open. | `SupervisedUserPrefStore` (already forces incognito off) |
| Lock "Reset settings" | Gate `chrome/browser/ui/webui/settings/reset_settings_handler.cc` | — |
| Session state survives clearing data / deleting a profile | Store it in **Local State** (browser-wide, outside every profile) | — |
| Clock changes don't end a session | Count down with `base::TimeTicks` (monotonic). Save "time remaining" checkpoints and never accept less than the last one. `network_time::NetworkTimeTracker` may only ever *extend* a session. | — |
| New tab page with Start + countdown | Point `chrome://newtab` at our own WebUI (Brave), or patch `chrome/browser/resources/new_tab_page/` (Helium) | Brave NTP, Helium `clean-new-tab-page.patch` |
| `mast://blocklist` page | A `DefaultWebUIConfig` subclass registered in `chrome_web_ui_configs.cc`, using mojo to talk to the browser | Helium `onboarding-page.patch` |
| Toolbar Block button + popover | A pinned toolbar action (`chrome_action_id.h`, `browser_actions.cc`, `pinned_toolbar_actions_model.cc`) that opens a WebUI bubble (`WebUIBubbleManager`), so the popover reuses the NTP's start control | Helium `toolbar-button-prefs.patch` |
| The session engine itself | A browser-wide `MastSessionService` started from a `ChromeBrowserMainExtraParts` | — |

Our existing Svelte UI (start control, block page, blocklist) is a good fit for the WebUI parts:
Chromium WebUIs are just HTML/TS bundles served from `chrome://`.

## The enterprise-policy route (and why we still fork)

Chromium already obeys admin policies: `URLBlocklist`, `BrowserGuestModeEnabled`,
`BrowserAddPersonEnabled`, `DeveloperToolsAvailability*` and `AllowDeletingBrowserHistory`.
They live in `/etc/chromium/policies/managed/` (Linux), `HKLM\SOFTWARE\Policies\…` (Windows)
and `/Library/Managed Preferences` (macOS). Changes on Linux and macOS are picked up live
(Linux after about 5 s). Windows may take up to its 15-minute reload. The exception is
`IncognitoModeAvailability`, which is `dynamic_refresh: false` and so needs a restart.

So an admin-level helper writing policies at session start could lock **any** Chromium browser,
including stock Chrome. What the fork adds on top:
- the custom block page with a countdown, instead of a generic "blocked by administrator" page;
- re-blocking tabs that are already open;
- the Start button built into the NTP and toolbar;
- locking incognito instantly;
- no "Managed by your organization" banner.

**The strongest product is both:** the fork for the experience, and the helper so other browsers
are blocked too during a session.

## OS helper (Mast Guard)

| OS | Mechanism | What it does |
|---|---|---|
| macOS | root `launchd` daemon | holds the session, writes managed policies for other browsers, optionally pf firewall/hosts. SelfControl works this way. |
| Windows | SYSTEM service | same, plus it can refuse to let the uninstaller run during a session |
| Linux | systemd unit | same, plus nftables/hosts |

A standard user can't stop it; an admin always can. That's the realistic ceiling, and the same
one every competitor (SelfControl, Cold Turkey, Freedom) hits.

## Building on AWS

| Target | Machine | Notes |
|---|---|---|
| Linux | `c7a.48xlarge` (192 vCPU), spot ≈ $3.2/h, 400–500 GB gp3 | Estimate ≈1–2 h per full build. Use Siso plus a remote-execution cache (BuildBuddy, EngFlow, or self-hosted Buildbarn) to cut rebuilds. |
| Windows | Native Windows EC2 (≥ 32 cores, 64 GB) | Cross-compiling from Linux works but breaks Crashpad (crash reports), which we want for a shipped product. |
| macOS | EC2 Mac dedicated host (`mac-m4pro.metal` ≈ $2/h) | Apple's licence forces a 24-hour minimum allocation. Requires Mac hardware. |

Development builds: set `is_component_build=true`, `symbol_level=0`. Official ThinLTO builds
need `vm.max_map_count=262144` on Linux.

## Suggested phases

1. **Linux-only proof:** stock Chromium plus `MastSessionService`, the navigation throttle,
   and the block page served as plain HTML. Start sessions from a temporary `chrome://mast`
   page. This proves enforcement.
2. **Locks:** `MastPrefStore` for incognito, guest, profiles and DevTools; reloading tabs
   already open; gating Reset settings.
3. **UI:** port the Svelte demo's start control, NTP and blocklist to WebUI; add the toolbar
   button and popover.
4. **Branding and packaging:** name and icons, Linux packages, then Windows and macOS builds.
5. **Mast Guard:** the OS helper (one OS first), including policies for other browsers.

## Risks

- **Rebasing every 4 weeks.** UI patches (NTP, toolbar, settings) break most often. Keep hooks small.
- **Escape routes outside the browser** (another browser, `--user-data-dir`, reinstalling)
  until Mast Guard exists.
- **Locking incognito at runtime and closing incognito windows already open** needs testing.
- **DRM sites:** Widevine needs a licence from Google for forks. Probably irrelevant to Mast.

## Not verified

- Real EC2 build times; the figures above are estimates.
- That clearing browsing data leaves every Local State key alone. Believed true, not audited line by line.
- How fast Windows applies raw registry policy writes without `gpupdate`.
