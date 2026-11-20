// Copyright 2026 The Mast Authors

#include "mast/browser/ui/color/mast_chrome_color_mixer.h"

#include <initializer_list>
#include <optional>

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "mast/common/mast_features.h"
#include "mast/ui/color/mast_palette.h"
#include "mast/ui/color/mast_site_color.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"
#include "ui/color/color_transform.h"

namespace mast {

namespace {

// The one-row tab row in a website's own colour (Customize: Tab bar on
// websites, "Match site"; design C23). Everything is built from that colour
// (mast/ui/color/mast_site_color.h): the row is the colour, the active tab and
// the address field are steps away from it, text and icons are dark on a light
// colour and light on a dark one. The window stays in dark mode either way: a
// page's own light or dark look follows the window's, so a light row must not
// turn the site light. Menus and bubbles keep the palette.
void AddSiteRow(ui::ColorMixer& mixer, const site_color::Row& r) {
  for (ui::ColorId id :
       std::initializer_list<ui::ColorId>{ui::kColorFrameActive, ui::kColorFrameInactive, kColorToolbar,
        kColorToolbarContentAreaSeparator,
        kColorTabBackgroundInactiveFrameActive,
        kColorTabBackgroundInactiveFrameInactive}) {
    mixer[id] = {r.bar};
  }
  mixer[kColorTabBackgroundInactiveHoverFrameActive] = {r.hover};
  mixer[kColorTabBackgroundInactiveHoverFrameInactive] = {r.hover};
  mixer[kColorTabBackgroundActiveFrameActive] = {r.pill};
  mixer[kColorTabBackgroundActiveFrameInactive] = {r.pill};
  mixer[kColorTabForegroundActiveFrameActive] = {r.text};
  mixer[kColorTabForegroundActiveFrameInactive] = {r.text};
  mixer[kColorTabForegroundInactiveFrameActive] = {r.text2};
  mixer[kColorTabForegroundInactiveFrameInactive] = {r.text2};
  for (ui::ColorId id : std::initializer_list<ui::ColorId>{kColorNewTabButtonForegroundFrameActive,
                         kColorNewTabButtonForegroundFrameInactive,
                         kColorNewTabButtonCRForegroundFrameActive,
                         kColorNewTabButtonCRForegroundFrameInactive}) {
    mixer[id] = {r.text2};
  }
  for (ui::ColorId id : std::initializer_list<ui::ColorId>{kColorNewTabButtonBackgroundFrameActive,
                         kColorNewTabButtonBackgroundFrameInactive,
                         kColorNewTabButtonCRBackgroundFrameActive,
                         kColorNewTabButtonCRBackgroundFrameInactive}) {
    mixer[id] = {SK_ColorTRANSPARENT};
  }
  mixer[kColorToolbarButtonIcon] = {r.text2};
  mixer[kColorToolbarButtonIconHovered] = {r.text};
  mixer[kColorToolbarButtonIconPressed] = {r.text};
  mixer[kColorToolbarButtonIconInactive] = {r.faint};
  mixer[kColorToolbarButtonIconDisabled] = {r.faint};
  mixer[kColorToolbarButtonText] = {r.text};

  // The address field, and its icons (the lock, the star).
  mixer[kColorLocationBarBackground] = {r.field};
  mixer[kColorLocationBarBackgroundHovered] = {r.field_hover};
  mixer[kColorLocationBarBorder] = {r.border};
  mixer[kColorOmniboxText] = {r.text};
  mixer[kColorOmniboxTextDimmed] = {r.text2};
  mixer[kColorOmniboxActionIcon] = {r.text2};
  mixer[kColorPageInfoForeground] = {r.text2};
  mixer[kColorPageInfoSubtitleForeground] = {r.text2};
  mixer[kColorLocationBarClearAllButtonIcon] = {r.text2};
  // The site-settings chip at the start of the field (LocationIconView): a
  // step further from the field than the field is from the row.
  mixer[kColorOmniboxIconBackground] = {r.raised};
  mixer[kColorOmniboxIconBackgroundTonal] = {r.raised};
  mixer[kColorOmniboxIconForeground] = {r.text2};
  mixer[kColorOmniboxIconForegroundTonal] = {r.text2};
  mixer[kColorOmniboxIconHover] = {SkColorSetA(r.text, 0x14)};
  mixer[kColorOmniboxIconPressed] = {SkColorSetA(r.text, 0x24)};
  mixer[kColorOmniboxActionIconHover] = {SkColorSetA(r.text, 0x14)};
  if (!r.light) {
    return;  // a dark row: the palette's dark suggestions suit it
  }
  // A light row: the focused field paints the suggestions' colour, so they go
  // light too, with dark text and icons.
  mixer[kColorOmniboxResultsBackground] = {r.field};
  mixer[kColorOmniboxResultsBackgroundHovered] = {r.raised};
  mixer[kColorOmniboxResultsBackgroundSelected] = {r.raised2};
  for (ui::ColorId id : std::initializer_list<ui::ColorId>{kColorOmniboxResultsTextSelected,
                         kColorOmniboxResultsTextAnswer,
                         kColorOmniboxKeywordSelected}) {
    mixer[id] = {r.text};
  }
  for (ui::ColorId id :
       std::initializer_list<ui::ColorId>{kColorOmniboxResultsTextDimmed, kColorOmniboxResultsTextDimmedSelected,
        kColorOmniboxResultsTextSecondary, kColorOmniboxResultsUrl,
        kColorOmniboxResultsUrlSelected, kColorOmniboxResultsIcon,
        kColorOmniboxResultsIconSelected, kColorOmniboxResultsIconHovered,
        kColorOmniboxResultsButtonIcon, kColorOmniboxResultsButtonIconSelected,
        kColorOmniboxKeywordSeparator}) {
    mixer[id] = {r.text2};
  }
}

}  // namespace

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

