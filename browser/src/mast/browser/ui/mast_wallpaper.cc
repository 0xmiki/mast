// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_wallpaper.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/prefs/pref_service.h"
#include "mast/browser/mast_prefs.h"
#include "mast/common/mast_features.h"
#include "mast/common/mast_tab_bar_state.h"
#include "mast/ui/color/mast_palette.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace mast {

namespace {

// How much of the image the row needs (DIP from the image's top): the row,
// with room for anything else Chromium paints with the theme image under it.
constexpr int kSliceHeight = 320;

// The page's wash over a dark image (rgb(0 0 0 / .22) in desktop.html); a
// light image has none, so it stays light.
constexpr SkAlpha kWash = 56;

struct Wallpaper {
  base::FilePath path;
  int version = 0;
  SkBitmap bitmap;     // decoded, full size; empty while loading
  gfx::Size screen;    // what `slice` was made for
  bool light = false;  // ... and for which tone
  gfx::ImageSkia slice;
};

Wallpaper& Current() {
  static base::NoDestructor<Wallpaper> wallpaper;
  return *wallpaper;
}

// The image's top slice, cover-scaled to the screen, drawn at each scale
// factor from the full-size bitmap (sharp on HiDPI), with the wash on top.
class SliceSource : public gfx::CanvasImageSource {
 public:
  SliceSource(gfx::ImageSkia image,
              gfx::Rect source,
              gfx::Size size,
              SkAlpha wash)
      : gfx::CanvasImageSource(size),
        image_(std::move(image)),
        source_(source),
        wash_(wash) {}

  void Draw(gfx::Canvas* canvas) override {
    canvas->DrawImageInt(image_, source_.x(), source_.y(), source_.width(),
                         source_.height(), 0, 0, size().width(),
                         size().height(), /*filter=*/true);
    if (wash_) {
      canvas->FillRect(gfx::Rect(size()), SkColorSetA(SK_ColorBLACK, wash_));
    }
  }

 private:
  const gfx::ImageSkia image_;
  const gfx::Rect source_;
  const SkAlpha wash_;
};

gfx::ImageSkia MakeSlice(const SkBitmap& bitmap,
                         const gfx::Size& screen,
                         bool light) {
  const double iw = bitmap.width(), ih = bitmap.height();
  const double sw = screen.width(), sh = screen.height();
  // CSS background-size: cover, centred (desktop.html does the same sums).
  const double k = std::max(sw / iw, sh / ih);
  const int height = std::min(kSliceHeight, screen.height());
  const gfx::Rect source(static_cast<int>((iw - sw / k) / 2),
                         static_cast<int>((ih - sh / k) / 2),
                         static_cast<int>(sw / k), static_cast<int>(height / k));
  return gfx::CanvasImageSource::MakeImageSkia<SliceSource>(
      gfx::ImageSkia::CreateFrom1xBitmap(bitmap), source,
      gfx::Size(screen.width(), height), light ? SkAlpha{0} : kWash);
}

SkBitmap ReadAndDecode(const base::FilePath& path) {
  std::string jpeg;
  if (!base::ReadFileToString(path, &jpeg)) {
    return SkBitmap();
  }
  // The page re-encoded the user's image as a JPEG before saving it.
  return gfx::JPEGCodec::Decode(base::as_byte_span(jpeg));
}

void OnDecoded(const base::FilePath& path, int version, SkBitmap bitmap) {
  Wallpaper& w = Current();
  if (w.path != path || w.version != version || bitmap.drawsNothing()) {
    return;  // replaced while loading, or unreadable
  }
  w.bitmap = std::move(bitmap);
  w.slice = gfx::ImageSkia();
  // Repaint every window (the same notification a theme change sends).
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyOnNativeThemeUpdated();
}

}  // namespace

gfx::ImageSkia WallpaperFrameImage(const BrowserView* browser_view) {
  if (!IsOneRowWindow() || !browser_view || !browser_view->GetWidget()) {
    return gfx::ImageSkia();
  }
  // A website in "Theme" or "Match site" (Customize: Tab bar on websites):
  // the row is the palette or the site's colour (mast_tab_bar.h).
  if (TabBarColor(browser_view)) {
    return gfx::ImageSkia();
  }
  Profile* profile = browser_view->GetProfile();
  if (!profile || profile->IsOffTheRecord()) {
    return gfx::ImageSkia();
  }
  const int version = profile->GetPrefs()->GetInteger(kWallpaperPref);
  if (version <= 0) {
    return gfx::ImageSkia();
  }
  Wallpaper& w = Current();
  const base::FilePath path = profile->GetPath().AppendASCII(kWallpaperFile);
  if (w.path != path || w.version != version) {
    // A new image (or the first paint): load it off the UI thread.
    w = Wallpaper();
    w.path = path;
    w.version = version;
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(&ReadAndDecode, path),
        base::BindOnce(&OnDecoded, path, version));
    return gfx::ImageSkia();
  }
  if (w.bitmap.drawsNothing()) {
    return gfx::ImageSkia();  // still loading
  }
  // The picture covers the screen as seen from the image's origin: a window
  // as tall as the screen still has picture down to its bottom edge.
  gfx::Size screen =
      display::Screen::Get()
          ->GetDisplayNearestView(browser_view->GetWidget()->GetNativeView())
          .size();
  screen.Enlarge(0, OneRowContentTop() + ThemeProperties::kFrameHeightAboveTabs);
  if (screen.IsEmpty()) {
    return gfx::ImageSkia();
  }
  const bool light =
      palette::CurrentBackdrop() == palette::Backdrop::kLightImage;
  if (w.slice.isNull() || w.screen != screen || w.light != light) {
    w.screen = screen;
    w.light = light;
    w.slice = MakeSlice(w.bitmap, screen, light);
  }
  return w.slice;
}

bool PaintWallpaperBackground(gfx::Canvas* canvas,
                              const views::View* view,
                              const BrowserView* browser_view) {
  const gfx::ImageSkia image = WallpaperFrameImage(browser_view);
  if (image.isNull()) {
    return false;
  }
  // Chromium's alignment for theme images (ThemedBackground::
  // PaintThemeAlignedImage): the image's origin is kFrameHeightAboveTabs above
  // the BrowserView, where the frame paints it too.
  gfx::Point origin;
  views::View::ConvertPointToTarget(view, browser_view, &origin);
  origin.Offset(0, ThemeProperties::kFrameHeightAboveTabs);
  if (browser_view->theme_background_y_offset()) {
    origin.set_y(origin.y() + *browser_view->theme_background_y_offset());
  }
  canvas->TileImageInt(image, origin.x(), origin.y(), 0, 0, view->width(),
                       view->height(), 1.0f, SkTileMode::kRepeat,
                       SkTileMode::kMirror);
  return true;
}

}  // namespace mast
