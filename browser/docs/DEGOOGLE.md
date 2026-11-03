# Removing Google from Mast (the ungoogled-chromium way)

Research as of 2026-11-03, against ungoogled-chromium tag `156.0.8078.79-1` (the same M156 as
[CHROMIUM.md](../../demo/docs/CHROMIUM.md)) and Helium `main`. Both repos were shallow-cloned and read; nothing
was built.

- https://github.com/ungoogled-software/ungoogled-chromium (BSD-3-Clause)
- https://github.com/imputnet/helium (GPL-3.0; built on ungoogled-chromium)

## TL;DR

- ungoogled-chromium removes Google with **four layers**. Three of them (patches, build flags,
  binary pruning) switch things off. The fourth, **domain substitution**, is a safety net: it
  rewrites Google domains in the source to a fake TLD, and a patched network stack refuses to
  connect to that TLD. Anything the patches missed fails closed.
- **Use it as a pinned dependency rather than copying it.** Pin ungoogled-chromium at the tag
  that matches our Chromium version, apply its `core` series, and put Mast's patches on top.
  68% of the core patch lines (≈12k of 17.6k) remove Safe Browsing. Its maintainers rebase that
  every release, so we shouldn't take it on ourselves.
- **One conflict with Mast:** ungoogled turns off `NetworkTimeTracker`, which we planned to use
  to resist clock changes. Fix: skip that one patch and point the tracker at a **Mast time
  server** signed with our own key (two constants in one file).
- **What ungoogling costs:** no component updates (CRLSet revocation lists, Widevine), no Chrome
  Web Store installs or updates, no spellcheck dictionaries, no translation, no Safe Browsing,
  no auto-update. Helium's answer is a **self-hosted services proxy** with an opt-out toggle,
  re-enabling those features one patch at a time. Mast will need the same for at least updates
  and CRLSets.
- **A trap for Mast specifically:** our blocklist is full of youtube.com and facebook.com.
  Domain substitution would rewrite them to `y0u1ub3.qjz9zk` and `f8c3b00k.qjz9zk`, and Mast
  would silently block nothing. Keep every domain string in `src/mast/` and exclude that
  directory from substitution.

## The four layers

### 1. Patches: switch off each Google service in code

`patches/series` is a GNU quilt series of 110 patches: 2 upstream build fixes plus two groups:

- **`core` (36):** background requests and Google-only code. Kept current with every release.
- **`extra` (72):** privacy defaults and `chrome://flags` toggles. Best effort; not needed for
  degoogling.

What `core` switches off:

| Area | Patches | How |
|---|---|---|
| Safe Browsing | `inox/0001-fix-building-without-safebrowsing`, `ungoogled/fix-building-without-safebrowsing` (127 files), `remove-unused-preferences-fields` (231 files), 2× `iridium/safe_browsing-*` | Built with `safe_browsing_mode=0`; the patches make the tree compile and drop the prefs |
| Sign-in / Gaia | `disable-gaia` | Stubs `GaiaAuthFetcher` (it ran even when signed out) |
| Push messaging | `disable-gcm` | Stubs the GCM client |
| Telemetry | `disable-crash-reporter`, `disable-domain-reliability`, `disable-webrtc-log-uploader`, `iridium/*incident-reporting*` | Uploaders return early |
| Field trials | `bromite/disable-fetching-field-trials` | No variations seed fetch |
| Google-specific behaviour | `disable-google-host-detection` (10 files) | `google_util` no longer recognises Google hosts, so no `X-Client-Data` header and no special-casing |
| Search | `replace-google-search-engine-with-nosearch`, `remove-navigation-source-param` | Default engine is "No Search"; strips a tracking parameter from omnibox searches |
| Network time | `disable-network-time-tracker` | `AreTimeFetchesEnabled()` returns `false` (**conflicts with Mast**, see below) |
| Updates | `inox/0015-disable-update-pings`, `block-requests` | `update_client`'s `CheckForUpdates()` returns immediately, which turns off **all** component and extension updates |
| Misc requests | `block-requests` | NTP One Google bar/backgrounds/promos, password affiliation, spellcheck dictionaries, translate ranker, DevTools CSS data |
| Web Store | `disable-webstore-urls`, `inox/0005-disable-default-extensions` | No CWS update URLs or default component extensions |
| AI | `disable-ai` (60 files) | Glic (Gemini in Chrome), ScreenAI, related components |
| Other | `inox/0003-disable-autofill-download-manager`, `inox/0021-disable-rlz`, `toggle-translation-via-switch`, `disable-profile-avatar-downloading`, `disable-privacy-sandbox`, `disable-fonts-googleapis-references`, `doh-changes` (DoH defaults to off), `disable-tos-dialog`, `disable-mei-preload` | — |

