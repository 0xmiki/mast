// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_SHORTCUTS_H_
#define MAST_BROWSER_UI_MAST_SHORTCUTS_H_

#include <map>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "url/gurl.h"

class BrowserWindowInterface;

namespace mast {

// The user's Alt + letter shortcuts (kShortcutsPref, the blocklist panel's
// Shortcuts tab, design C32) for one window: each opens its page in a new tab.
// They're registered with the window's FocusManager the way extensions'
// keyboard shortcuts are, so a web page that uses the same keys itself still
// gets them first, and Chromium's own Alt keys are never taken
// (IsShortcutKey).
class MastShortcuts : public ui::AcceleratorTarget,
                      public views::WidgetObserver {
 public:
  // Starts one for a normal window (from OnBrowserWindowReady). It lives until
  // the window closes, and follows changes to the list.
  static void Attach(BrowserWindowInterface* browser);

  MastShortcuts(BrowserWindowInterface* browser, views::Widget* widget);
  MastShortcuts(const MastShortcuts&) = delete;
  MastShortcuts& operator=(const MastShortcuts&) = delete;
  ~MastShortcuts() override;

  // ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool CanHandleAccelerators() const override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

 private:
  void Register();
  void Unregister();

  const raw_ptr<BrowserWindowInterface> browser_;
  raw_ptr<views::Widget> widget_;
  std::map<ui::Accelerator, GURL> urls_;
  PrefChangeRegistrar prefs_;
  base::CallbackListSubscription did_close_;
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};
};

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_SHORTCUTS_H_
