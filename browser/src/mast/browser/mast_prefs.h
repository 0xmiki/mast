// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_PREFS_H_
#define MAST_BROWSER_MAST_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace mast {

// Which widgets the desktop new tab page shows, in order ("clock", "distractions").
inline constexpr char kDesktopWidgetsPref[] = "mast.desktop.widgets";

// Profile prefs: Mast's own, and Mast's defaults for some of Chromium's. Called
// last in RegisterProfilePrefs(), so every upstream pref is already registered.
// (Blocks live in Local State instead; see MastBlocks.)
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_PREFS_H_
