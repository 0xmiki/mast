// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_tab_bar.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/bind_post_task.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window_theme_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "mast/browser/mast_prefs.h"
#include "mast/common/mast_features.h"
#include "mast/common/mast_tab_bar_state.h"
#include "mast/ui/color/mast_site_color.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace mast {

namespace {

// The strip at the top of the page that meets the tab row, in DIPs.
constexpr int kStripHeight = 2;
// Leaves out a scrollbar at the right edge.
constexpr int kStripRightInset = 16;
// Beyond this spread (OKLab, ~0.02 is just noticeable) the strip isn't one
// colour: a picture or gradient header, which no single colour matches.
constexpr float kMaxSpread = 0.035f;
// A sample this close to the current colour isn't a change.
constexpr float kSameColor = 0.012f;
constexpr base::TimeDelta kResampleEvery = base::Seconds(2);

std::map<BrowserWindowInterface*, std::unique_ptr<MastTabBar>>& TabBars() {
  static base::NoDestructor<
      std::map<BrowserWindowInterface*, std::unique_ptr<MastTabBar>>>
      tab_bars;
  return *tab_bars;
}

// The strip's colour, if it is one colour: the per-channel median in OKLab,
// and how far most pixels sit from it.
std::optional<SkColor> StripColor(const SkBitmap& bitmap) {
  if (bitmap.drawsNothing()) {
    return std::nullopt;
  }
  std::vector<float> ls, as, bs;
  for (int y = 0; y < bitmap.height(); ++y) {
    for (int x = 0; x < bitmap.width(); x += 2) {
      float l, a, b;
      site_color::ToOklab(bitmap.getColor(x, y), l, a, b);
      ls.push_back(l);
      as.push_back(a);
      bs.push_back(b);
    }
  }
  if (ls.size() < 8) {
    return std::nullopt;
  }
  auto median = [](std::vector<float> v) {
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    return v[v.size() / 2];
  };
  const float ml = median(ls), ma = median(as), mb = median(bs);
  std::vector<float> spread;
  spread.reserve(ls.size());
  for (size_t i = 0; i < ls.size(); ++i) {
    spread.push_back(std::hypot(ls[i] - ml, std::hypot(as[i] - ma, bs[i] - mb)));
  }
  // The 85th percentile: a few pixels of text or an icon edge don't count.
  std::nth_element(spread.begin(), spread.begin() + spread.size() * 85 / 100,
                   spread.end());
  if (spread[spread.size() * 85 / 100] > kMaxSpread) {
    return std::nullopt;
  }
  return site_color::FromOklab(ml, ma, mb);
}

// A colour the row can take ("Match site"), or nothing for the palette.
std::optional<SkColor> Usable(std::optional<SkColor> color) {
  if (!color || !site_color::RowFor(*color)) {
    return std::nullopt;
  }
  return SkColorSetA(*color, 0xFF);
}

bool Same(const std::optional<SkColor>& a, const std::optional<SkColor>& b) {
  if (a.has_value() != b.has_value()) {
    return false;
  }
  return !a || site_color::Distance(*a, *b) < kSameColor;
}

}  // namespace

// static
void MastTabBar::Attach(BrowserWindowInterface* browser) {
  if (!IsOneRowWindow() || !browser ||
      browser->GetType() != BrowserWindowInterface::TYPE_NORMAL ||
      !browser->GetProfile() || browser->GetProfile()->IsOffTheRecord() ||
      !BrowserView::GetBrowserViewForBrowser(browser) ||
      TabBars().contains(browser)) {
    return;
  }
  TabBars().emplace(browser, std::make_unique<MastTabBar>(browser));
}

MastTabBar::MastTabBar(BrowserWindowInterface* browser)
    : browser_(browser),
      browser_view_(BrowserView::GetBrowserViewForBrowser(browser)) {
  browser_->GetTabStripModel()->AddObserver(this);
  Observe(browser_->GetTabStripModel()->GetActiveWebContents());
  prefs_.Init(browser_->GetProfile()->GetPrefs());
  const auto changed = base::BindRepeating(&MastTabBar::Update,
                                           base::Unretained(this));
  prefs_.Add(kTabBarPref, changed);
  prefs_.Add(kWallpaperPref, changed);
  did_close_ = browser_->RegisterBrowserDidClose(
      base::BindRepeating([](BrowserWindowInterface* browser) {
        TabBars().erase(browser);
      }));
  Update();
}

MastTabBar::~MastTabBar() {
  SetTabBarColor(browser_view_.get(), std::nullopt);
  if (TabStripModel* model = browser_->GetTabStripModel()) {
    model->RemoveObserver(this);
  }
}

bool MastTabBar::MatchingSites() const {
  return browser_->GetProfile()->GetPrefs()->GetString(kTabBarPref) == "site";
}

std::optional<uint32_t> MastTabBar::Decide() {
  content::WebContents* contents = web_contents();
  if (!contents) {
    return std::nullopt;
  }
  PrefService* prefs = browser_->GetProfile()->GetPrefs();
  const std::string mode = prefs->GetString(kTabBarPref);
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  const bool website =
      contents->GetLastCommittedURL().SchemeIsHTTPOrHTTPS() && entry &&
      entry->GetPageType() != content::PAGE_TYPE_ERROR;
  if (!website || mode == "picture") {
    return std::nullopt;  // the picture, or the palette without one
  }
  if (mode == "site") {
    const auto it = sites_.find(contents);
    if (it != sites_.end() && it->second.color) {
      return *it->second.color;
    }
  }
  // The palette. Without a picture that's what the row shows anyway, so the
  // window keeps its usual colours (no needless rebuild).
  return prefs->GetInteger(kWallpaperPref) > 0
             ? std::optional<uint32_t>(SK_ColorTRANSPARENT)
             : std::nullopt;
}

