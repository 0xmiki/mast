// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_NAVIGATION_THROTTLE_H_
#define MAST_BROWSER_MAST_NAVIGATION_THROTTLE_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace mast {

// Cancels navigations to blocked sites and shows the Mast block page instead.
// Not part of the web page, so there is nothing in the DOM to delete, and no
// extension can see or skip it. Web navigations wait for the feeds
// (mast_feeds.h) on the rare occasion they're still loading, so a restored
// session can't slip past the adult list.
class MastNavigationThrottle : public content::NavigationThrottle {
 public:
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  explicit MastNavigationThrottle(content::NavigationThrottleRegistry& registry);
  ~MastNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult Check();
  ThrottleCheckResult Decide();
  void OnFeedsReady();

  base::WeakPtrFactory<MastNavigationThrottle> weak_factory_{this};
};

}  // namespace mast

#endif  // MAST_BROWSER_MAST_NAVIGATION_THROTTLE_H_
