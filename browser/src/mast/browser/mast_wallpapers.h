// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_WALLPAPERS_H_
#define MAST_BROWSER_MAST_WALLPAPERS_H_

#include <string_view>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

class Profile;

namespace mast {

// Customize's built-in pictures (design C26): NASA photos, public domain,
// shipped beside the browser in mast-wallpapers/ (wallpapers/CREDITS.md). A
// new profile starts on the first one.
struct WallpaperPreset {
  const char* id;
  const char* name;
  const char* file;
  const char* credit;
};

inline constexpr WallpaperPreset kWallpaperPresets[] = {
    {"sunrise", "Sunrise", "sunrise.jpg",
     "Sunrise over Earth, from the Space Station · NASA"},
    {"aurora", "Aurora", "aurora.jpg",
     "Aurora, from the Space Station · NASA"},
    {"cosmic-cliffs", "Cosmic Cliffs", "cosmic-cliffs.jpg",
     "Cosmic Cliffs, James Webb Space Telescope · NASA, ESA, CSA, STScI"},
};
inline constexpr char kDefaultWallpaperPreset[] = "sunrise";

// Null for an unknown id.
const WallpaperPreset* FindWallpaperPreset(std::string_view id);

// The preset's file beside the browser.
base::FilePath WallpaperPresetPath(const WallpaperPreset& preset);

// Makes the preset the profile's picture: the file is copied to the profile's
// wallpaper (off the UI thread), the picture gets a new version, its tone is
// dark, and kWallpaperPresetPref remembers which one. Everything that shows
// the picture (the new tab page, the tab row, the blocked page) follows.
// `done(ok)` runs on the UI thread.
void UseWallpaperPreset(Profile* profile,
                        std::string_view id,
                        base::OnceCallback<void(bool)> done);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_WALLPAPERS_H_
