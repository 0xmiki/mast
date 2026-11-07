// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_TAB_BAR_H_
#define MAST_BROWSER_UI_MAST_TAB_BAR_H_

#include <map>
#include <optional>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/skia/include/core/SkColor.h"

class BrowserView;
class BrowserWindowInterface;

namespace mast {

// Customize's "Tab bar on websites" for one one-row window (design C23):
//   - "picture": Customize's picture behind the tabs everywhere (as before);
//   - "theme": the picture on the new tab page, the palette on websites;
//   - "site": each website's own colour, so the site runs up to the top.
// Mast's own pages (the new tab page, Settings...), error pages and the
// blocked page always keep the picture or palette.
//
// The decision is published per window (mast/common/mast_tab_bar_state.h) and
// the window rebuilt its colours the way an installed web app's window does
// when its theme-color changes (AppBrowserController::UpdateThemeSupplier):
// BrowserWidget::GetColorProviderKey() puts it in the key, the colour mixer
// (mast_chrome_color_mixer.cc) builds the row from it, and the frame skips the
// picture.
//
// "Match site" colour: the pixels actually at the top of the page (a 2px strip,
// copied like tab hover cards' thumbnails), taken once the page first paints,
// when it loads, when its theme-color changes and every few seconds while the
// tab is in front (headers change as you scroll). A strip that isn't one
// colour (a picture header) or a colour that wouldn't work
// (mast/ui/color/mast_site_color.h) falls back to the palette. The page's
// theme-color stands in until the first strip arrives. Each tab keeps its
// colour, so switching tabs never flashes; a page keeps its colour until a
// changed one is seen twice in a row, so scrolling and animations don't flicker.
class MastTabBar : public TabStripModelObserver,
                   public content::WebContentsObserver {
 public:
  // Starts one for a normal, one-row, on-the-record window (from
  // OnBrowserWindowReady). It lives until the window closes.
  static void Attach(BrowserWindowInterface* browser);

  explicit MastTabBar(BrowserWindowInterface* browser);
  MastTabBar(const MastTabBar&) = delete;
  MastTabBar& operator=(const MastTabBar&) = delete;
  ~MastTabBar() override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(TabStripModel* tab_strip_model,
                              const TabStripModelChange& change,
                              const TabStripSelectionChange& selection) override;

  // content::WebContentsObserver (the active tab):
  void PrimaryPageChanged(content::Page& page) override;
  void DidFirstVisuallyNonEmptyPaint() override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void DidStopLoading() override;
  void DidChangeThemeColor() override;
  void OnBackgroundColorChanged() override;

 private:
  struct Site {
    std::optional<SkColor> color;    // the row's colour; none: the palette
    bool sampled = false;            // `color` came from the page's pixels
    bool stale = false;              // a new page: adopt its first sample
    // A changed sample seen once (which may be "no colour"), waiting for a
    // second look before the row follows it.
    std::optional<std::optional<SkColor>> pending;
  };

  // Re-decides the row for the active tab; repaints the window if it changed.
  void Update();
  // What the row sits on for the active tab (see mast_tab_bar_state.h).
  std::optional<uint32_t> Decide();
  bool MatchingSites() const;
  void ScheduleSample(base::TimeDelta delay);
  void Sample();
  void OnSampled(base::WeakPtr<content::WebContents> contents,
                 const content::CopyFromSurfaceResult& result);
  // The site's declared theme-color, until its pixels are known.
  void TakeThemeColor();

  const raw_ptr<BrowserWindowInterface> browser_;
  const raw_ptr<BrowserView> browser_view_;
  std::map<content::WebContents*, Site> sites_;
  std::optional<uint32_t> published_;
  PrefChangeRegistrar prefs_;
  base::OneShotTimer sample_soon_;
  base::RepeatingTimer resample_;
  base::CallbackListSubscription did_close_;
  base::WeakPtrFactory<MastTabBar> weak_factory_{this};
};

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_TAB_BAR_H_
