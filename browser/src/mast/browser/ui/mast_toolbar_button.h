// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_TOOLBAR_BUTTON_H_
#define MAST_BROWSER_UI_MAST_TOOLBAR_BUTTON_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BrowserWindowInterface;

namespace content {
class WebContents;
}

namespace mast {

// "Block" for the site in the active tab, or "Blocked" (in the signal colour)
// when that site is blocked. Opens a small bubble to choose how long; right
// after blocking, a second bubble offers the 10-second undo. Hidden on pages
// that aren't websites (new tab, settings).
class MastToolbarButton : public ToolbarButton {
  METADATA_HEADER(MastToolbarButton, ToolbarButton)

 public:
  explicit MastToolbarButton(BrowserWindowInterface* browser);
  MastToolbarButton(const MastToolbarButton&) = delete;
  MastToolbarButton& operator=(const MastToolbarButton&) = delete;
  ~MastToolbarButton() override;

  // Shows the undo bubble for a block that just happened.
  void ShowUndo(std::u16string text);

 private:
  // ToolbarButton: blocked sites read in the signal colour.
  bool ShouldBlendHighlightColor() const override;
  std::optional<SkColor> GetHighlightTextColor() const override;

  content::WebContents* ActiveContents() const;
  void ButtonPressed();
  void Update();

  raw_ptr<BrowserWindowInterface> browser_;
  base::RepeatingTimer timer_;
  std::u16string shown_;
  bool blocked_ = false;
  base::WeakPtrFactory<MastToolbarButton> weak_factory_{this};
};

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_TOOLBAR_BUTTON_H_
