// Copyright 2026 The Mast Authors

#ifndef MAST_COMMON_MAST_TAB_BAR_STATE_H_
#define MAST_COMMON_MAST_TAB_BAR_STATE_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"

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

// How much of Customize's picture the row shows, 0 to 1 (1 if never set). A
// new tab page fades its picture in (desktop.html); when a new tab replaces a
// website's colour, the row holds its picture back and fades it in with the
// page's, so the two arrive as one (MastTabBar). Read by mast_wallpaper.cc.
float WallpaperOpacity(const void* browser_view);
void SetWallpaperOpacity(const void* browser_view, std::optional<float> opacity);

// The new tab page in `web_contents` started fading its picture in (sent by
// the page, mast_ui.cc). MastTabBar starts the row's fade on it.
void WallpaperRevealed(const void* web_contents);
base::CallbackListSubscription OnWallpaperRevealed(
    base::RepeatingCallback<void(const void*)> callback);

// Text typed (or pasted) on a new tab page while nothing on it was focused
// (desktop.html, mast_ui.cc). MastTabBar moves it into that window's address
// bar, which takes the focus, so typing anywhere on a new tab searches.
void NewTabTyped(const void* web_contents, const std::u16string& text);
base::CallbackListSubscription OnNewTabTyped(
    base::RepeatingCallback<void(const void*, const std::u16string&)> callback);

}  // namespace mast

#endif  // MAST_COMMON_MAST_TAB_BAR_STATE_H_