void MastTabBar::Update() {
  const std::optional<uint32_t> next = Decide();
  if (MatchingSites() && web_contents()) {
    if (!resample_.IsRunning()) {
      resample_.Start(FROM_HERE, kResampleEvery,
                      base::BindRepeating(&MastTabBar::Sample,
                                          base::Unretained(this)));
    }
    if (!sites_[web_contents()].sampled) {
      ScheduleSample(base::TimeDelta());
    }
  } else {
    resample_.Stop();
  }
  if (next == published_) {
    return;
  }
  published_ = next;
  SetTabBarColor(browser_view_.get(), next);
  // What an installed web app's window does when its theme-color changes:
  // the window asks for its ColorProvider again and repaints.
  if (auto* theme_observer = BrowserWindowThemeObserver::From(browser_.get())) {
    theme_observer->NotifyThemeChanged(BrowserThemeChangeType::kWebAppTheme);
  }
}

void MastTabBar::OnTabStripModelChanged(TabStripModel* tab_strip_model,
                                        const TabStripModelChange& change,
                                        const TabStripSelectionChange& selection) {
  if (change.type() == TabStripModelChange::kRemoved) {
    for (const auto& removed : change.GetRemove()->contents) {
      sites_.erase(removed.contents);
    }
  } else if (change.type() == TabStripModelChange::kReplaced) {
    sites_.erase(change.GetReplace()->old_contents);
  }
  if (selection.active_tab_changed()) {
    Observe(selection.new_contents);
    if (web_contents() && !sites_[web_contents()].color) {
      TakeThemeColor();
    }
  }
  Update();
}

void MastTabBar::PrimaryPageChanged(content::Page& page) {
  // A new page keeps the last colour until its own is known (no flash).
  Site& site = sites_[web_contents()];
  site.stale = true;
  site.sampled = false;
  site.pending.reset();
  Update();
}

void MastTabBar::DidFirstVisuallyNonEmptyPaint() {
  ScheduleSample(base::Milliseconds(50));
}

void MastTabBar::DocumentOnLoadCompletedInPrimaryMainFrame() {
  ScheduleSample(base::Milliseconds(100));
}

void MastTabBar::DidStopLoading() {
  ScheduleSample(base::Milliseconds(250));
}

void MastTabBar::DidChangeThemeColor() {
  TakeThemeColor();
  ScheduleSample(base::Milliseconds(100));
}

void MastTabBar::OnBackgroundColorChanged() {
  ScheduleSample(base::Milliseconds(100));
}

void MastTabBar::TakeThemeColor() {
  if (!MatchingSites() || !web_contents()) {
    return;
  }
  Site& site = sites_[web_contents()];
  if (site.sampled && !site.stale) {
    return;  // the pixels win
  }
  if (const std::optional<SkColor> color = Usable(web_contents()->GetThemeColor())) {
    site.color = color;
    Update();
  }
}

void MastTabBar::ScheduleSample(base::TimeDelta delay) {
  if (!MatchingSites()) {
    return;
  }
  sample_soon_.Start(FROM_HERE, delay,
                     base::BindOnce(&MastTabBar::Sample, base::Unretained(this)));
}

void MastTabBar::Sample() {
  content::WebContents* contents = web_contents();
  if (!contents || !MatchingSites() ||
      !contents->GetLastCommittedURL().SchemeIsHTTPOrHTTPS() ||
      !contents->CompletedFirstVisuallyNonEmptyPaint()) {
    return;  // nothing of the page on screen yet
  }
  content::RenderWidgetHostView* view = contents->GetRenderWidgetHostView();
  if (!view || !view->IsSurfaceAvailableForCopy()) {
    return;
  }
  const gfx::Size size = view->GetViewBounds().size();
  if (size.width() <= kStripRightInset * 2 || size.height() < kStripHeight) {
    return;
  }
  view->CopyFromSurface(
      gfx::Rect(0, 0, size.width() - kStripRightInset, kStripHeight),
      gfx::Size(), base::TimeDelta(),
      base::BindPostTaskToCurrentDefault(
          base::BindOnce(&MastTabBar::OnSampled, weak_factory_.GetWeakPtr(),
                         contents->GetWeakPtr())));
}

void MastTabBar::OnSampled(base::WeakPtr<content::WebContents> contents,
                           const content::CopyFromSurfaceResult& result) {
  if (!contents || !result.has_value()) {
    return;
  }
  const auto it = sites_.find(contents.get());
  if (it == sites_.end()) {
    return;  // the tab left this window
  }
  Site& site = it->second;
  const std::optional<SkColor> color = Usable(StripColor(result->bitmap));
  if (site.stale || !site.sampled) {
    // The page's first look: take it as it is.
    site.color = color;
    site.stale = false;
    site.sampled = true;
    site.pending.reset();
  } else if (Same(color, site.color)) {
    site.pending.reset();
  } else if (site.pending && Same(*site.pending, color)) {
    // A change on the same page, seen twice in a row: follow it.
    site.color = color;
    site.pending.reset();
  } else {
    site.pending = color;  // seen once
  }
  if (contents.get() == web_contents()) {
    Update();
  }
}

}  // namespace mast