Their rule for writing these: delete code line by line rather than wrapping it in `#if 0` or
comments, so `quilt refresh` can't quietly absorb new upstream lines into a disabled block.

### 2. Build flags (`flags.gn`)

```gn
google_api_key=""  google_default_client_id=""  google_default_client_secret=""
use_official_google_api_keys=false
safe_browsing_mode=0
disable_fieldtrial_testing_config=true
enable_hangout_services_extension=false
enable_remoting=false  enable_reporting=false
enable_mdns=false  enable_service_discovery=false
```

With no API keys, the keyed Google APIs (sync, geolocation, translate, etc.) can't authenticate
even if some code still calls them.

### 3. Binary pruning (`pruning.list`)

Deletes about 14.1k prebuilt files from the source tree before building, so everything shipped
is built from source. Most are test data (BoringSSL, libphonenumber, HarfBuzz and `net/data`
make up half of them). `fix-building-with-prunned-binaries` removes the build targets that
needed them. This is about auditability rather than Google, so it's **optional for Mast**.

### 4. Domain substitution, the safety net

`domain_regex.list` holds 21 regexes that rewrite Google, gstatic, YouTube, DoubleClick,
Facebook, Microsoft and similar domains into look-alikes under the non-existent TLD `qjz9zk`
(`google.com` → `9oo91e.qjz9zk`). They're applied to the ≈17.9k source files listed in
`domain_substitution.list`. Then `block-trk-and-subdomains` adds a check in `URLRequest`: any
host ending in `qjz9zk`, and any URL with a `trk:` scheme, gets `ERR_BLOCKED_BY_CLIENT` plus a
`LOG(ERROR)`. Patches also mark known-dead URLs with a numbered `trk:NNN:` prefix, so a log
line points to the exact source site.

The result: if Google adds a new background request next release that no patch covers, it still
goes nowhere, and it shows up in the logs.

Substitution can't be cleanly undone, so the order matters: **prune → patch → substitute →
`gn gen` → build**. Patches are always written and refreshed against the *unsubstituted* tree.

## Tooling

| Tool | Purpose |
|---|---|
| `utils/downloads.py retrieve/unpack` | Fetch and unpack the Chromium source tarball for `chromium_version.txt` |
| `utils/prune_binaries.py`, `utils/patches.py apply`, `utils/domain_substitution.py apply` | The three tree-processing steps |
| `devutils/update_lists.py -t build/src` | Regenerates `pruning.list` and `domain_substitution.list` from a fresh tree. Takes `--domain-exclude-prefix` (we need it for `mast/`) |
| `devutils/set_quilt_vars.sh` + `quilt push -a --refresh` | The rebase loop |
| `devutils/validate_config.py`, `validate_patches.py` | Check that the series and lists are consistent and that every patch applies with no fuzz |

## How Helium gets functionality back

Helium carries most of ungoogled's `core` series (some renamed), but deliberately drops four:
`disable-crash-reporter` (it runs its own crash server, `crash.helium.computer`),
`replace-google-search-engine-with-nosearch`, `doh-changes` and `0005-disable-default-extensions`
(it ships uBlock Origin as a component). It then adds patches that send specific features through
**Helium services** instead of Google:

