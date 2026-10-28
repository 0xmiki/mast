// Copyright 2026 The Mast Authors

#include "mast/browser/mast_blocklist.h"

#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace mast {

const std::vector<std::string>& PresetSites() {
  static const base::NoDestructor<std::vector<std::string>> kSites({
      // Social
      "x.com", "twitter.com", "facebook.com", "instagram.com", "tiktok.com",
      "reddit.com", "snapchat.com", "threads.net", "bsky.app", "pinterest.com",
      "tumblr.com", "discord.com",
      // Video & streaming
      "youtube.com", "netflix.com", "twitch.tv", "kick.com", "primevideo.com",
      "disneyplus.com", "hulu.com", "max.com",
      // News & forums
      "news.ycombinator.com", "cnn.com", "foxnews.com", "bbc.com",
      "nytimes.com", "buzzfeed.com", "9gag.com",
      // Shopping
      "amazon.com", "ebay.com", "aliexpress.com", "temu.com", "etsy.com",
      "shein.com",
      // Games
      "chess.com", "lichess.org", "roblox.com", "store.steampowered.com",
      "poki.com",
  });
  return *kSites;
}

std::string NormalizeHost(std::string_view host) {
  std::string h = base::ToLowerASCII(host);
  for (std::string_view prefix : {"www.", "m.", "mobile."}) {
    if (h.starts_with(prefix)) {
      return h.substr(prefix.size());
    }
  }
  return h;
}

std::string CleanDomain(std::string_view input) {
  std::string text(base::TrimWhitespaceASCII(input, base::TRIM_ALL));
  if (text.empty()) {
    return std::string();
  }
  if (text.find("://") == std::string::npos) {
    text = "https://" + text;
  }
  GURL url(text);
  if (!url.is_valid() || !url.has_host() || url.host().find('.') == std::string::npos) {
    return std::string();
  }
  return NormalizeHost(url.host());
}

bool HostMatches(std::string_view host, std::string_view domain) {
  if (host == domain) {
    return true;
  }
  return host.size() > domain.size() && host.ends_with(domain) &&
         host[host.size() - domain.size() - 1] == '.';
}

}  // namespace mast
