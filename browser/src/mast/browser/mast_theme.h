// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_THEME_H_
#define MAST_BROWSER_MAST_THEME_H_

#include <string_view>

#include "mast/ui/color/mast_palette.h"
#include "ui/color/color_provider_manager.h"
#include "ui/native_theme/native_theme.h"

namespace mast {

// Switches the browser to a palette (mast_palette.h) and repaints it: every
// ColorProvider is rebuilt with the new colours (the mixers read
// palette::Current()), then windows and pages hear a theme change, the same
// path an OS dark/light switch takes. A no-op if the palette is already on.
inline void ApplyPalette(std::string_view id) {
  if (!palette::SetCurrent(id)) {
    return;
  }
  ui::ColorProviderManager::Get().ResetColorProviderCache();
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyOnNativeThemeUpdated();
}

}  // namespace mast

#endif  // MAST_BROWSER_MAST_THEME_H_
