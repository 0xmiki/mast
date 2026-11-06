// Copyright 2026 The Mast Authors

#include "mast/browser/webui/mast_ui.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/url_constants.h"
#include "mast/browser/desktop/mast_desktop.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_prefs.h"
#include "components/prefs/pref_service.h"
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

// "espn.com/nba" -> https://espn.com/nba. Only web pages go on the desktop.
GURL DesktopUrl(const std::string& input) {
  std::string text(base::TrimWhitespaceASCII(input, base::TRIM_ALL));
  if (text.find("://") == std::string::npos) {
    text = "https://" + text;
  }
  GURL url(text);
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS() && !url.host().empty()
             ? url
             : GURL();
}

// Chromium shows the bookmarks bar on new tabs as soon as any bookmark exists,
// and the desktop's sites are bookmarks. Mast's default is no bar (Ctrl+Shift+B
// still shows it). Set once, only while the user hasn't chosen: a pref default
// can't do it, because the bookmarks prefs register after Mast's profile prefs.
void HideBookmarksBarByDefault(PrefService* prefs) {
  const PrefService::Preference* state =
      prefs->FindPreference(bookmarks::prefs::kBookmarkBarVisibilityState);
  if (state && state->IsDefaultValue()) {
    prefs->SetInteger(
        bookmarks::prefs::kBookmarkBarVisibilityState,
        static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysHide));
  }
}