  // WebUI pages (Settings, History, Downloads...): page and cards follow the
  // palette like Mast's own pages (md_colors.css and cr_shared_vars.css read
  // these through chrome://theme/colors.css; see mast-theme.patch).
  mixer[kColorWebuiPageBackground] = {p.chrome};
  mixer[kColorSettingsWebuiPageBackground] = {p.chrome};
  mixer[kColorWebuiCardBackground] = {p.chrome2};
  mixer[kColorSettingsSectionBackground] = {p.chrome2};
  mixer[ui::kColorPrimaryForeground] = {p.text};
  mixer[ui::kColorSecondaryForeground] = {p.muted};

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

  // On Customize's image (mast_wallpaper.h paints it behind the row), the row
  // follows the design (C10): only the active tab is a pill, and it and the
  // address field are translucent, so the picture shows through. Text and
  // icons are strong enough to read on any picture: light on a dark image,
  // dark on a light one (the user's Dark/Light pick). The address field stays a
  // dark pill either way; its suggestions open below it in the same colour.
  //
  // A window can set its own row instead (Customize: Tab bar on websites;
  // mast/browser/ui/mast_tab_bar.h), carried in its key like an installed web
  // app's colours: transparent for the palette (a website in "Theme"), or a
  // website's colour ("Match site").
  const std::optional<SkColor> own_row =
      key.app_controller ? std::nullopt : key.app_background_color;
  const palette::Backdrop backdrop =
      own_row ? palette::Backdrop::kPalette : palette::CurrentBackdrop();
  if (IsOneRowWindow() && backdrop != palette::Backdrop::kPalette) {
    const bool light = backdrop == palette::Backdrop::kLightImage;
    const SkColor fg = light ? SkColorSetRGB(0x14, 0x14, 0x14) : p.text;
    const SkColor fg2 = light ? SkColorSetARGB(0xD9, 0x14, 0x14, 0x14)
                              : SkColorSetARGB(0xD9, 0xF2, 0xF2, 0xF2);
    const SkColor faint = light ? SkColorSetARGB(0x66, 0x00, 0x00, 0x00)
                                : SkColorSetARGB(0x66, 0xFF, 0xFF, 0xFF);
    // Inactive tabs paint no fill (they filled the frame colour, which read as
    // dark pills on the picture).
    mixer[kColorTabBackgroundInactiveFrameActive] = {SK_ColorTRANSPARENT};
    mixer[kColorTabBackgroundInactiveFrameInactive] = {SK_ColorTRANSPARENT};
    mixer[kColorTabBackgroundInactiveHoverFrameActive] = {
        light ? SkColorSetARGB(0x33, 0xFF, 0xFF, 0xFF)
              : SkColorSetARGB(0x14, 0xFF, 0xFF, 0xFF)};
    mixer[kColorTabBackgroundInactiveHoverFrameInactive] = {
        light ? SkColorSetARGB(0x33, 0xFF, 0xFF, 0xFF)
              : SkColorSetARGB(0x14, 0xFF, 0xFF, 0xFF)};
    const SkColor pill = light ? SkColorSetARGB(0x99, 0xFF, 0xFF, 0xFF)
                               : SkColorSetARGB(0x26, 0xFF, 0xFF, 0xFF);
    mixer[kColorTabBackgroundActiveFrameActive] = {pill};
    mixer[kColorTabBackgroundActiveFrameInactive] = {pill};
    mixer[kColorTabForegroundActiveFrameActive] = {fg};
    mixer[kColorTabForegroundActiveFrameInactive] = {fg};
    mixer[kColorTabForegroundInactiveFrameActive] = {fg2};
    mixer[kColorTabForegroundInactiveFrameInactive] = {fg2};
    // The + after the tabs: an icon, no disc (it grew one on the picture).
    mixer[kColorNewTabButtonForegroundFrameActive] = {fg2};
    mixer[kColorNewTabButtonForegroundFrameInactive] = {fg2};
    mixer[kColorNewTabButtonCRForegroundFrameActive] = {fg2};
    mixer[kColorNewTabButtonCRForegroundFrameInactive] = {fg2};
    mixer[kColorNewTabButtonBackgroundFrameActive] = {SK_ColorTRANSPARENT};
    mixer[kColorNewTabButtonBackgroundFrameInactive] = {SK_ColorTRANSPARENT};
    mixer[kColorNewTabButtonCRBackgroundFrameActive] = {SK_ColorTRANSPARENT};
    mixer[kColorNewTabButtonCRBackgroundFrameInactive] = {SK_ColorTRANSPARENT};
    // Back, forward, the menu; disabled ones stay visible, just quieter.
    mixer[kColorToolbarButtonIcon] = {fg2};
    mixer[kColorToolbarButtonIconHovered] = {fg};
    mixer[kColorToolbarButtonIconPressed] = {fg};
    mixer[kColorToolbarButtonIconInactive] = {faint};
    mixer[kColorToolbarButtonIconDisabled] = {faint};
    // The window buttons pick their icon colour from this, and the toolbar's
    // ink drops contrast with it; the row itself paints the picture.
    if (light) {
      mixer[kColorToolbar] = {SkColorSetRGB(0xE8, 0xE8, 0xE8)};
    }
    // The address field: the design's dark pill (#0a0a0a at 60%).
    mixer[kColorLocationBarBackground] = {SkColorSetARGB(0x99, 0x0A, 0x0A, 0x0A)};
    mixer[kColorLocationBarBackgroundHovered] = {
        SkColorSetARGB(0xB3, 0x0A, 0x0A, 0x0A)};
  }

