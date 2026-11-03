// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_toolbar_button.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"
#include "mast/ui/color/mast_palette.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_combobox_model.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace mast {

namespace {

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
  if (until->LocalMidnight() == now.LocalMidnight()) {
    return u"until " + base::TimeFormatTimeOfDay(*until);
  }
  static constexpr const char16_t* kMonths[] = {
      u"Jan", u"Feb", u"Mar", u"Apr", u"May", u"Jun",
      u"Jul", u"Aug", u"Sep", u"Oct", u"Nov", u"Dec"};
  base::Time::Exploded e;
  until->LocalExplode(&e);
  return u"until " + std::u16string(kMonths[e.month - 1]) + u" " +
         base::NumberToString16(e.day_of_month);
}

}  // namespace

// Choose how long to block the current site. Durations that would shorten an
// existing block aren't offered.
class MastBlockBubble : public views::BubbleDialogDelegateView {
  METADATA_HEADER(MastBlockBubble, views::BubbleDialogDelegateView)

 public:
  MastBlockBubble(views::View* anchor,
              std::string domain,
              base::WeakPtr<MastToolbarButton> button)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT),
        domain_(std::move(domain)),
        button_(std::move(button)) {
    const std::optional<Block> current = MastBlocks::Get().Of(domain_);
    const std::u16string domain16 = base::UTF8ToUTF16(domain_);
    SetShowCloseButton(true);
    SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(), 12));

    if (current && current->forever()) {
      SetTitle(u"Blocked forever");
      AddChildView(std::make_unique<views::Label>(
          domain16 + u" will stay blocked. This can’t change."));
      SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk));
      SetButtonLabel(ui::mojom::DialogButton::kOk, u"Done");
      return;
    }

    SetTitle(current ? u"Blocked " + UntilText(current->until)
                     : u"Block " + domain16);
    AddChildView(std::make_unique<views::Label>(
        current ? u"You can make this longer, never shorter."
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
    // Show the undo bubble once this one has closed.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&MastToolbarButton::ShowUndo, button_,
                       u"Blocked " + base::UTF8ToUTF16(domain_) + u" " +
                           UntilText(until)));
  }

  std::string domain_;
  base::WeakPtr<MastToolbarButton> button_;
  std::vector<Duration> choices_;
  raw_ptr<views::Combobox> combobox_ = nullptr;
};

BEGIN_METADATA(MastBlockBubble)
END_METADATA

// "Blocked youtube.com until midnight", with Undo for the first 10 seconds.
class MastUndoBubble : public views::BubbleDialogDelegateView {
  METADATA_HEADER(MastUndoBubble, views::BubbleDialogDelegateView)

 public:
  MastUndoBubble(views::View* anchor, std::u16string text)
      : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT) {
    SetShowCloseButton(false);
    SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical));
    AddChildView(std::make_unique<views::Label>(std::move(text)));
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk));
    SetButtonLabel(ui::mojom::DialogButton::kOk, u"Done");
    SetExtraView(std::make_unique<views::MdTextButton>(
        base::BindRepeating(&MastUndoBubble::OnUndo, base::Unretained(this)),
        u"Undo"));
    close_timer_.Start(FROM_HERE, MastBlocks::kUndoWindow,
                       base::BindOnce(&MastUndoBubble::CloseBubble,
                                      base::Unretained(this)));
  }

 private:
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

MastToolbarButton::MastToolbarButton(BrowserWindowInterface* browser)
    : ToolbarButton(base::BindRepeating(&MastToolbarButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_(browser) {
  GetViewAccessibility().SetName(u"Block this site");
  Update();
  timer_.Start(FROM_HERE, base::Seconds(1),
               base::BindRepeating(&MastToolbarButton::Update,
                                   base::Unretained(this)));
}

MastToolbarButton::~MastToolbarButton() = default;

bool MastToolbarButton::ShouldBlendHighlightColor() const {
  return blocked_;
}

std::optional<SkColor> MastToolbarButton::GetHighlightTextColor() const {
  return blocked_ ? std::optional<SkColor>(palette::kEmber) : std::nullopt;
}

content::WebContents* MastToolbarButton::ActiveContents() const {
  tabs::TabInterface* tab = browser_->GetActiveTabInterface();
  return tab ? tab->GetContents() : nullptr;
}

void MastToolbarButton::ButtonPressed() {
  content::WebContents* contents = ActiveContents();
  if (!contents) {
    return;
  }
  const GURL url = contents->GetLastCommittedURL();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  // Block the list entry that covers this host (youtube.com for m.youtube.com).
  const std::optional<BlockMatch> match = MastBlocks::Get().ForUrl(url);
  const std::string domain = match ? match->domain : NormalizeHost(url.host());
  views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<MastBlockBubble>(this, domain, weak_factory_.GetWeakPtr()))
      ->Show();
}

void MastToolbarButton::ShowUndo(std::u16string text) {
  views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<MastUndoBubble>(this, std::move(text)))
      ->Show();
  Update();
}

void MastToolbarButton::Update() {
  content::WebContents* contents = ActiveContents();
  const GURL url = contents ? contents->GetLastCommittedURL() : GURL();
  // Only for websites; Mast's own pages and settings have nothing to block.
  SetVisible(url.SchemeIsHTTPOrHTTPS());
  const bool blocked = MastBlocks::Get().IsBlocked(url);
  const std::u16string text = blocked ? u"Blocked" : u"Block";
  if (text == shown_ && blocked == blocked_) {
    return;
  }
  shown_ = text;
  blocked_ = blocked;
  SetHighlight(text, blocked ? std::optional<SkColor>(palette::kEmber)
                             : std::nullopt);
  SetTooltipText(blocked ? u"This site is blocked" : u"Block this site");
}

BEGIN_METADATA(MastToolbarButton)
END_METADATA

}  // namespace mast
