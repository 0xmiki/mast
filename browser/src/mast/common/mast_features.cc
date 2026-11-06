// Copyright 2026 The Mast Authors

#include "mast/common/mast_features.h"

namespace mast {

BASE_FEATURE(kMastOneRowWindow, base::FEATURE_ENABLED_BY_DEFAULT);
// Off: Chromium's WebUI suggestion list lays itself out from the address bar's
// own bounds, so a wider window alone renders a narrow column (seen 2026-11-06).
BASE_FEATURE(kMastWideOmniboxPopup, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsOneRowWindow() {
  return base::FeatureList::IsEnabled(kMastOneRowWindow);
}

}  // namespace mast
