// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_block_bubble.h"

#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/controls/rich_hover_button.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"
#include "mast/ui/color/mast_palette.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_combobox_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/color/color_variant.h"
#include "ui/gfx/text_constants.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/focus_ring.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_tracker.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace mast {

namespace {

// Mast's block mark: a circle with a slash (the block page uses the same).
constexpr gfx::PathElement kBlockPath[] = {
    gfx::CANVAS_DIMENSIONS, 20,
    gfx::STROKE, 1.5f,
    gfx::CIRCLE, 10, 10, 7,
    gfx::NEW_PATH,
    gfx::STROKE, 1.5f,
    gfx::MOVE_TO, 5.2f, 14.8f,
    gfx::LINE_TO, 14.8f, 5.2f};
constexpr gfx::VectorIconRep kBlockReps[] = {{kBlockPath}};
constexpr gfx::VectorIcon kBlockIcon(kBlockReps, std::size(kBlockReps),
                                     "mast_block");

// Mast's bubbles: square-ish corners and left-aligned text (views::Label
// centres by default).
constexpr int kCornerRadius = 8;

std::unique_ptr<views::Label> Text(std::u16string text) {
  auto label = std::make_unique<views::Label>(std::move(text));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetMultiLine(true);
  return label;
}

std::u16string DurationLabel(Duration d) {
  switch (d) {
    case Duration::kHour:
      return u"For 1 hour";
    case Duration::kFourHours:
      return u"For 4 hours";
    case Duration::kToday:
      return u"Until midnight";
    case Duration::kWeek:
      return u"For a week";
    case Duration::kMonth:
      return u"For a month";
    case Duration::kForever:
      return u"Forever";
  }
}

// "forever", "until midnight", "until 18:20", "until Nov 10".
std::u16string UntilText(const std::optional<base::Time>& until) {
  if (!until) {
    return u"forever";
  }
  const base::Time now = base::Time::Now();
  if (*until == *UntilFor(Duration::kToday, now)) {
    return u"until midnight";
  }
  base::Time::Exploded e;
  until->LocalExplode(&e);
  if (until->LocalMidnight() == now.LocalMidnight()) {
    return u"until " + base::NumberToString16(e.hour) + u":" +
           (e.minute < 10 ? u"0" : u"") + base::NumberToString16(e.minute);
  }
  static constexpr const char16_t* kMonths[] = {
      u"Jan", u"Feb", u"Mar", u"Apr", u"May", u"Jun",
      u"Jul", u"Aug", u"Sep", u"Oct", u"Nov", u"Dec"};
  return u"until " + std::u16string(kMonths[e.month - 1]) + u" " +
         base::NumberToString16(e.day_of_month);
}

// The site-info icon at the start of the address bar of `web_contents`' window.
views::View* PageInfoAnchor(content::WebContents* web_contents) {
  tabs::TabInterface* tab =
      web_contents ? tabs::TabInterface::MaybeGetFromContents(web_contents)
                   : nullptr;
  BrowserWindowInterface* window =
      tab ? tab->GetBrowserWindowInterface() : nullptr;
  BrowserView* browser_view =
      window ? BrowserView::GetBrowserViewForBrowser(window) : nullptr;
  LocationBarView* location_bar =
      browser_view ? browser_view->GetLocationBarView() : nullptr;
  return location_bar ? location_bar->location_icon_view() : nullptr;
}

void ShowUndo(std::unique_ptr<views::ViewTracker> anchor, std::u16string text);

// Dialog buttons get a hairline focus ring with its own colour id, which skips
// Chromium's 3:1 contrast lift (that turned the ring white around ember).
void QuietFocusRings(views::DialogDelegate* dialog) {
  for (views::MdTextButton* b : {dialog->GetOkButton(), dialog->GetCancelButton()}) {
    if (b && views::FocusRing::Get(b)) {
      views::FocusRing::Get(b)->SetColorId(kColorLocationBarBorder);
    }
  }
}

}  // namespace

// "Blocked youtube.com until midnight", with Undo for the first 10 seconds.
class MastUndoBubble : public views::BubbleDialogDelegateView {
  METADATA_HEADER(MastUndoBubble, views::BubbleDialogDelegateView)