  // The page area before a page paints (a new site loading, a blank or
  // crashed renderer). Chromium's grey here read as a separate sheet under the
  // row; it's the row's colour instead, so a loading page looks like the row
  // running down until the site arrives. ContentsWebView repaints it whenever
  // the window's colours change. It must be opaque (RenderWidgetHostView::
  // SetBackgroundColor).
  SkColor page_area = p.chrome;
  if (IsOneRowWindow() && own_row && SkColorGetA(*own_row) == SK_AlphaOPAQUE) {
    if (const std::optional<site_color::Row> row = site_color::RowFor(*own_row)) {
      AddSiteRow(mixer, *row);
      page_area = row->bar;
    }
  }
  mixer[kColorWebContentsBackground] = {page_area};

  // The download button's progress ring: Chromium picks a Google colour near
  // the throbber with barely visible contrast (a pale yellow on a light row).
  // It's the row's own strong icon colour instead, whatever the row is.
  mixer[kColorDownloadToolbarButtonActive] = {kColorToolbarButtonIconHovered};
  mixer[kColorDownloadToolbarButtonAnimationForeground] = {
      kColorToolbarButtonIconHovered};

  // The + after the tabs hovers like every other button in the row (back,
  // forward, extensions, the menu). Chromium gives tab strip buttons their own
  // ink drop, built from the new tab button's colours: an opaque dark grey.
  for (ui::ColorId id : std::initializer_list<ui::ColorId>{
           kColorTabStripControlButtonInkDrop,
           kColorTabStripControlButtonInkDropFrameInactive}) {
    mixer[id] = {kColorToolbarInkDropHover};
  }
  for (ui::ColorId id : std::initializer_list<ui::ColorId>{
           kColorTabStripControlButtonInkDropRipple,
           kColorTabStripControlButtonInkDropRippleFrameInactive}) {
    mixer[id] = {kColorToolbarInkDropRipple};
  }
}

}  // namespace mast
