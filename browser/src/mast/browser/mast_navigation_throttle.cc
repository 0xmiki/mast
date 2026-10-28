// Copyright 2026 The Mast Authors

#include "mast/browser/mast_navigation_throttle.h"

#include <memory>

#include "content/public/browser/navigation_handle.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_session.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace mast {

// static
void MastNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  // Always added: a session can start while a navigation is already pending.
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
  MastSession& session = MastSession::Get();
  if (!session.IsBlocked(url)) {
    return PROCEED;
  }
  return ThrottleCheckResult(
      CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      BlockPageHtml(NormalizeHost(url.host()), session.end_time()));
}

}  // namespace mast
