// Copyright 2026 The Mast Authors

#include "mast/browser/mast_wallpapers.h"

#include <cstdlib>
#include <string>
#include <utility>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "mast/browser/mast_prefs.h"
#include "mast/browser/mast_theme.h"

namespace mast {

const WallpaperPreset* FindWallpaperPreset(std::string_view id) {
  for (const WallpaperPreset& preset : kWallpaperPresets) {
    if (id == preset.id) {
      return &preset;
    }
  }
  return nullptr;
}

base::FilePath WallpaperPresetPath(const WallpaperPreset& preset) {
  base::FilePath dir;
  base::PathService::Get(base::DIR_ASSETS, &dir);
  return dir.AppendASCII("mast-wallpapers").AppendASCII(preset.file);
}

void UseWallpaperPreset(Profile* profile,
                        std::string_view id,
                        base::OnceCallback<void(bool)> done) {
  const WallpaperPreset* preset = FindWallpaperPreset(id);
  if (!profile || !preset) {
    std::move(done).Run(false);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&base::CopyFile, WallpaperPresetPath(*preset),
                     profile->GetPath().AppendASCII(kWallpaperFile)),
      base::BindOnce(
          [](base::WeakPtr<Profile> weak, std::string id,
             base::OnceCallback<void(bool)> done, bool copied) {
            Profile* profile = weak.get();
            if (!profile || !copied) {
              std::move(done).Run(false);
              return;
            }
            PrefService* prefs = profile->GetPrefs();
            // A new version per change, never reused (see kWallpaperPref).
            prefs->SetInteger(kWallpaperPref,
                              std::abs(prefs->GetInteger(kWallpaperPref)) + 1);
            prefs->SetString(kWallpaperTonePref, "dark");
            prefs->SetString(kWallpaperPresetPref, id);
            if (!ApplyTheme(prefs)) {
              RepaintBrowser();  // same colours, new picture
            }
            std::move(done).Run(true);
          },
          profile->GetWeakPtr(), std::string(id), std::move(done)));
}

}  // namespace mast
