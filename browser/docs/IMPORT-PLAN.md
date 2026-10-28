# Switching from Chrome to Mast in one click

Research, 2026-10-29 (upstream Chromium, brave-core, Helium, Firefox, Flathub).

## What exists today

| | History | Bookmarks | Passwords | Cookies (stay signed in) | Tabs | Extensions |
|---|---|---|---|---|---|---|
| Chromium's own importer (Linux) | Firefox only; **no Chrome source** | | | "not supported yet" | | |
| Brave's Chrome importer | ✓ | ✓ | ✗ (disabled for Chrome) | ✗ (removed) | ✗ | ✓ (reinstalls from the Web Store) |
| Helium (port of Brave's) | ✓ | ✓ | ✗ | ✗ | ✗ | ✓ |
| Firefox importing Chrome on Linux | ✓ | ✓ | ✗ | ✗ | | |
| **Mast (plan)** | ✓ | ✓ | **✓** | **✓** | **✓** | **✓** |

Why Mast can do better on Linux:
- Chrome encrypts passwords and cookies with a key kept in the system keyring under the name
  "Chrome Safe Storage".
- Chromium 156's own key code (`os_crypt_async`'s `FreedesktopSecretKeyProvider`) can read
  that key if the name is passed in instead of hard-coded. That's one small patch.
- Linux has no equivalent of Windows' app-bound encryption, so nothing ties the key to Chrome
  itself.

## How it works: "migrate and relaunch"

Chrome and Mast are both Chromium, so most of a Chrome profile is already in Mast's format.
Copy it and re-encrypt the few encrypted parts, instead of rebuilding everything through the
old importer:

1. **Detect:**
   - Chrome profiles in `~/.config/google-chrome{,-beta,-unstable}` (also nixpkgs Chrome) and
     Flatpak `~/.var/app/com.google.Chrome/config/google-chrome`
   - names and avatars from `Local State`
   - the last-used profile is preselected
2. **Check Chrome is closed.** Its `SingletonLock` must not point at a live process.
   Otherwise offer "Close Chrome to continue" or "Quit Chrome for me".
3. **Copy as-is:**
   - History, Favicons, Top Sites, Bookmarks
   - Sessions (open tabs)
   - Extensions and their settings
   - Local Storage, IndexedDB, Session Storage, Service Workers (many sites keep logins here,
     not in cookies)
   - Preferences, with Google sign-in and sync keys stripped
4. **Re-encrypt** Login Data (passwords), Network/Cookies (sign-ins) and Web Data
   (autofill, cards):
   - decrypt with Chrome's key (read-only; never create a key)
   - re-encrypt with Mast's
   - cookie values keep their `SHA256(host)` prefix
5. **Relaunch** into the migrated profile, then show a summary with counts per type and
   anything that failed.

**Guards:**
- Refuse if Chrome's database versions are newer than Mast's, and fall back to
  history-and-bookmarks only.
- Copy `-journal` and `-wal` files along with each database.
- Explain the keyring unlock prompt *before* it appears. Many NixOS and Hyprland setups have
  no keyring, so there's no prompt at all.

## Onboarding (first run, `chrome://mast/welcome` style)

1. **"Bring your Chrome with you."** A detected profile card with one **Import everything**
   button, and "Choose what" behind a disclosure. "Skip" is always visible.
2. **Progress:** one bar, with steps named in plain words ("Bookmarks… Passwords… Sign-ins…").
3. **Done:** "Your tabs, logins and bookmarks are here." Then two quick choices: **Set as
   default browser** and **Start your first focus session**.

The same import stays available later under Settings → Import from Chrome.

## Scope and order

1. **v1 (Linux):**
   - history, bookmarks, passwords, cookies, site storage, open tabs, extensions, autofill
     entries and addresses
   - cards optional
   - one profile at a time
2. **Later:**
   - import into an existing profile (merge, instead of a fresh copy)
   - Brave, Edge, Vivaldi and Arc sources (same Chromium format)
   - macOS and Windows (Windows needs a different approach because of app-bound encryption)

## Risks and unknowns

- **Google may refuse copied Google-account cookies** (session binding). Other sites are fine.
- **Flatpak Chrome** may use the portal key (v12), which can't be read from outside the
  sandbox. In that case warn, and import everything except passwords and cookies.
- **Chrome newer than Mast:** database version skew (the guard above).
- **Trust:** Mast can read every Chrome secret on the machine. Import only on an explicit click,
  never silently.

## Licensing

- Brave's importer files are MPL-2.0 (per-file copyleft). We'd borrow at most their
  browser-detection helpers and keep those files MPL.
- The re-encryption code is new and uses only Chromium's BSD code.

## Cost estimate

- About 600–900 lines in `src/mast/importer/` plus the welcome page.
- Two or three build rounds on the builder, about $0.25–0.50.
- Testing needs a real Chrome profile on the builder: install Chrome and gnome-keyring, and
  sign in to a test site.
