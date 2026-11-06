// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_WEBUI_MAST_UI_H_
#define MAST_BROWSER_WEBUI_MAST_UI_H_

#include <string>
#include <string_view>

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

namespace mast {

inline constexpr char kMastHost[] = "mast";

// chrome://mast — the blocklist: pick categories or sites, choose how long, block.
// Also serves the new tab page (chrome://new-tab-page[-third-party]).
class MastUI : public content::WebUIController {
 public:
  MastUI(content::WebUI* web_ui, std::string_view host);
  MastUI(const MastUI&) = delete;
  MastUI& operator=(const MastUI&) = delete;
  ~MastUI() override;

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class MastUIConfig : public content::WebUIConfig {
 public:
  explicit MastUIConfig(std::string_view host);

  // content::WebUIConfig:
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace mast

#endif  // MAST_BROWSER_WEBUI_MAST_UI_H_
