// Copyright 2026 The Mast Authors

#ifndef MAST_COMMON_MAST_FEATURES_H_
#define MAST_COMMON_MAST_FEATURES_H_

#include "base/feature_list.h"

namespace mast {

// One row: tabs, then back / address / forward, then the menu, in a single row
// that is also the titlebar. Off: Chromium's two rows (tabs above the toolbar).
// Try either without a rebuild: --disable-features=MastOneRowWindow
BASE_DECLARE_FEATURE(kMastOneRowWindow);

// In the one-row window the address bar is narrow, so its suggestions open
// wider than it, growing towards the tabs.
BASE_DECLARE_FEATURE(kMastWideOmniboxPopup);

bool IsOneRowWindow();

// The gap above and below a tab's pill in the one-row window (DIP): the pill is
// centred in the row, like the design (Chromium insets it 6 above, 7 below).
inline constexpr int kOneRowTabPillInset = 5;

// The address bar's width in the one-row window.
inline constexpr int kOneRowLocationBarWidth = 228;
// The suggestion list's width when it opens wider than the address bar.
inline constexpr int kWideOmniboxPopupWidth = 560;

}  // namespace mast

#endif  // MAST_COMMON_MAST_FEATURES_H_
