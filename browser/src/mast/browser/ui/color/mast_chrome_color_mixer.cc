// Copyright 2026 The Mast Authors

#include "mast/browser/ui/color/mast_chrome_color_mixer.h"

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "mast/common/mast_features.h"
#include "mast/ui/color/mast_palette.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"
#include "ui/color/color_transform.h"

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
  // the toolbar's colour so it flows into the toolbar.
  mixer[ui::kColorFrameActive] = {p::kBg};
  mixer[ui::kColorFrameInactive] = {p::kBg};
  mixer[kColorTabBackgroundInactiveFrameActive] = {p::kBg};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {p::kBg};
  mixer[kColorTabBackgroundInactiveHoverFrameActive] = {p::kChrome2};
  mixer[kColorTabBackgroundActiveFrameActive] = {p::kChrome};
  mixer[kColorTabBackgroundActiveFrameInactive] = {p::kChrome};
  mixer[kColorTabForegroundActiveFrameActive] = {p::kText};
  mixer[kColorTabForegroundInactiveFrameActive] = {p::kMuted};
  mixer[kColorTabDividerFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorTabStrokeFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {SK_ColorTRANSPARENT};

  // Toolbar: one calm surface; a single hairline above the page.
  mixer[kColorToolbar] = {p::kChrome};
  mixer[kColorToolbarButtonIcon] = {p::kMuted};
  mixer[kColorToolbarButtonText] = {p::kText};
  mixer[kColorToolbarSeparator] = {SK_ColorTRANSPARENT};
  mixer[kColorToolbarTopSeparatorFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorToolbarContentAreaSeparator] = {p::kLine};
  mixer[kColorBookmarkBarBackground] = {p::kChrome};
  mixer[kColorInfoBarBackground] = {p::kChrome};

  // Omnibox: a field, and its dropdown.
  mixer[kColorLocationBarBackground] = {p::kChrome2};
  mixer[kColorLocationBarBackgroundHovered] = {p::kRaised};
  mixer[kColorOmniboxText] = {p::kText};
  // The focused field paints the results colour, so it must stay a field:
  // with the toolbar's colour the field vanished on focus, leaving only a ring.
  mixer[kColorOmniboxResultsBackground] = {p::kChrome2};
  mixer[kColorOmniboxResultsBackgroundHovered] = {p::kRaised};
  mixer[kColorOmniboxResultsBackgroundSelected] = {p::kRaised2};
  // Also the omnibox focus ring (mast-theme.patch gives the ring this id,
  // which skips Chromium's 3:1 contrast lift): a hairline, not a halo.
  mixer[kColorLocationBarBorder] = {SkColorSetRGB(0x3B, 0x3A, 0x38)};
  mixer[kColorOmniboxResultsFocusIndicator] = {SK_ColorTRANSPARENT};

  // Menus, bubbles, dialogs, focus.
  // Bubbles, dialogs and menus sit one step above the page (#262626), so they
  // keep an edge on Mast's own pages, which are #1c1c1c.
  mixer[ui::kColorMenuBackground] = {p::kChrome2};
  mixer[ui::kColorMenuItemBackgroundHighlighted] = {p::kRaised2};
  mixer[ui::kColorMenuItemBackgroundSelected] = {p::kRaised2};
  mixer[ui::kColorMenuSeparator] = {p::kLine};
  mixer[ui::kColorBubbleBackground] = {p::kChrome2};
  mixer[ui::kColorBubbleBorder] = {p::kRaised2};
  mixer[ui::kColorDialogBackground] = {p::kChrome2};
  mixer[ui::kColorComboboxBackground] = {p::kChrome2};
  mixer[ui::kColorComboboxContainerOutline] = {p::kRaised2};
  mixer[ui::kColorPrimaryBackground] = {p::kChrome};
  mixer[ui::kColorTextfieldBackground] = {p::kChrome2};
  // Focus is neutral: the signal colour means blocked, nothing else.
  // Keyboard focus stays visible but quiet. Chromium lifts focus rings to 3:1
  // contrast against their background, so this is a floor, not the result.
  mixer[ui::kColorFocusableBorderFocused] = {p::kFaint};

  // Text selection: neutral too.
  constexpr SkColor kSelection = SkColorSetRGB(0x45, 0x43, 0x40);
  mixer[ui::kColorTextSelectionBackground] = {kSelection};
  mixer[ui::kColorTextSelectionForeground] = {p::kText};
  mixer[kColorOmniboxSelectionBackground] = {kSelection};
  mixer[kColorOmniboxSelectionForeground] = {p::kText};

  // One row (mast/common/mast_features.h): tabs and toolbar share the frame's
  // colour; the active tab is a raised pill, like the address field.
  if (IsOneRowWindow()) {
    mixer[kColorToolbar] = {p::kBg};
    mixer[kColorTabBackgroundActiveFrameActive] = {p::kChrome2};
    mixer[kColorTabBackgroundActiveFrameInactive] = {p::kChrome2};
    mixer[kColorTabBackgroundInactiveHoverFrameActive] = {
        SkColorSetRGB(0x1E, 0x1E, 0x1E)};
  }
}

}  // namespace mast
