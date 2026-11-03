// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_BLOCKLIST_H_
#define MAST_BROWSER_MAST_BLOCKLIST_H_

#include <string>
#include <string_view>
#include <vector>

namespace mast {

// One thing people block, e.g. "YouTube": every domain that opens it, plus
// subdomains left open because work depends on them (aws.amazon.com).
struct Service {
  std::string name;
  std::vector<std::string> domains;
  std::vector<std::string> except;
};

struct Category {
  std::string id;
  std::string name;
  std::vector<Service> services;
};

// The curated list (blocklist/blocklist.json, embedded at build time).
const std::vector<Category>& Categories();

// True if `host` is under one of the list's `except` entries.
bool IsExcepted(std::string_view host);

// The category a listed domain belongs to; empty for user-added sites.
std::string CategoryNameOf(std::string_view domain);

// True if the domain is in the curated list.
bool IsListed(std::string_view domain);

// "www.YouTube.com" -> "youtube.com". Strips www./m./mobile. prefixes.
std::string NormalizeHost(std::string_view host);

// User input such as "https://www.espn.com/nba" -> "espn.com". Empty if invalid.
std::string CleanDomain(std::string_view input);

// True if `host` is `domain` or one of its subdomains.
bool HostMatches(std::string_view host, std::string_view domain);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_BLOCKLIST_H_