 public:
  MastUndoBubble(views::View* anchor, std::u16string text)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_LEFT) {
    set_corner_radius(kCornerRadius);
    SetShowCloseButton(false);
    SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical));
    AddChildView(Text(std::move(text)));
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk));
    SetButtonLabel(ui::mojom::DialogButton::kOk, u"Done");
    // No default button: Chromium draws the default one prominent (a light pill).
    SetDefaultButton(static_cast<int>(ui::mojom::DialogButton::kNone));
    SetButtonStyle(ui::mojom::DialogButton::kOk, ui::ButtonStyle::kTonal);
    SetExtraView(std::make_unique<views::MdTextButton>(
        base::BindRepeating(&MastUndoBubble::OnUndo, base::Unretained(this)),
        u"Undo"));
    close_timer_.Start(FROM_HERE, MastBlocks::kUndoWindow,
                       base::BindOnce(&MastUndoBubble::CloseBubble,
                                      base::Unretained(this)));
  }

 private:
  // views::WidgetDelegate: the dialog buttons exist from here on.
  void OnWidgetInitialized() override {
    BubbleDialogDelegateView::OnWidgetInitialized();
    QuietFocusRings(this);
  }

  void OnUndo() {
    MastBlocks::Get().Undo();
    CloseBubble();
  }

  void CloseBubble() {
    if (GetWidget()) {
      GetWidget()->Close();
    }
  }

  base::OneShotTimer close_timer_;
};

BEGIN_METADATA(MastUndoBubble)
END_METADATA

// Choose how long to block the site. Durations that would shorten an existing
// block aren't offered.
class MastBlockBubble : public views::BubbleDialogDelegateView {
  METADATA_HEADER(MastBlockBubble, views::BubbleDialogDelegateView)

 public:
  MastBlockBubble(views::View* anchor, std::string domain)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_LEFT),
        domain_(std::move(domain)) {
    anchor_.SetView(anchor);
    set_corner_radius(kCornerRadius);
    const std::optional<Block> current = MastBlocks::Get().Of(domain_);
    const std::u16string domain16 = base::UTF8ToUTF16(domain_);
    SetShowCloseButton(true);
    SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(), 12));

    if (current && current->forever()) {
      SetTitle(u"Blocked forever");
      AddChildView(Text(domain16 + u" will stay blocked. This can’t change."));
      SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk));
      SetButtonLabel(ui::mojom::DialogButton::kOk, u"Done");
      return;
    }

    SetTitle(current ? u"Blocked " + UntilText(current->until)
                     : u"Block " + domain16);
    AddChildView(Text(current ? u"You can make this longer, never shorter."
                              : u"Choose how long."));

    std::vector<ui::SimpleComboboxModel::Item> items;
    const base::Time now = base::Time::Now();
    const std::optional<Duration> last =
        DurationFromId(MastBlocks::Get().last_duration());
    std::optional<size_t> selected;
    for (Duration d : kDurations) {
      if (current && !Outlasts(UntilFor(d, now), current->until)) {
        continue;
      }
      if (d == last || (!selected && d == Duration::kToday)) {
        selected = choices_.size();
      }
      choices_.push_back(d);
      items.emplace_back(DurationLabel(d));
    }
    combobox_ = AddChildView(std::make_unique<views::Combobox>(
        std::make_unique<ui::SimpleComboboxModel>(std::move(items))));
    combobox_->SetSelectedIndex(selected.value_or(0));
    combobox_->GetViewAccessibility().SetName(u"How long");

    SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk) |
               static_cast<int>(ui::mojom::DialogButton::kCancel));
    SetButtonLabel(ui::mojom::DialogButton::kOk, current ? u"Extend" : u"Block");
    SetAcceptCallback(
        base::BindOnce(&MastBlockBubble::OnAccept, base::Unretained(this)));
    signal_ok_ = true;
  }

  // views::WidgetDelegate: the dialog buttons exist from here on.
  void OnWidgetInitialized() override {
    BubbleDialogDelegateView::OnWidgetInitialized();
    // Block is white, like Mast's pages' primary buttons: ember is kept for
    // blocked states, so it keeps meaning blocked.
    if (views::MdTextButton* ok = GetOkButton(); ok && signal_ok_) {
      ok->SetBgColorOverrideDeprecated(palette::Current().text);
      ok->SetEnabledTextColors(ui::ColorVariant(palette::Current().chrome2));
    }
    QuietFocusRings(this);
  }

 private:
  void OnAccept() {
    const std::optional<size_t> index = combobox_->GetSelectedIndex();
    if (!index || *index >= choices_.size()) {
      return;
    }
    const Duration d = choices_[*index];
    const std::optional<base::Time> until = UntilFor(d, base::Time::Now());
    MastBlocks& blocks = MastBlocks::Get();
    blocks.set_last_duration(DurationId(d));
    if (blocks.BlockSites({domain_}, until) == 0) {
      return;
    }
    // Show the undo bubble from the same icon once this one has closed.
    auto anchor = std::make_unique<views::ViewTracker>(anchor_.view());
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&ShowUndo, std::move(anchor),
                       u"Blocked " + base::UTF8ToUTF16(domain_) + u" " +
                           UntilText(until)));
  }

  std::string domain_;
  bool signal_ok_ = false;
  views::ViewTracker anchor_;
  std::vector<Duration> choices_;
  raw_ptr<views::Combobox> combobox_ = nullptr;
};

