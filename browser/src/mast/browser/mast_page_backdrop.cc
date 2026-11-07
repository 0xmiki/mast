// Copyright 2026 The Mast Authors

#include "mast/browser/mast_page_backdrop.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "chrome/browser/themes/theme_properties.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "mast/browser/mast_prefs.h"
#include "mast/common/mast_features.h"

namespace mast {

namespace {

struct Cache {
  base::FilePath path;
  int version = 0;
  std::string data_url;  // empty while loading
};

Cache& GetCache() {
  static base::NoDestructor<Cache> cache;
  return *cache;
}

std::string ReadAsDataUrl(const base::FilePath& path) {
  std::string jpeg;
  if (!base::ReadFileToString(path, &jpeg) || jpeg.empty()) {
    return std::string();
  }
  return "data:image/jpeg;base64," + base::Base64Encode(jpeg);
}

void OnRead(const base::FilePath& path, int version, std::string data_url) {
  Cache& c = GetCache();
  if (c.path == path && c.version == version) {
    c.data_url = std::move(data_url);
  }
}

}  // namespace

PageBackdrop GetPageBackdrop(content::BrowserContext* context) {
  PageBackdrop backdrop;
  if (!context || context->IsOffTheRecord()) {
    return backdrop;
  }
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  const int version = prefs ? prefs->GetInteger(kWallpaperPref) : 0;
  if (version <= 0) {
    return backdrop;
  }
  Cache& c = GetCache();
  const base::FilePath path = context->GetPath().AppendASCII(kWallpaperFile);
  if (c.path != path || c.version != version) {
    c.path = path;
    c.version = version;
    c.data_url.clear();
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(&ReadAsDataUrl, path),
        base::BindOnce(&OnRead, path, version));
    return backdrop;
  }
  backdrop.data_url = c.data_url;
  // The picture's top-left is Chromium's theme-image origin, as in the tab row
  // (mast_wallpaper.cc) and the new tab page (mast_ui.cc's WallpaperTop()).
  backdrop.top = OneRowContentTop() + ThemeProperties::kFrameHeightAboveTabs;
  backdrop.light = prefs->GetString(kWallpaperTonePref) == "light";
  return backdrop;
}

}  // namespace mast
