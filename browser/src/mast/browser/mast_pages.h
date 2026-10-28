// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_PAGES_H_
#define MAST_BROWSER_MAST_PAGES_H_

#include <string>
#include <string_view>

#include "base/time/time.h"

namespace mast {

// The block page shown in place of a blocked site.
std::string BlockPageHtml(std::string_view host, base::Time session_end);

// chrome://mast: start a session, watch it, manage the blocklist.
// `as_new_tab` titles it "New Tab" so tabs and history read naturally.
std::string StartPageHtml(bool as_new_tab);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_PAGES_H_
