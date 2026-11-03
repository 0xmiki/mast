// Copyright 2026 The Mast Authors

#include "mast/browser/mast_ui.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/url_constants.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_blocks.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "url/gurl.h"

namespace mast {

namespace {

std::string StringArg(const base::ListValue& args, size_t i = 0) {
  return args.size() > i && args[i].is_string() ? args[i].GetString()
                                                 : std::string();
}

double Ms(base::Time t) {
  return static_cast<double>(t.InMillisecondsSinceUnixEpoch());
}

base::Value BlockValue(const Block& block) {
  base::DictValue v;
  v.Set("since", Ms(block.since));
  if (block.until) {
    v.Set("until", Ms(*block.until));
  } else {
    v.Set("until", base::Value());
  }
  return base::Value(std::move(v));
}

// The curated categories never change while the browser runs; build them once.
const base::ListValue& CategoriesValue() {
  static const base::NoDestructor<base::ListValue> kValue([] {
    base::ListValue categories;
    for (const Category& c : Categories()) {
      base::ListValue services;
      for (const Service& s : c.services) {
        base::ListValue domains;
        for (const std::string& d : s.domains) {
          domains.Append(d);
        }
        services.Append(base::DictValue().Set("name", s.name).Set(
            "domains", std::move(domains)));
      }
      categories.Append(base::DictValue()
                            .Set("id", c.id)
                            .Set("name", c.name)
                            .Set("services", std::move(services)));
    }
    return categories;
  }());
  return *kValue;
}

class MastHandler : public content::WebUIMessageHandler {
 public:
  MastHandler() = default;
  MastHandler(const MastHandler&) = delete;
  MastHandler& operator=(const MastHandler&) = delete;
  ~MastHandler() override = default;

  // content::WebUIMessageHandler:
  void RegisterMessages() override {
    On("getState", &MastHandler::HandleGetState);
    On("block", &MastHandler::HandleBlock);
    On("undo", &MastHandler::HandleUndo);
    On("addSite", &MastHandler::HandleAddSite);
    On("removeSite", &MastHandler::HandleRemoveSite);
  }

 private:
  using Handler = void (MastHandler::*)(const base::ListValue&);

  void On(const char* message, Handler handler) {
    web_ui()->RegisterMessageCallback(
        message, base::BindRepeating(handler, base::Unretained(this)));
  }

  void HandleGetState(const base::ListValue& args) { SendState(); }

  // ["block", [domains...], durationId]
  void HandleBlock(const base::ListValue& args) {
    MastBlocks& blocks = MastBlocks::Get();
    std::vector<std::string> domains;
    if (!args.empty() && args[0].is_list()) {
      for (const base::Value& d : args[0].GetList()) {
        if (d.is_string()) {
          domains.push_back(d.GetString());
        }
      }
    }
    const std::optional<Duration> duration = DurationFromId(StringArg(args, 1));
    base::DictValue result;
    if (duration && !domains.empty()) {
      const std::optional<base::Time> until =
          UntilFor(*duration, base::Time::Now());
      blocks.set_last_duration(DurationId(*duration));
      result.Set("changed", blocks.BlockSites(domains, until));
      result.Set("until", until ? base::Value(Ms(*until)) : base::Value());
    }
    SendState(std::move(result));
  }

  void HandleUndo(const base::ListValue& args) {
    MastBlocks::Get().Undo();
    SendState(base::DictValue().Set("undone", true));
  }

  void HandleAddSite(const base::ListValue& args) {
    MastBlocks::Get().AddSite(StringArg(args));
    SendState();
  }

  void HandleRemoveSite(const base::ListValue& args) {
    MastBlocks::Get().RemoveSite(StringArg(args));
    SendState();
  }

  void SendState(base::DictValue result = {}) {
    AllowJavascript();
    MastBlocks& blocks = MastBlocks::Get();
    base::DictValue active;
    for (const auto& [domain, block] : blocks.Active()) {
      active.Set(domain, BlockValue(block));
    }
    base::ListValue custom;
    for (const std::string& site : blocks.CustomSites()) {
      custom.Append(site);
    }
    base::DictValue state;
    state.Set("categories", CategoriesValue().Clone());
    state.Set("custom", std::move(custom));
    state.Set("blocks", std::move(active));
    state.Set("lastDuration", blocks.last_duration());
    state.Set("canUndo", blocks.CanUndo());
    state.Set("stopped", blocks.StoppedToday());
    state.Set("now", Ms(base::Time::Now()));
    state.Set("result", std::move(result));
    CallJavascriptFunction("mastState", base::Value(std::move(state)));
  }
};

}  // namespace

MastUIConfig::MastUIConfig(std::string_view host)
    : WebUIConfig(content::kChromeUIScheme, host) {}

std::unique_ptr<content::WebUIController> MastUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<MastUI>(web_ui, host());
}

MastUI::MastUI(content::WebUI* web_ui, std::string_view host)
    : content::WebUIController(web_ui) {
  const bool as_new_tab = host != kMastHost;
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), std::string(host));
  // The page is one self-contained document with inline script and style.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-inline';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline';");
  source->DisableTrustedTypesCSP();
  source->SetRequestFilter(
      base::BindRepeating([](const std::string& path) {
        return path.empty() || path == "index.html";
      }),
      base::BindRepeating(
          [](bool as_new_tab, const std::string& path,
             content::WebUIDataSource::GotDataCallback callback) {
            std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
                StartPageHtml(as_new_tab)));
          },
          as_new_tab));
  web_ui->AddMessageHandler(std::make_unique<MastHandler>());
}

MastUI::~MastUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(MastUI)

}  // namespace mast