| Patch | Restores |
|---|---|
| `helium/core/services-prefs` | Settings page: on/off toggle plus a custom URL (default `https://services.helium.imput.net`). When off, everything points at `helium-services-are-disabled.qjz9zk` |
| `component-updates`, `reenable-update-checks` | Component updater (CRLSets, etc.) through the proxy |
| `proxy-extension-downloads`, `spoof-extension-downloader-platform` | Chrome Web Store installs and updates through the proxy |
| `reenable-spellcheck-downloads` | Hunspell dictionaries |
| `add-updater-preference`, `add-update-channel-flag` | Its own browser updates (`updates.helium.computer`) |

The shared code lives in a new `components/helium_services/`, which matches the
own-directory layout CHROMIUM.md plans for `src/mast/`.

## What this means for Mast

### Proposed layout

```
mast-browser/
  ungoogled-chromium/        # git submodule, pinned to the tag matching chromium_version.txt
  patches/series             # ../ungoogled-chromium core series, minus drop-list, then mast/*
  patches/mast/*.patch       # our ~20–40 hooks (CHROMIUM.md)
  patches/drop.list          # ungoogled patches we skip (start: network-time-tracker, crash-reporter, nosearch)
  src/mast/                  # our code; excluded from domain substitution
  flags.gn                   # ungoogled flags.gn + Mast branding
```

Pipeline: download → (optional) prune → ungoogled `core` minus the drop list → Mast patches →
domain substitution with `mast/` excluded → build. Skip ungoogled's `extra` series apart from
anything we choose deliberately. It is 72 UI flags we would otherwise have to rebase.

### Decisions

| Item | ungoogled default | Recommendation for Mast |
|---|---|---|
| Network time | Off | **Keep, but self-hosted.** Skip `disable-network-time-tracker`. Patch `kTimeServiceURL` and `kPubKey`/`kKeyVersion` in `components/network_time/network_time_tracker.cc` (currently `http://clients2.google.com/time/1/current` with an ML-DSA-44 key) to point at a Mast time server and Mast's key. Call `StartTimeFetch()` at session start. Exclude the new URL from substitution. |
| Browser auto-update | None (distro packages) | **Needed.** Own update server (Sparkle on macOS, own service on Windows, packages on Linux). Not Google's `chrome/updater`. |
| Crash reports | `disable-crash-reporter` | Drop that patch like Helium does and point Crashpad at a Mast crash server, with opt-in consent. A shipped product needs crash reports. |
| Component updates (CRLSets, CT log list) | Off | **Re-enable through a Mast proxy** the way Helium does. Without them, certificate revocation goes stale. |
| Safe Browsing | Removed | Accept the loss (it needs a Google API key anyway). Revisit if we want a proxy like Brave's. |
| Chrome Web Store | Off | Phase 2: proxy installs and updates like Helium, or ship without extensions at first. Mast's blocking doesn't depend on extensions. |
| Sign-in / sync | Off | Off. |
| Default search | "No Search" | Choose a non-Google default in `prepopulated_engines.json` (e.g. DuckDuckGo). "No Search" is a poor first run. |
| Widevine | `enable_widevine=true`, but the component updater is blocked, so no CDM | Irrelevant to Mast (as CHROMIUM.md already says). |
| Binary pruning | On | Optional. Adopt later for auditability. |

### Mast-specific gotchas

1. **The blocklist vs domain substitution.** The preset (`src/lib/mast/blocklist.ts` in the
   demo) contains youtube.com, facebook.com and other domains the regexes rewrite. If any copy
   of it ends up in a substituted file (a WebUI `.ts`, a `.json`, or a hook patch into an
   upstream `.cc`), Mast would block the `qjz9zk` look-alikes and let the real sites through.
   Rules:
   - All domain strings live under `src/mast/`.
   - Run `update_lists.py` with `--domain-exclude-prefix mast/`.
   - Add a CI check that greps the built resources for `qjz9zk` inside Mast code.
2. **The Mast time server URL and Mast update URLs** must also stay out of substitution. This
   only matters if they use a substituted domain (e.g. if we ever host on `appspot.com`).
