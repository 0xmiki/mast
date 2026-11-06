// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_PAGES_H_
#define MAST_BROWSER_MAST_PAGES_H_

#include <string>
#include <string_view>


namespace mast {

struct BlockMatch;

// The block page shown in place of a blocked site.
std::string BlockPageHtml(std::string_view host, const BlockMatch& match);

// chrome://mast: the blocklist, every category and site.
// `as_new_tab` titles it "New Tab" so tabs and history read naturally.
std::string StartPageHtml(bool as_new_tab);

// The new tab page: saved sites as a desktop of icons, plus widgets (clock,
// and one button that blocks every distraction).
std::string DesktopPageHtml();

}  // namespace mast

#endif  // MAST_BROWSER_MAST_PAGES_H_
