// Copyright 2026 The Mast Authors

#include "mast/browser/ui/color/mast_chrome_color_mixer.h"

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "mast/ui/color/mast_palette.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"

namespace mast {

void AddMastChromeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderKey& key) {
  if (key.custom_theme ||
      key.contrast_mode == ui::ColorProviderKey::ContrastMode::kHigh ||
      key.color_mode != ui::ColorProviderKey::ColorMode::kDark) {
    return;
  }
  namespace p = palette;
  ui::ColorMixer& mixer = provider->AddPostprocessingMixer();

  // Frame and tab strip: the darkest layer. Tabs sit on it; the active tab is
  // the toolbar's colour so it flows into the toolbar (the demo's flared tab).
  mixer[ui::kColorFrameActive] = {p::kBg};
  mixer[ui::kColorFrameInactive] = {p::kBg};
  mixer[kColorTabBackgroundInactiveFrameActive] = {p::kBg};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {p::kBg};
  mixer[kColorTabBackgroundInactiveHoverFrameActive] = {p::kChrome2};
  mixer[kColorTabBackgroundActiveFrameActive] = {p::kChrome};
  mixer[kColorTabBackgroundActiveFrameInactive] = {p::kChrome};
  mixer[kColorTabForegroundActiveFrameActive] = {p::kText};
  mixer[kColorTabForegroundInactiveFrameActive] = {p::kMuted};
  mixer[kColorTabDividerFrameActive] = {p::kLine};
  mixer[kColorTabStrokeFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {SK_ColorTRANSPARENT};

  // Toolbar: one calm surface, no separators.
  mixer[kColorToolbar] = {p::kChrome};
  mixer[kColorToolbarButtonIcon] = {p::kMuted};
  mixer[kColorToolbarButtonText] = {p::kText};
  mixer[kColorToolbarSeparator] = {SK_ColorTRANSPARENT};
  mixer[kColorToolbarTopSeparatorFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorToolbarContentAreaSeparator] = {p::kLine};
  mixer[kColorBookmarkBarBackground] = {p::kChrome};
  mixer[kColorInfoBarBackground] = {p::kChrome};

  // Omnibox: the pill and its dropdown.
  mixer[kColorLocationBarBackground] = {p::kChrome2};
  mixer[kColorLocationBarBackgroundHovered] = {p::kRaised};
  mixer[kColorOmniboxText] = {p::kText};
  mixer[kColorOmniboxResultsBackground] = {p::kChrome};
  mixer[kColorOmniboxResultsBackgroundHovered] = {p::kRaised};
  mixer[kColorOmniboxResultsBackgroundSelected] = {p::kRaised2};

  // Menus, bubbles, dialogs, focus.
  mixer[ui::kColorMenuBackground] = {p::kChrome};
  mixer[ui::kColorMenuItemBackgroundHighlighted] = {p::kRaised};
  mixer[ui::kColorMenuSeparator] = {p::kLine};
  mixer[ui::kColorBubbleBackground] = {p::kChrome};
  mixer[ui::kColorDialogBackground] = {p::kChrome};
  mixer[ui::kColorPrimaryBackground] = {p::kChrome};
  mixer[ui::kColorTextfieldBackground] = {p::kChrome2};
  mixer[ui::kColorFocusableBorderFocused] = {p::kEmber};
}

}  // namespace mast
