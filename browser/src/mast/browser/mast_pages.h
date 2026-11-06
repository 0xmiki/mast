// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_PAGES_H_
#define MAST_BROWSER_MAST_PAGES_H_

#include <string>
#include <string_view>


namespace mast {

struct BlockMatch;

// The block page shown in place of a blocked site.
std::string BlockPageHtml(std::string_view host, const BlockMatch& match);

// chrome://mast: the blocklist, every category and site.
// `as_new_tab` titles it "New Tab" so tabs and history read naturally.
std::string StartPageHtml(bool as_new_tab);

// The new tab page: saved sites as a desktop of icons, plus widgets (clock,
// and one button that blocks every distraction).
// `wallpaper` is the Customize image's version (kWallpaperPref; 0 is none).
// `wallpaper_top` is how far the image starts above the page (WallpaperTop()).
// `light_image` is the Customize tone (kWallpaperTonePref).
std::string DesktopPageHtml(int wallpaper, int wallpaper_top, bool light_image);

// The palette in use as CSS custom properties (":root{--bg:...}"), for
// pages already open when the palette changes.
std::string PaletteCss();

}  // namespace mast

#endif  // MAST_BROWSER_MAST_PAGES_H_
