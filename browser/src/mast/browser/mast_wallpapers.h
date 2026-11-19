// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_WALLPAPERS_H_
#define MAST_BROWSER_MAST_WALLPAPERS_H_

#include <string_view>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

class Profile;

namespace mast {

// Customize's built-in pictures (design C26): public-domain paintings,
// shipped beside the browser in mast-wallpapers/ (wallpapers/CREDITS.md). A
// new profile starts on the first one.
struct WallpaperPreset {
  const char* id;
  const char* name;
  const char* file;
  const char* credit;
};

inline constexpr WallpaperPreset kWallpaperPresets[] = {
    {"sierra-nevada", "Sierra Nevada", "sierra-nevada.jpg",
     "Among the Sierra Nevada, California · Albert Bierstadt, 1868"},
    {"ulysses", "Ulysses", "ulysses.jpg",
     "Ulysses Deriding Polyphemus · J. M. W. Turner, 1829"},
    {"ninth-wave", "The Ninth Wave", "ninth-wave.jpg",
     "The Ninth Wave · Ivan Aivazovsky, 1850"},
};
inline constexpr char kDefaultWallpaperPreset[] = "sierra-nevada";

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
