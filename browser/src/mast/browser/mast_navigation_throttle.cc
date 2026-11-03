// Copyright 2026 The Mast Authors

#include "mast/browser/mast_navigation_throttle.h"

#include <memory>
#include <optional>

#include "content/public/browser/navigation_handle.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_blocks.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace mast {

// static
void MastNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  // Always added: a site can be blocked while a navigation is already pending.
  registry.AddThrottle(std::make_unique<MastNavigationThrottle>(registry));
}

MastNavigationThrottle::MastNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

MastNavigationThrottle::~MastNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
MastNavigationThrottle::WillStartRequest() {
  return Check();
}

content::NavigationThrottle::ThrottleCheckResult
MastNavigationThrottle::WillRedirectRequest() {
  return Check();
}

const char* MastNavigationThrottle::GetNameForLogging() {
  return "MastNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
MastNavigationThrottle::Check() {
  const GURL& url = navigation_handle()->GetURL();
  MastBlocks& blocks = MastBlocks::Get();
  const std::optional<BlockMatch> match = blocks.ForUrl(url);
  if (!match) {
    return PROCEED;
  }
  if (navigation_handle()->IsInPrimaryMainFrame()) {
    blocks.RecordBlocked();
  }
  return ThrottleCheckResult(CANCEL, net::ERR_BLOCKED_BY_CLIENT,
                             BlockPageHtml(NormalizeHost(url.host()), *match));
}

}  // namespace mast
