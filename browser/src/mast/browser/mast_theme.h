// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_THEME_H_
#define MAST_BROWSER_MAST_THEME_H_

#include "components/prefs/pref_service.h"
#include "mast/browser/mast_prefs.h"
#include "mast/ui/color/mast_palette.h"
#include "ui/color/color_provider_manager.h"
#include "ui/native_theme/native_theme.h"

namespace mast {

// Repaints every window without changing colours (Customize's image changed;
// the tab row shows it, mast/browser/ui/mast_wallpaper.h). The same
// notification an OS dark/light switch sends.
inline void RepaintBrowser() {
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyOnNativeThemeUpdated();
}

// What the tab row sits on, from a profile's Customize prefs.
inline palette::Backdrop BackdropFromPrefs(const PrefService* prefs) {
  if (prefs->GetInteger(kWallpaperPref) <= 0) {
    return palette::Backdrop::kPalette;
  }
  return prefs->GetString(kWallpaperTonePref) == "light"
             ? palette::Backdrop::kLightImage
             : palette::Backdrop::kDarkImage;
}

// Applies a profile's palette and backdrop (mast_palette.h). If either changed,
// every ColorProvider is rebuilt (the mixers read them) and windows and pages
// repaint. Returns whether anything changed.
inline bool ApplyTheme(const PrefService* prefs) {
  const bool palette_changed =
      palette::SetCurrent(prefs->GetString(kPalettePref));
  const bool backdrop_changed =
      palette::SetBackdrop(BackdropFromPrefs(prefs));
  if (!palette_changed && !backdrop_changed) {
    return false;
  }
  ui::ColorProviderManager::Get().ResetColorProviderCache();
  RepaintBrowser();
  return true;
}

}  // namespace mast

#endif  // MAST_BROWSER_MAST_THEME_H_
