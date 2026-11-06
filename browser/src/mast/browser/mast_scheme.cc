// Copyright 2026 The Mast Authors

#include "mast/browser/mast_scheme.h"

#include <string>

#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace mast {

bool HandleMastScheme(GURL* url, content::BrowserContext* browser_context) {
  if (!url->SchemeIs(kMastScheme)) {
    return false;
  }
  // Swap the scheme textually: everything after "mast:" stays as typed.
  const std::string& spec = url->spec();
  *url = GURL(std::string(content::kChromeUIScheme) +
              spec.substr(std::char_traits<char>::length(kMastScheme)));
  return false;
}

}  // namespace mast