BEGIN_METADATA(MastBlockBubble)
END_METADATA

namespace {

void ShowUndo(std::unique_ptr<views::ViewTracker> anchor, std::u16string text) {
  if (!anchor->view() || !anchor->view()->GetWidget()) {
    return;  // the window closed meanwhile
  }
  views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<MastUndoBubble>(anchor->view(), std::move(text)))
      ->Show();
}

void ShowBlockBubble(base::WeakPtr<content::WebContents> web_contents) {
  if (!web_contents) {
    return;
  }
  const GURL url = web_contents->GetLastCommittedURL();
  views::View* anchor = PageInfoAnchor(web_contents.get());
  if (!anchor || !url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  // Block the list entry that covers this host (youtube.com for m.youtube.com).
  const std::optional<BlockMatch> match = MastBlocks::Get().ForUrl(url);
  std::string domain = match ? match->domain : NormalizeHost(url.host());
  views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<MastBlockBubble>(anchor, std::move(domain)))
      ->Show();
}

}  // namespace

std::unique_ptr<views::View> CreateBlockSiteRow(
    content::WebContents* web_contents,
    base::RepeatingClosure close_page_info) {
  const GURL url =
      web_contents ? web_contents->GetLastCommittedURL() : GURL();
  const bool blocked = MastBlocks::Get().IsBlocked(url);
  auto row = std::make_unique<RichHoverButton>(
      base::BindRepeating(
          [](base::WeakPtr<content::WebContents> web_contents,
             base::RepeatingClosure close_page_info) {
            close_page_info.Run();
            // After page info has closed, so the new bubble takes the anchor.
            base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
                FROM_HERE, base::BindOnce(&ShowBlockBubble, web_contents));
          },
          web_contents ? web_contents->GetWeakPtr()
                       : base::WeakPtr<content::WebContents>(),
          std::move(close_page_info)),
      PageInfoViewFactory::GetImageModel(kBlockIcon),
      /*title_text=*/blocked ? u"Blocked" : u"Block this site",
      /*subtitle_text=*/blocked ? u"Make the block longer" : std::u16string(),
      PageInfoViewFactory::GetOpenSubpageIcon());
  row->SetTitleTextStyleAndColor(views::style::STYLE_BODY_3_MEDIUM,
                                 kColorPageInfoForeground);
  row->SetSubtitleTextStyleAndColor(views::style::STYLE_BODY_4,
                                    kColorPageInfoSubtitleForeground);
  row->SetTooltipText(u"Block this site for a while, or forever");
  row->SetVisible(url.SchemeIsHTTPOrHTTPS());
  return row;
}

}  // namespace mast
