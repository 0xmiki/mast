// Copyright 2026 The Mast Authors

#ifndef MAST_COMMON_MAST_TAB_BAR_STATE_H_
#define MAST_COMMON_MAST_TAB_BAR_STATE_H_

#include <cstdint>
#include <optional>

namespace mast {

// What a one-row window's tab row sits on right now (Customize: Tab bar on
// websites), set by its MastTabBar (mast/browser/ui/mast_tab_bar.h) as the
// active tab changes, and read where Chromium builds the window's colours
// (BrowserWidget::GetColorProviderKey) and paints the frame (mast_wallpaper.cc).
// Keyed by the window's BrowserView. An ARGB colour (SkColor):
//   - nothing: as before (Customize's picture if there is one, else the palette);
//   - transparent (0): the palette, no picture, e.g. a website in "Theme";
//   - opaque: the website's colour ("Match site").
// Base-only, so any upstream target can depend on it.
std::optional<uint32_t> TabBarColor(const void* browser_view);
void SetTabBarColor(const void* browser_view, std::optional<uint32_t> color);
// True if some window's row is `color` now. "Match site" makes a ColorProvider
// per website colour; the ones no window shows any more can be dropped
// (ui/color/color_provider_manager.cc, mast-theme.patch).
bool TabBarColorInUse(uint32_t color);

}  // namespace mast

#endif  // MAST_COMMON_MAST_TAB_BAR_STATE_H_
