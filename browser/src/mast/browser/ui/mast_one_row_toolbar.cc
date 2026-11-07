// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_one_row_toolbar.h"

#include <optional>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/reload_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "mast/browser/mast_prefs.h"
#include "mast/browser/mast_theme.h"
#include "mast/browser/ui/mast_tab_bar.h"
#include "mast/common/mast_features.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"

namespace mast {

namespace {

// Mast has no bookmarks bar (saved sites live on the new tab page, and its
// toggle is gone from the menu and Ctrl+Shift+B). Profiles that showed it keep
// it hidden.
void HideBookmarksBar(PrefService* prefs) {
  if (prefs->FindPreference(bookmarks::prefs::kShowBookmarkBar)) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
  }
  if (prefs->FindPreference(bookmarks::prefs::kBookmarkBarVisibilityState)) {
    prefs->SetInteger(
        bookmarks::prefs::kBookmarkBarVisibilityState,
        static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysHide));
  }
}

}  // namespace

void OnBrowserWindowReady(BrowserWindowInterface* browser) {
  // Customize's palette and backdrop, before the window first paints. Later
  // picks apply from the new tab page (mast_ui.cc).
  if (browser && browser->GetProfile()) {
    ApplyTheme(browser->GetProfile()->GetPrefs());
    HideBookmarksBar(browser->GetProfile()->GetPrefs());
  }
  if (!IsOneRowWindow() || !browser ||
      browser->GetType() != BrowserWindowInterface::TYPE_NORMAL) {
    return;
  }
  // Customize's "Tab bar on websites" for this window.
  MastTabBar::Attach(browser);
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  ToolbarView* toolbar = browser_view ? browser_view->toolbar() : nullptr;
  LocationBarView* location_bar =
      toolbar ? toolbar->location_bar_view() : nullptr;
  if (!location_bar) {
    return;
  }

  // The address bar has a fixed width instead of filling the row: the tabs
  // take the room (the layout puts the toolbar at the end of the tab row).
  location_bar->SetPreferredSize(
      gfx::Size(kOneRowLocationBarWidth,
                location_bar->GetPreferredSize().height()));
  location_bar->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));

  // back, address, forward (the design's ← [ address ☆ ] →).
  if (views::View* forward = toolbar->forward_button()) {
    if (const std::optional<size_t> at = toolbar->GetIndexOf(location_bar)) {
      toolbar->ReorderChildView(forward, *at + 1);
    }
  }
  // No reload button in the one row; Ctrl+R and F5 still reload.
  if (views::View* reload = toolbar->reload_button()) {
    reload->SetVisible(false);
  }
  toolbar->InvalidateLayout();
  browser_view->InvalidateLayout();
}

}  // namespace mast