3. **Block layers.** ungoogled blocks at `URLRequest` with `ERR_BLOCKED_BY_CLIENT`. Mast blocks
   at the `NavigationThrottle` with the same error code plus our HTML. They're independent, but
   the error code is shared: Mast's tab-reload logic should key off the throttle, not the error
   code.
4. **Licensing.** ungoogled-chromium is BSD-3-Clause, so it's compatible with any Mast license.
   The few Bromite-sourced patches (`core/bromite/disable-fetching-field-trials`, three in
   `extra`) come from a GPL-3.0 project. Check their terms, or rewrite that one core patch
   (about 76 lines), before shipping closed-source. Helium itself is GPL-3.0, so copy its
   *approach* to the services proxy rather than its patches unless Mast is GPL.

### Verifying it worked

Domain substitution makes leaks fail closed, but we should still prove there are none:

- Launch with `--log-net-log=/tmp/netlog.json` on a fresh profile, idle 10 minutes, open the
  NTP and settings, run a Mast session, then assert that no request goes to a host outside an
  allowlist (Mast services only). Make this a CI job.
- `grep 'Blocked URL in TrkProtocolHandler'` in the stderr log. Each hit is a feature that
  tried to reach Google and should probably be switched off properly.

## Updated phases (amends CHROMIUM.md)

0. **(new) Ungoogled base:** submodule plus pipeline, building a plain ungoogled Chromium on
   Linux with the netlog CI check. Do this first so all later Mast work is written against the
   tree we ship.
1. Linux proof (as before), plus **the network time patch and a stub Mast time server**.
2. Locks (as before).
3. UI (as before). WebUI resources stay under `mast/`.
4. Branding and packaging, plus the **Mast update server** and **component-update proxy**.
5. Mast Guard (as before).

## Not verified

- That `NetworkTimeTracker`'s signed request/response format can be served by our own server
  just by changing the URL and key. The client side is two constants. The server needs to
  reproduce the `client_update_protocol` signing; the format wasn't checked.
- How far ungoogled-chromium tags lag behind Chromium stable. This sets how quickly Mast can
  ship security updates.
- The Bromite patches' exact licence terms.
- That vanilla Chromium (not Chrome) still fetches the variations seed and component updates
  without API keys. ungoogled patches them anyway, but this affects how urgent the proxy is.

## Status in Mast (2026-11-03)

What's built:

- **Pins.** `chromium_version.txt` is `156.0.8078.79` and `ungoogled_version.txt` is
  `156.0.8078.79-1`.
  - The builder clones that tag; it isn't a submodule.
  - `remote/ungoogle.sh` builds the base tree: reset → `core` patches → domain substitution.
  - The base is committed in `src` as the commit Mast's patches are generated and applied
    against (`remote/make-patches.py` reads `git HEAD`).
- **`patches/ungoogled-drop.list`** skips `replace-google-search-engine-with-nosearch`, so
  Google stays the default search.
- **`patches/ungoogled-extra.list`** takes one `extra` patch,
  `fix-building-without-mdns-and-service-discovery`, which ungoogled's own `flags.gn` needs.
- **`mast-ungoogled.patch`** restores `SearchTermsData::GoogleBaseURLValue()` to
  `https://www.google.com/`. Substitution had rewritten `kGoogleHomepageURL`, which silently
  broke search and suggest. Lens / search-by-image stays substituted, so it's off.
- **Build flags.** `flags.gn` is merged under Mast's `remote/args.gn`; Mast wins on conflicts.
- **Guards.**
  - `mast/` and the blocklist are never substituted.
  - `build.sh` fails if `qjz9zk` shows up in Mast's code.
- **Network check.** `./mast shots` records a net log and writes `shots/<run>/network.txt`. It
  lists every host contacted and every request stopped by substitution.

Not done yet (deviations from the plan above):
- **Network time.** `disable-network-time-tracker` is still applied because there's no Mast
  time server yet, so clock-tamper resistance isn't implemented.
- **Proxies.** No component-update proxy (CRLSets) and no update server.
- **Pruning.** Binary pruning is skipped.
