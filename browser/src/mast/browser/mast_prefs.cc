// Copyright 2026 The Mast Authors

#include "mast/browser/mast_prefs.h"

#include <string>
#include <utility>

#include "base/values.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace mast {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  // No tab-search button at the start of the tab strip. Ctrl+Shift+A still
  // opens tab search, and anyone who pins it again keeps it.
  registry->SetDefaultPrefValue(prefs::kTabSearchPinnedToTabstrip,
                                base::Value(false));

  // Start where you left off: tabs, windows and tab groups come back. (Chromium
  // opens a lone new tab page, which dropped every group on restart.)
  // Settings > On startup still changes it.
  registry->SetDefaultPrefValue(
      prefs::kRestoreOnStartup,
      base::Value(SessionStartupPref::kPrefValueLast));

  base::ListValue widgets;
  widgets.Append("clock");
  widgets.Append("distractions");
  registry->RegisterListPref(kDesktopWidgetsPref, std::move(widgets));
  registry->RegisterStringPref(kPalettePref, "onyx");
  registry->RegisterIntegerPref(kWallpaperPref, 0);
  registry->RegisterStringPref(kWallpaperTonePref, "dark");
  registry->RegisterStringPref(kTabBarPref, "site");
  registry->RegisterBooleanPref(kWelcomedPref, false);
  registry->RegisterStringPref(kWallpaperPresetPref, std::string());
  registry->RegisterListPref(kShortcutsPref);
}

}  // namespace mast
