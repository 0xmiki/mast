// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_NAVIGATION_THROTTLE_H_
#define MAST_BROWSER_MAST_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace mast {

// Cancels navigations to blocked sites and shows the Mast block page instead. Not part of the web page, so there is nothing in the DOM
// to delete, and no extension can see or skip it.
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
};

}  // namespace mast

#endif  // MAST_BROWSER_MAST_NAVIGATION_THROTTLE_H_
