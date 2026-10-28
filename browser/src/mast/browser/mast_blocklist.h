// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_BLOCKLIST_H_
#define MAST_BROWSER_MAST_BLOCKLIST_H_

#include <string>
#include <string_view>
#include <vector>

namespace mast {

// Well-known distractions, blocked by every session. Users add their own on top.
const std::vector<std::string>& PresetSites();

// "www.YouTube.com" -> "youtube.com". Strips www./m./mobile. prefixes.
std::string NormalizeHost(std::string_view host);

// User input such as "https://www.espn.com/nba" -> "espn.com". Empty if invalid.
std::string CleanDomain(std::string_view input);

// True if `host` is `domain` or one of its subdomains.
bool HostMatches(std::string_view host, std::string_view domain);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_BLOCKLIST_H_
