// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_TOOLBAR_BUTTON_H_
#define MAST_BROWSER_UI_MAST_TOOLBAR_BUTTON_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BrowserWindowInterface;

namespace mast {

// The live session countdown (ember pill); hidden when no session is running.
// Opens the Mast new tab page, where sessions start and the blocklist lives.
class MastToolbarButton : public ToolbarButton {
  METADATA_HEADER(MastToolbarButton, ToolbarButton)

 public:
  explicit MastToolbarButton(BrowserWindowInterface* browser);
  MastToolbarButton(const MastToolbarButton&) = delete;
  MastToolbarButton& operator=(const MastToolbarButton&) = delete;
  ~MastToolbarButton() override;

 private:
  // ToolbarButton: an ember-tinted pill with light ember text during a session
  // (by default the text takes the highlight colour too and disappears).
  bool ShouldBlendHighlightColor() const override;
  std::optional<SkColor> GetHighlightTextColor() const override;

  void ButtonPressed();
  void Update();

  raw_ptr<BrowserWindowInterface> browser_;
  base::RepeatingTimer timer_;
  std::u16string shown_;
};

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_TOOLBAR_BUTTON_H_
