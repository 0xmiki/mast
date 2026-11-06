// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_WALLPAPER_H_
#define MAST_BROWSER_UI_MAST_WALLPAPER_H_

#include "ui/gfx/image/image_skia.h"

class BrowserView;

namespace gfx {
class Canvas;
}  // namespace gfx

namespace views {
class View;
}  // namespace views

namespace mast {

// Customize's image behind the one-row tab row, so the new tab page's
// wallpaper continues up to the top of the window (design C12).
//
// The image is laid out like a desktop wallpaper: scaled to cover the screen,
// centred, with the page's dark wash. Its top-left is Chromium's theme-image
// origin: kFrameHeightAboveTabs above the BrowserView. Two painters use it,
// the way Chromium paints a theme's frame image: the frame (behind the tabs
// and the window buttons) and the toolbar's ThemedBackground (the address bar's
// part of the row). desktop.html lays out the same picture under the row.

// The top slice of the image, or an empty image: no image set, not a one-row
// window, or still loading (the window repaints once it has loaded).
gfx::ImageSkia WallpaperFrameImage(const BrowserView* browser_view);

// For ThemedBackground: paints the image behind `view`, aligned with the
// frame's, and returns true; returns false if there is nothing to paint.
bool PaintWallpaperBackground(gfx::Canvas* canvas,
                              const views::View* view,
                              const BrowserView* browser_view);

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_WALLPAPER_H_
