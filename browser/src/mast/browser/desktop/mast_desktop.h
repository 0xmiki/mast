// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_DESKTOP_MAST_DESKTOP_H_
#define MAST_BROWSER_DESKTOP_MAST_DESKTOP_H_

#include <memory>

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace power_bookmarks {
class SuggestedSaveLocationProvider;
}  // namespace power_bookmarks

namespace mast {

// The sites on the desktop new tab page are the bookmarks in one folder,
// "Desktop" under Other bookmarks. It's found by a meta-info key, not by its
// title, so renaming or moving it in the bookmark manager keeps it working.

// The Desktop folder, or null if there isn't one (or the model isn't loaded).
const bookmarks::BookmarkNode* FindDesktopFolder(
    const bookmarks::BookmarkModel* model);

// The Desktop folder, created on first use. Null only if the model isn't loaded.
const bookmarks::BookmarkNode* DesktopFolder(bookmarks::BookmarkModel* model);

// Makes new bookmarks of web pages (the star in the address bar, Ctrl+D) go to
// the Desktop folder. Registered by ChromeBookmarkClient.
std::unique_ptr<power_bookmarks::SuggestedSaveLocationProvider>
CreateDesktopSaveLocationProvider(bookmarks::BookmarkModel* model);

}  // namespace mast

#endif  // MAST_BROWSER_DESKTOP_MAST_DESKTOP_H_
