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
  // The palette picked in Customize (mast_palette.h); providers are rebuilt
  // when it changes.
  const palette::Palette& p = palette::Current();
  ui::ColorMixer& mixer = provider->AddPostprocessingMixer();

  // Frame and tab strip: the darkest layer. Tabs sit on it; the active tab is
  // the toolbar's colour so it flows into the toolbar.
  mixer[ui::kColorFrameActive] = {p.bg};
  mixer[ui::kColorFrameInactive] = {p.bg};
  mixer[kColorTabBackgroundInactiveFrameActive] = {p.bg};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {p.bg};
  mixer[kColorTabBackgroundInactiveHoverFrameActive] = {p.chrome2};
  mixer[kColorTabBackgroundActiveFrameActive] = {p.chrome};
  mixer[kColorTabBackgroundActiveFrameInactive] = {p.chrome};
  mixer[kColorTabForegroundActiveFrameActive] = {p.text};
  mixer[kColorTabForegroundInactiveFrameActive] = {p.muted};
  mixer[kColorTabDividerFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorTabStrokeFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {SK_ColorTRANSPARENT};

  // Toolbar: one calm surface; a single hairline above the page.
  mixer[kColorToolbar] = {p.chrome};
  mixer[kColorToolbarButtonIcon] = {p.muted};
  mixer[kColorToolbarButtonText] = {p.text};
  mixer[kColorToolbarSeparator] = {SK_ColorTRANSPARENT};
  mixer[kColorToolbarTopSeparatorFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorToolbarContentAreaSeparator] = {p.line};
  mixer[kColorBookmarkBarBackground] = {p.chrome};
  mixer[kColorInfoBarBackground] = {p.chrome};

  // Omnibox: a field, and its dropdown.
  mixer[kColorLocationBarBackground] = {p.chrome2};
  mixer[kColorLocationBarBackgroundHovered] = {p.raised};
  mixer[kColorOmniboxText] = {p.text};
  // The focused field paints the results colour, so it must stay a field:
  // with the toolbar's colour the field vanished on focus, leaving only a ring.
  mixer[kColorOmniboxResultsBackground] = {p.chrome2};
  mixer[kColorOmniboxResultsBackgroundHovered] = {p.raised};
  mixer[kColorOmniboxResultsBackgroundSelected] = {p.raised2};
  // Also the omnibox focus ring (mast-theme.patch gives the ring this id,
  // which skips Chromium's 3:1 contrast lift): a hairline, not a halo.
  mixer[kColorLocationBarBorder] = {p.border};
  mixer[kColorOmniboxResultsFocusIndicator] = {SK_ColorTRANSPARENT};

  // Menus, bubbles, dialogs, focus.
  // Bubbles, dialogs and menus sit one step above the page (#141414), so they
  // keep an edge on Mast's own pages, which are #070707.
  mixer[ui::kColorMenuBackground] = {p.chrome2};
  mixer[ui::kColorMenuItemBackgroundHighlighted] = {p.raised2};
  mixer[ui::kColorMenuItemBackgroundSelected] = {p.raised2};
  mixer[ui::kColorMenuSeparator] = {p.line};
  mixer[ui::kColorBubbleBackground] = {p.chrome2};
  mixer[ui::kColorBubbleBorder] = {p.raised2};
  mixer[ui::kColorDialogBackground] = {p.chrome2};
  mixer[ui::kColorComboboxBackground] = {p.chrome2};
  mixer[ui::kColorComboboxContainerOutline] = {p.raised2};
  mixer[ui::kColorPrimaryBackground] = {p.chrome};
  mixer[ui::kColorTextfieldBackground] = {p.chrome2};
  // Focus is neutral: the signal colour means blocked, nothing else.
  // Keyboard focus stays visible but quiet. Chromium lifts focus rings to 3:1
  // contrast against their background, so this is a floor, not the result.
  mixer[ui::kColorFocusableBorderFocused] = {p.faint};

  // Text selection: neutral too.
  constexpr SkColor kSelection = SkColorSetRGB(0x45, 0x43, 0x40);
  mixer[ui::kColorTextSelectionBackground] = {kSelection};
  mixer[ui::kColorTextSelectionForeground] = {p.text};
  mixer[kColorOmniboxSelectionBackground] = {kSelection};
  mixer[kColorOmniboxSelectionForeground] = {p.text};

  // One row (mast/common/mast_features.h), seamless: the tab row is the page's
  // colour with no line under it; the active tab is a raised pill, like the
  // address field.
  if (IsOneRowWindow()) {
    mixer[ui::kColorFrameActive] = {p.chrome};
    mixer[ui::kColorFrameInactive] = {p.chrome};
    mixer[kColorToolbar] = {p.chrome};
    mixer[kColorTabBackgroundInactiveFrameActive] = {p.chrome};
    mixer[kColorTabBackgroundInactiveFrameInactive] = {p.chrome};
    mixer[kColorTabBackgroundActiveFrameActive] = {p.chrome2};
    mixer[kColorTabBackgroundActiveFrameInactive] = {p.chrome2};
    mixer[kColorTabBackgroundInactiveHoverFrameActive] = {p.hover};
    // The 1px separator above the page takes the row's colour: transparent
    // showed the darker region behind it as a line.
    mixer[kColorToolbarContentAreaSeparator] = {p.chrome};
  }
}

}  // namespace mast
