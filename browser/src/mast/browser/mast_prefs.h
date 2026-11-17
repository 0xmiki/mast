// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_PREFS_H_
#define MAST_BROWSER_MAST_PREFS_H_

#include <string_view>

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace mast {

// Which widgets the desktop new tab page shows, in order ("clock", "distractions").
inline constexpr char kDesktopWidgetsPref[] = "mast.desktop.widgets";

// Customize (the new tab page): the palette id (mast_palette.h), and the
// wallpaper's version: 0 is none, otherwise "Mast Wallpaper.jpg" in the
// profile folder, served as chrome://new-tab-page/wallpaper-<version>.jpg (a
// new version for each upload, so the page never shows a cached old one).
inline constexpr char kPalettePref[] = "mast.theme.palette";
inline constexpr char kWallpaperPref[] = "mast.ntp.wallpaper";
inline constexpr char kWallpaperFile[] = "Mast Wallpaper.jpg";
// Whether the image is "dark" or "light": the tab row's text and icons, and the
// new tab page's tiles and widgets, follow it.
inline constexpr char kWallpaperTonePref[] = "mast.ntp.wallpaper_tone";
// What the one-row tab row shows on websites (Customize: Tab bar on websites):
// "site" (the default: each website's own colour), "picture" (Customize's
// picture, as on the new tab page) or "theme" (the palette). See
// mast/browser/ui/mast_tab_bar.h.
inline constexpr char kTabBarPref[] = "mast.tab_bar";
// Whether this profile has been shown the welcome page (mast://mast/welcome,
// design C25). It opens once, in the first window.
inline constexpr char kWelcomedPref[] = "mast.welcomed";
// Which built-in picture is the wallpaper (mast_wallpapers.h); empty for an
// uploaded one or none.
inline constexpr char kWallpaperPresetPref[] = "mast.ntp.wallpaper_preset";
// Alt + letter shortcuts (the blocklist panel's Shortcuts tab, design C32): a
// list of {"key": "N", "name": "NVIDIA stock", "url": "https://..."}. Each opens
// its page in a new tab, in any window (mast/browser/ui/mast_shortcuts.h).
inline constexpr char kShortcutsPref[] = "mast.shortcuts";

// A letter a shortcut may use: A to Z, except the ones Chromium has (Alt+D the
// address bar, Alt+E and Alt+F the menu; Alt+digits switch tabs). Upper case.
inline bool IsShortcutKey(std::string_view key) {
  return key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z' &&
         std::string_view("DEF").find(key[0]) == std::string_view::npos;
}

// Profile prefs: Mast's own, and Mast's defaults for some of Chromium's. Called
// last in RegisterProfilePrefs(), so every upstream pref is already registered.
// (Blocks live in Local State instead; see MastBlocks.)
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_PREFS_H_
