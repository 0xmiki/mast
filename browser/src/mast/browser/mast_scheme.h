// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_SCHEME_H_
#define MAST_BROWSER_MAST_SCHEME_H_

class GURL;

namespace content {
class BrowserContext;
}  // namespace content

namespace mast {

// mast:// is how Mast's own pages are addressed (mast://settings, mast://mast).
// Internally they stay chrome:// pages; the address bar shows them as mast://
// (location_bar_model_impl.cc), and typing mast:// opens them.
inline constexpr char kMastScheme[] = "mast";

// A BrowserURLHandler forward rewrite: mast://x -> chrome://x. Returns false so
// Chromium's own handlers still see the rewritten URL.
bool HandleMastScheme(GURL* url, content::BrowserContext* browser_context);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_SCHEME_H_