class MastHandler : public content::WebUIMessageHandler,
                    public bookmarks::BaseBookmarkModelObserver {
 public:
  explicit MastHandler(Profile* profile)
      : profile_(profile),
        bookmarks_(BookmarkModelFactory::GetForBrowserContext(profile)) {
    HideBookmarksBarByDefault(profile->GetPrefs());
  }
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
    On("blockDistractions", &MastHandler::HandleBlockDistractions);
    On("setDistraction", &MastHandler::HandleSetDistraction);
    On("desktopAdd", &MastHandler::HandleDesktopAdd);
    On("desktopRemove", &MastHandler::HandleDesktopRemove);
    On("desktopMove", &MastHandler::HandleDesktopMove);
  }

  void OnJavascriptAllowed() override {
    if (bookmarks_ && !observation_.IsObserving()) {
      observation_.Observe(bookmarks_.get());
    }
  }
  void OnJavascriptDisallowed() override { observation_.Reset(); }

  // bookmarks::BaseBookmarkModelObserver: the desktop is a bookmarks folder,
  // so any change (here, in the bookmark manager, or the star) repaints it.
  void BookmarkModelChanged() override {
    if (IsJavascriptAllowed()) {
      SendState();
    }
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
    std::vector<std::string> domains;
    if (!args.empty() && args[0].is_list()) {
      for (const base::Value& d : args[0].GetList()) {
        if (d.is_string()) {
          domains.push_back(d.GetString());
        }
      }
    }
    Block(domains, StringArg(args, 1));
  }

  // ["blockDistractions", durationId]: everything in the distraction set.
  void HandleBlockDistractions(const base::ListValue& args) {
    Block(MastBlocks::Get().DistractionDomains(), StringArg(args, 0));
  }

  void Block(const std::vector<std::string>& domains,
             const std::string& duration_id) {
    MastBlocks& blocks = MastBlocks::Get();
    const std::optional<Duration> duration = DurationFromId(duration_id);
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

  // ["setDistraction", categoryId, on]
  void HandleSetDistraction(const base::ListValue& args) {
    const bool on = args.size() > 1 && args[1].is_bool() && args[1].GetBool();
    MastBlocks::Get().SetDistraction(StringArg(args, 0), on);
    SendState();
  }

  // ["desktopAdd", url, title]: a new site at the end of the desktop.
  void HandleDesktopAdd(const base::ListValue& args) {
    const GURL url = DesktopUrl(StringArg(args, 0));
    const bookmarks::BookmarkNode* folder = Folder();
    if (url.is_empty() || !folder) {
      return SendState(base::DictValue().Set("invalidSite", true));
    }
    std::string title(base::TrimWhitespaceASCII(StringArg(args, 1), base::TRIM_ALL));
    if (title.empty()) {
      title = NormalizeHost(url.host());
    }
    // The observer repaints the page.
    bookmarks_->AddNewURL(folder, folder->children().size(),
                          base::UTF8ToUTF16(title), url);
  }

  // ["desktopRemove", id]
  void HandleDesktopRemove(const base::ListValue& args) {
    if (const bookmarks::BookmarkNode* node = DesktopNode(StringArg(args, 0))) {
      bookmarks_->Remove(node, bookmarks::metrics::BookmarkEditSource::kUser,
                         FROM_HERE);
    }
  }

  // ["desktopMove", id, index]: index is the site's new position.
  void HandleDesktopMove(const base::ListValue& args) {
    const bookmarks::BookmarkNode* node = DesktopNode(StringArg(args, 0));
    if (!node || args.size() < 2 || !args[1].is_int()) {
      return;
    }
    const bookmarks::BookmarkNode* folder = node->parent();
    const size_t from = *folder->GetIndexOf(node);
    size_t to = static_cast<size_t>(std::max(0, args[1].GetInt()));
    to = std::min(to, folder->children().size() - 1);
    // BookmarkModel::Move counts positions before the node is taken out.
    bookmarks_->Move(node, folder, to > from ? to + 1 : to);
  }

  const bookmarks::BookmarkNode* Folder() {
    return bookmarks_ ? DesktopFolder(bookmarks_.get()) : nullptr;
  }

  // A site on the desktop by its bookmark id (only children of the folder).
  const bookmarks::BookmarkNode* DesktopNode(const std::string& id) {
    const bookmarks::BookmarkNode* folder = Folder();
    int64_t value = 0;
    if (!folder || !base::StringToInt64(id, &value)) {
      return nullptr;
    }
    for (const auto& child : folder->children()) {
      if (child->id() == value && child->is_url()) {
        return child.get();
      }
    }
    return nullptr;
  }

  base::ListValue DesktopValue() {
    base::ListValue sites;
    const bookmarks::BookmarkNode* folder = Folder();
    if (!folder) {
      return sites;
    }
    for (const auto& child : folder->children()) {
      if (!child->is_url()) {
        continue;
      }
      sites.Append(base::DictValue()
                       .Set("id", base::NumberToString(child->id()))
                       .Set("title", child->GetTitle())
                       .Set("url", child->url().spec()));
    }
    return sites;
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
    base::ListValue distractions;
    for (const std::string& id : blocks.DistractionCategories()) {
      distractions.Append(id);
    }
    state.Set("distractions", std::move(distractions));
    state.Set("desktop", DesktopValue());
    // A model still loading reports no sites yet; the observer repaints.
    state.Set("desktopReady", bookmarks_ && bookmarks_->loaded());
    state.Set("widgets",
              profile_->GetPrefs()->GetList(kDesktopWidgetsPref).Clone());
    state.Set("result", std::move(result));
    CallJavascriptFunction("mastState", base::Value(std::move(state)));
  }

  const raw_ptr<Profile> profile_;
  const raw_ptr<bookmarks::BookmarkModel> bookmarks_;
  base::ScopedObservation<bookmarks::BookmarkModel,
                          bookmarks::BookmarkModelObserver>
      observation_{this};
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
  Profile* profile = Profile::FromWebUI(web_ui);
  // Site icons for the desktop: chrome://favicon2, local favicon database
  // only (the page never asks for Google's server fallback).
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
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
            // New tabs open the desktop; chrome://mast is the full blocklist.
            std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
                as_new_tab ? DesktopPageHtml() : StartPageHtml(false)));
          },
          as_new_tab));
  web_ui->AddMessageHandler(std::make_unique<MastHandler>(profile));
}

MastUI::~MastUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(MastUI)

}  // namespace mast
