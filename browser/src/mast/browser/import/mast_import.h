// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_IMPORT_MAST_IMPORT_H_
#define MAST_BROWSER_IMPORT_MAST_IMPORT_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/time/time.h"

class Profile;

namespace mast {

// One-click import for the welcome page (design C25), the way Brave does it:
// Chromium-family browsers (Chrome, Brave, Edge, Vivaldi, Opera, Chromium) are
// read straight from their profile folders (on Linux, Flatpak and Snap ones
// too), and Firefox goes through Chromium's own importer, which can't read a
// profile while Firefox runs. Passwords and open tabs are left alone on purpose.
//   - Bookmarks: the bookmarks bar's (Firefox: toolbar's) sites go onto the
//     new tab page (Mast's desktop folder); everything else goes into a
//     "From <browser>" folder.
//   - History: the most recent pages, so they come up as you type.
//   - Extensions: the source's Web Store extensions, reinstalled from the
//     Web Store (no prompts: the user asked for them in one click).
struct ImportSource {
  std::string id;           // stable for this session, e.g. "chrome/Default"
  std::string browser;      // "Google Chrome"
  std::string profile;      // the profile's name in that browser
  base::FilePath path;      // its profile folder
  base::Time last_used;     // newest first in the list
  int bookmarks = 0;
  int extensions = 0;
  bool history = false;
  bool firefox = false;     // imported by Chromium's importer
  int firefox_index = -1;   // its index in Chromium's ImporterList
};

struct ImportResult {
  std::string browser;
  int bookmarks = 0;
  int on_new_tab = 0;       // of those, put on the new tab page
  int history = 0;
  int extensions = 0;       // reinstalls started and finished
  bool ok = true;
  bool close_first = false; // nothing came over: close the browser, try again
};

// The browsers and profiles on this computer that have something to import,
// most recently used first. Asynchronous (reads files off the UI thread).
void FindImportSources(
    base::OnceCallback<void(std::vector<ImportSource>)> callback);

// Imports the chosen kinds of data from `source` into `profile`.
void RunImport(Profile* profile,
               const ImportSource& source,
               bool bookmarks,
               bool history,
               bool extensions,
               base::OnceCallback<void(ImportResult)> callback);

}  // namespace mast

#endif  // MAST_BROWSER_IMPORT_MAST_IMPORT_H_
