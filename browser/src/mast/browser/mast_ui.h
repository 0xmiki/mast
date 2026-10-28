// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_UI_H_
#define MAST_BROWSER_MAST_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

namespace mast {

inline constexpr char kMastHost[] = "mast";

// chrome://mast — start a session, watch the countdown, manage the blocklist.
class MastUI : public content::WebUIController {
 public:
  explicit MastUI(content::WebUI* web_ui);
  MastUI(const MastUI&) = delete;
  MastUI& operator=(const MastUI&) = delete;
  ~MastUI() override;

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class MastUIConfig : public content::DefaultWebUIConfig<MastUI> {
 public:
  MastUIConfig();
};

}  // namespace mast

#endif  // MAST_BROWSER_MAST_UI_H_
