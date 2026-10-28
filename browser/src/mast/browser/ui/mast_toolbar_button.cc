// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_toolbar_button.h"

#include <algorithm>
#include <optional>

#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "mast/browser/mast_session.h"
#include "mast/ui/color/mast_palette.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "url/gurl.h"

namespace mast {

namespace {

constexpr SkColor kEmber = palette::kEmber;

std::u16string Countdown(base::TimeDelta left) {
  const int64_t s = std::max<int64_t>(0, left.InSecondsF() + 0.999);
  const int64_t h = s / 3600, m = (s % 3600) / 60, sec = s % 60;
  return base::UTF8ToUTF16(
      h ? base::StringPrintf("%d:%02d:%02d", static_cast<int>(h),
                             static_cast<int>(m), static_cast<int>(sec))
        : base::StringPrintf("%02d:%02d", static_cast<int>(m),
                             static_cast<int>(sec)));
}

}  // namespace

MastToolbarButton::MastToolbarButton(BrowserWindowInterface* browser)
    : ToolbarButton(base::BindRepeating(&MastToolbarButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_(browser) {
  SetTooltipText(u"Block distractions");
  GetViewAccessibility().SetName(u"Mast");
  Update();
  timer_.Start(FROM_HERE, base::Seconds(1),
               base::BindRepeating(&MastToolbarButton::Update,
                                   base::Unretained(this)));
}

MastToolbarButton::~MastToolbarButton() = default;

bool MastToolbarButton::ShouldBlendHighlightColor() const {
  return true;
}

std::optional<SkColor> MastToolbarButton::GetHighlightTextColor() const {
  return palette::kEmber2;
}

void MastToolbarButton::ButtonPressed() {
  browser_->OpenGURL(GURL("chrome://newtab/"),
                     WindowOpenDisposition::NEW_FOREGROUND_TAB);
}

void MastToolbarButton::Update() {
  MastSession& session = MastSession::Get();
  const bool active = session.IsActive();
  const std::u16string text =
      active ? Countdown(session.end_time() - base::Time::Now()) : u"Block";
  // Only there during a session, as the countdown; the new tab page is where
  // sessions start.
  SetVisible(active);
  if (text == shown_) {
    return;
  }
  shown_ = text;
  SetHighlight(text, active ? std::optional<SkColor>(kEmber) : std::nullopt);
  SetTooltipText(active ? u"Deep work session running" : u"Block distractions");
}

BEGIN_METADATA(MastToolbarButton)
END_METADATA

}  // namespace mast
