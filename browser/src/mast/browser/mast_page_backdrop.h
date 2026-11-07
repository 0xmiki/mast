// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_PAGE_BACKDROP_H_
#define MAST_BROWSER_MAST_PAGE_BACKDROP_H_

#include <string>

namespace content {
class BrowserContext;
}  // namespace content

namespace mast {

// Customize's picture for pages that can't load it from chrome://: the blocked
// page is shown in the blocked site's own origin, so the picture travels inside
// it as a data: URL. Laid out like the new tab page's (desktop.html), so the
// picture runs on from the tab row.
struct PageBackdrop {
  std::string data_url;  // empty: no picture, or not loaded yet
  int top = 0;           // how far the picture starts above the page (DIP)
  bool light = false;    // the picture's tone (Customize: Dark / Light)
};

// The picture for `context`'s pages. The file is read and encoded off the UI
// thread and cached per version: the first call after a change starts that and
// returns no picture (the new tab page asks early, so a blocked page usually
// finds it ready). No picture in off-the-record windows (their tab row has
// none either).
PageBackdrop GetPageBackdrop(content::BrowserContext* context);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_PAGE_BACKDROP_H_
