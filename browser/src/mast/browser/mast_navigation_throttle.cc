// Copyright 2026 The Mast Authors

#include "mast/browser/mast_navigation_throttle.h"

#include <memory>
#include <optional>

#include "base/functional/bind.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_feeds.h"
#include "mast/browser/mast_page_backdrop.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_blocks.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace mast {

// static
void MastNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  // Always added: a site can be blocked while a navigation is already pending.
  MastFeeds::Get().Load();
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
  if (!navigation_handle()->GetURL().SchemeIsHTTPOrHTTPS()) {
    return PROCEED;
  }
  if (!MastFeeds::Get().ready()) {
    MastFeeds::Get().WhenReady(base::BindOnce(
        &MastNavigationThrottle::OnFeedsReady, weak_factory_.GetWeakPtr()));
    return DEFER;
  }
  return Decide();
}

void MastNavigationThrottle::OnFeedsReady() {
  const ThrottleCheckResult result = Decide();
  if (result.action() == PROCEED) {
    Resume();
  } else {
    CancelDeferredNavigation(result);
  }
}

content::NavigationThrottle::ThrottleCheckResult
MastNavigationThrottle::Decide() {
  const GURL& url = navigation_handle()->GetURL();
  const std::string host = NormalizeHost(url.host());
  auto backdrop = [this] {
    return GetPageBackdrop(
        navigation_handle()->GetWebContents()->GetBrowserContext());
  };
  // Adult sites: always, quietly. Not counted, not named, no end time.
  if (!IsExcepted(host) && IsAlwaysBlocked(host)) {
    return ThrottleCheckResult(CANCEL, net::ERR_BLOCKED_BY_CLIENT,
                               QuietBlockPageHtml(host, backdrop()));
  }
  MastBlocks& blocks = MastBlocks::Get();
  const std::optional<BlockMatch> match = blocks.ForUrl(url);
  if (!match) {
    return PROCEED;
  }
  if (navigation_handle()->IsInPrimaryMainFrame()) {
    blocks.RecordBlocked();
  }
  return ThrottleCheckResult(CANCEL, net::ERR_BLOCKED_BY_CLIENT,
                             BlockPageHtml(host, *match, backdrop()));
}

}  // namespace mast
