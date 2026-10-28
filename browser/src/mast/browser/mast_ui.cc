// Copyright 2026 The Mast Authors

#include "mast/browser/mast_ui.h"

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/url_constants.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_session.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace mast {

namespace {

int IntArg(const base::ListValue& args, int fallback) {
  if (args.empty()) {
    return fallback;
  }
  if (args[0].is_int()) {
    return args[0].GetInt();
  }
  if (args[0].is_double()) {
    return static_cast<int>(args[0].GetDouble());
  }
  return fallback;
}

std::string StringArg(const base::ListValue& args) {
  return !args.empty() && args[0].is_string() ? args[0].GetString()
                                              : std::string();
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
    On("start", &MastHandler::HandleStart);
    On("extend", &MastHandler::HandleExtend);
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

  void HandleStart(const base::ListValue& args) {
    MastSession::Get().Start(IntArg(args, MastSession::Get().last_minutes()));
    SendState();
  }

  void HandleExtend(const base::ListValue& args) {
    MastSession::Get().Extend(IntArg(args, 30));
    SendState();
  }

  void HandleUndo(const base::ListValue& args) {
    MastSession::Get().Undo();
    SendState();
  }

  void HandleAddSite(const base::ListValue& args) {
    MastSession::Get().AddSite(StringArg(args));
    SendState();
  }

  void HandleRemoveSite(const base::ListValue& args) {
    MastSession::Get().RemoveSite(StringArg(args));
    SendState();
  }

  void SendState() {
    AllowJavascript();
    MastSession& session = MastSession::Get();
    base::ListValue presets;
    for (const std::string& site : PresetSites()) {
      presets.Append(site);
    }
    base::ListValue custom;
    for (const std::string& site : session.CustomSites()) {
      custom.Append(site);
    }
    base::DictValue state;
    state.Set("active", session.IsActive());
    state.Set("startedAt", static_cast<double>(
                               session.start_time().InMillisecondsSinceUnixEpoch()));
    state.Set("endsAt", static_cast<double>(
                            session.end_time().InMillisecondsSinceUnixEpoch()));
    state.Set("lastMinutes", session.last_minutes());
    state.Set("presets", std::move(presets));
    state.Set("custom", std::move(custom));
    CallJavascriptFunction("mastState", base::Value(std::move(state)));
  }
};

}  // namespace

MastUIConfig::MastUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kMastHost) {}

MastUI::MastUI(content::WebUI* web_ui) : content::WebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kMastHost);
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
          [](const std::string& path,
             content::WebUIDataSource::GotDataCallback callback) {
            std::move(callback).Run(
                base::MakeRefCounted<base::RefCountedString>(StartPageHtml()));
          }));
  web_ui->AddMessageHandler(std::make_unique<MastHandler>());
}

MastUI::~MastUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(MastUI)

}  // namespace mast
