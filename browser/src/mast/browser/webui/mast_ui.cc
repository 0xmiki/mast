// Copyright 2026 The Mast Authors

#include "mast/browser/webui/mast_ui.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/memory/weak_ptr.h"
#include "base/files/file_path.h"
#include "base/values.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/themes/theme_properties.h"
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
#include "chrome/browser/shell_integration.h"
#include "mast/browser/desktop/mast_desktop.h"
#include "mast/browser/import/mast_import.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_blocks.h"
#include "mast/browser/mast_feeds.h"
#include "mast/browser/mast_page_backdrop.h"
#include "mast/browser/mast_pages.h"
#include "mast/browser/mast_prefs.h"
#include "mast/browser/mast_theme.h"
#include "mast/browser/mast_wallpapers.h"
#include "mast/common/mast_features.h"
#include "mast/common/mast_tab_bar_state.h"
#include "mast/ui/color/mast_palette.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/url_formatter/url_fixer.h"
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

// Customize's built-in pictures (mast_wallpapers.h), served as preset-<id>.jpg.
base::ListValue PresetsValue() {
  base::ListValue list;
  for (const WallpaperPreset& p : kWallpaperPresets) {
    list.Append(base::DictValue()
                    .Set("id", p.id)
                    .Set("name", p.name)
                    .Set("credit", p.credit));
  }
  return list;
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
        // A feed's row ("Other gambling sites") lists no domains to show.
        const bool feed = s.domains.size() == 1 && IsFeedKey(s.domains[0]);
        services.Append(base::DictValue()
                            .Set("name", s.name)
                            .Set("domains", std::move(domains))
                            .Set("feed", feed));
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

// The welcome page's search engines (design: Onboarding, step 3), by their
// prepopulated ids (third_party/search_engines_data): Google, DuckDuckGo,
// Bing, Brave, Baidu, Yandex. The page lists them in this order.
constexpr int kWelcomeEngines[] = {1, 92, 3, 109, 21, 15};

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

// Customize's wallpaper lives in the profile folder: clearing browsing data
// keeps it (the page's own storage would not), and the native frame can use
// the same file later.
base::FilePath WallpaperPath(Profile* profile) {
  return profile->GetPath().AppendASCII(kWallpaperFile);
}

// The page scales the image down (2560 px wide at most) and sends a JPEG.
constexpr size_t kMaxWallpaperBytes = 8 * 1024 * 1024;

constexpr base::TaskTraits kFileTraits = {
    base::MayBlock(), base::TaskPriority::USER_VISIBLE,
    base::TaskShutdownBehavior::BLOCK_SHUTDOWN};

scoped_refptr<base::RefCountedMemory> ReadWallpaper(
    const base::FilePath& path) {
  std::string jpeg;
  if (!base::ReadFileToString(path, &jpeg)) {
    jpeg.clear();
  }
  return base::MakeRefCounted<base::RefCountedString>(std::move(jpeg));
}

std::string HexColor(SkColor c) {
  return base::StringPrintf("#%02x%02x%02x", SkColorGetR(c), SkColorGetG(c),
                            SkColorGetB(c));
}

// Where the wallpaper's top-left is above the page, in DIP: the image starts
// kFrameHeightAboveTabs above the window's content (the theme-image origin the
// tab row paints from), and the page starts below the one-row tab row.
int WallpaperTop() {
  return OneRowContentTop() + ThemeProperties::kFrameHeightAboveTabs;
}

// The palettes Customize offers, with the colours its swatches show.
base::ListValue PalettesValue() {
  base::ListValue palettes;
  for (const palette::Palette& p : palette::kPalettes) {
    palettes.Append(base::DictValue()
                        .Set("id", p.id)
                        .Set("name", p.name)
                        .Set("surface", HexColor(p.chrome))
                        .Set("field", HexColor(p.chrome2))
                        .Set("text", HexColor(p.text)));
  }
  return palettes;
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
    // The welcome page (resources/welcome.html, design C25).
    On("welcomeSources", &MastHandler::HandleWelcomeSources);
    On("welcomeImport", &MastHandler::HandleWelcomeImport);
    On("welcomeDefault", &MastHandler::HandleWelcomeDefault);
    On("welcomeEngine", &MastHandler::HandleWelcomeEngine);
    On("welcomeSetEngine", &MastHandler::HandleWelcomeSetEngine);
    On("block", &MastHandler::HandleBlock);
    On("undo", &MastHandler::HandleUndo);
    On("addSite", &MastHandler::HandleAddSite);
    On("removeSite", &MastHandler::HandleRemoveSite);
    On("createCategory", &MastHandler::HandleCreateCategory);
    On("renameCategory", &MastHandler::HandleRenameCategory);
    On("deleteCategory", &MastHandler::HandleDeleteCategory);
    On("addShortcut", &MastHandler::HandleAddShortcut);
    On("removeShortcut", &MastHandler::HandleRemoveShortcut);
    On("blockDistractions", &MastHandler::HandleBlockDistractions);
    On("setDistractions", &MastHandler::HandleSetDistractions);
    On("desktopAdd", &MastHandler::HandleDesktopAdd);
    On("desktopRemove", &MastHandler::HandleDesktopRemove);
    On("desktopMove", &MastHandler::HandleDesktopMove);
    On("setPalette", &MastHandler::HandleSetPalette);
    On("setWallpaper", &MastHandler::HandleSetWallpaper);
    On("clearWallpaper", &MastHandler::HandleClearWallpaper);
    On("setWallpaperTone", &MastHandler::HandleSetWallpaperTone);
    On("setWallpaperPreset", &MastHandler::HandleSetWallpaperPreset);
    On("setTabBar", &MastHandler::HandleSetTabBar);
    On("typeInAddressBar", &MastHandler::HandleTypeInAddressBar);
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

  // ["welcomeSources"]: the browsers to import from, and whether Mast is the
  // default browser. Replies through mastWelcome({sources, isDefault}).
  void HandleWelcomeSources(const base::ListValue& args) {
    AllowJavascript();
    FindImportSources(base::BindOnce(&MastHandler::OnSources,
                                     weak_factory_.GetWeakPtr()));
    CheckDefault();
  }

  void OnSources(std::vector<ImportSource> sources) {
    sources_ = std::move(sources);
    base::ListValue list;
    for (const ImportSource& s : sources_) {
      list.Append(base::DictValue()
                      .Set("id", s.id)
                      .Set("browser", s.browser)
                      .Set("profile", s.profile)
                      .Set("bookmarks", s.bookmarks)
                      .Set("extensions", s.extensions)
                      .Set("history", s.history)
                      .Set("firefox", s.firefox)
                      .Set("lastUsed", s.last_used.InMillisecondsFSinceUnixEpochIgnoringNull()));
    }
    CallJavascriptFunction("mastWelcome",
                           base::Value(base::DictValue().Set("sources", std::move(list))));
  }

  // ["welcomeImport", id, {bookmarks, history, extensions}]: one click.
  void HandleWelcomeImport(const base::ListValue& args) {
    AllowJavascript();
    const std::string id = StringArg(args);
    const base::DictValue* what =
        args.size() > 1 && args[1].is_dict() ? &args[1].GetDict() : nullptr;
    auto want = [&](const char* key) {
      return !what || what->FindBool(key).value_or(true);
    };
    const auto it = std::ranges::find(sources_, id, &ImportSource::id);
    if (it == sources_.end()) {
      return;
    }
    RunImport(profile_, *it, want("bookmarks"), want("history"),
              want("extensions"),
              base::BindOnce(&MastHandler::OnImported, weak_factory_.GetWeakPtr()));
  }

  void OnImported(ImportResult r) {
    CallJavascriptFunction(
        "mastWelcome",
        base::Value(base::DictValue().Set(
            "result", base::DictValue()
                          .Set("browser", r.browser)
                          .Set("bookmarks", r.bookmarks)
                          .Set("onNewTab", r.on_new_tab)
                          .Set("history", r.history)
                          .Set("extensions", r.extensions)
                          .Set("ok", r.ok))));
  }

  // ["welcomeDefault"]: make Mast the default browser (the OS may ask).
  void HandleWelcomeDefault(const base::ListValue& args) {
    AllowJavascript();
    base::MakeRefCounted<shell_integration::DefaultBrowserWorker>()
        ->StartSetAsDefault(base::BindOnce(&MastHandler::OnDefault,
                                           weak_factory_.GetWeakPtr()));
  }

  // ["welcomeEngine"]: the search engine in use, as its prepopulated id (0
  // for one the user made). Replies through mastWelcome({engine}).
  void HandleWelcomeEngine(const base::ListValue& args) {
    AllowJavascript();
    SendEngine();
  }

  // ["welcomeSetEngine", id]: one of the welcome page's engines
  // (kWelcomeEngines) becomes the default, added from Chromium's built-in list
  // if the profile doesn't have it yet (a profile starts with its region's).
  void HandleWelcomeSetEngine(const base::ListValue& args) {
    AllowJavascript();
    const int id = args.size() > 0 && args[0].is_int() ? args[0].GetInt() : 0;
    TemplateURLService* service =
        TemplateURLServiceFactory::GetForProfile(profile_);
    if (service && std::ranges::find(kWelcomeEngines, id) !=
                       std::end(kWelcomeEngines)) {
      service->SetUserSelectedDefaultSearchProviderByPrepopulateId(id);
    }
    SendEngine();
  }

  void SendEngine() {
    const TemplateURLService* service =
        TemplateURLServiceFactory::GetForProfile(profile_);
    const TemplateURL* engine =
        service ? service->GetDefaultSearchProvider() : nullptr;
    CallJavascriptFunction(
        "mastWelcome",
        base::Value(base::DictValue().Set(
            "engine", engine ? engine->prepopulate_id() : 0)));
  }

  void CheckDefault() {
    base::MakeRefCounted<shell_integration::DefaultBrowserWorker>()
        ->StartCheckIsDefault(base::BindOnce(&MastHandler::OnDefault,
                                             weak_factory_.GetWeakPtr()));
  }

  void OnDefault(shell_integration::DefaultWebClientState state) {
    CallJavascriptFunction(
        "mastWelcome",
        base::Value(base::DictValue().Set(
            "isDefault", state == shell_integration::IS_DEFAULT)));
  }

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

  // ["blockDistractions", durationId]: what the distraction set still has
  // open (blocks already running are left as they are).
  void HandleBlockDistractions(const base::ListValue& args) {
    Block(MastBlocks::Get().OpenDistractionDomains(), StringArg(args, 0));
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

  // ["setDistractions", [categoryIds], [skipped site keys], youtube mode]:
  // the whole set at once (the blocklist panel, design C28d). The mode is
  // "site", "music" or "shorts" (YouTubeModeId).
  void HandleSetDistractions(const base::ListValue& args) {
    auto strings = [&](size_t i) {
      std::vector<std::string> out;
      if (args.size() > i && args[i].is_list()) {
        for (const base::Value& v : args[i].GetList()) {
          if (v.is_string()) {
            out.push_back(v.GetString());
          }
        }
      }
      return out;
    };
    MastBlocks::Distractions d;
    d.categories = strings(0);
    d.skipped = strings(1);
    d.youtube = YouTubeModeFromId(StringArg(args, 2)).value_or(YouTubeMode::kSite);
    MastBlocks::Get().SetDistractions(d);
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

  // ["setPalette", id]: the whole browser switches, not only this page.
  void HandleSetPalette(const base::ListValue& args) {
    const std::string id = StringArg(args);
    if (std::ranges::none_of(palette::kPalettes, [&](const palette::Palette& p) {
          return p.id == id;
        })) {
      return;
    }
    profile_->GetPrefs()->SetString(kPalettePref, id);
    ApplyTheme(profile_->GetPrefs());
    SendState();
  }

  // ["setWallpaperTone", "dark" | "light"]: whether the image is dark or
  // light, so the tab row's text and icons (and the page's) stay readable.
  void HandleSetWallpaperTone(const base::ListValue& args) {
    SetTone(StringArg(args));
    ApplyTheme(profile_->GetPrefs());
    SendState();
  }

  // ["setTabBar", "picture" | "theme" | "site"]: what the tab row shows on
  // websites. Each window follows the pref (mast/browser/ui/mast_tab_bar.h).
  void HandleSetTabBar(const base::ListValue& args) {
    const std::string mode = StringArg(args);
    if (mode == "picture" || mode == "theme" || mode == "site") {
      profile_->GetPrefs()->SetString(kTabBarPref, mode);
    }
    SendState();
  }

  void SetTone(const std::string& tone) {
    if (tone == "dark" || tone == "light") {
      profile_->GetPrefs()->SetString(kWallpaperTonePref, tone);
    }
  }

  // ["setWallpaper", base64 JPEG, "dark" | "light" (the page's guess)]
  void HandleSetWallpaper(const base::ListValue& args) {
    SetTone(StringArg(args, 1));
    std::optional<std::vector<uint8_t>> jpeg =
        base::Base64Decode(StringArg(args));
    if (!jpeg || jpeg->empty() || jpeg->size() > kMaxWallpaperBytes) {
      return SendState(base::DictValue().Set("wallpaperFailed", true));
    }
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, kFileTraits,
        base::BindOnce(
            [](const base::FilePath& path, const std::vector<uint8_t>& data) {
              return base::WriteFile(path, data);
            },
            WallpaperPath(profile_), std::move(*jpeg)),
        base::BindOnce(&MastHandler::OnWallpaperWritten,
                       weak_factory_.GetWeakPtr()));
  }

  void OnWallpaperWritten(bool ok) {
    if (!ok) {
      return SendState(base::DictValue().Set("wallpaperFailed", true));
    }
    // A new version per upload, never reused (see kWallpaperPref).
    PrefService* prefs = profile_->GetPrefs();
    prefs->SetInteger(kWallpaperPref,
                      std::abs(prefs->GetInteger(kWallpaperPref)) + 1);
    prefs->SetString(kWallpaperPresetPref, std::string());  // the user's own
    if (!ApplyTheme(prefs)) {
      RepaintBrowser();  // same colours, new picture
    }
    SendState();
  }

  // ["typeInAddressBar", text]: typed or pasted on the new tab page with
  // nothing on it focused; the address bar takes it, and the focus.
  void HandleTypeInAddressBar(const base::ListValue& args) {
    NewTabTyped(web_ui()->GetWebContents(),
                base::UTF8ToUTF16(StringArg(args)));
  }

  // ["setWallpaperPreset", id]: one of Customize's built-in pictures.
  void HandleSetWallpaperPreset(const base::ListValue& args) {
    UseWallpaperPreset(profile_, StringArg(args),
                       base::BindOnce(
                           [](base::WeakPtr<MastHandler> self, bool ok) {
                             if (self) {
                               self->SendState();
                             }
                           },
                           weak_factory_.GetWeakPtr()));
  }

  void HandleClearWallpaper(const base::ListValue& args) {
    PrefService* prefs = profile_->GetPrefs();
    prefs->SetString(kWallpaperPresetPref, std::string());
    prefs->SetInteger(kWallpaperPref,
                      -std::abs(prefs->GetInteger(kWallpaperPref)));
    if (!ApplyTheme(prefs)) {
      RepaintBrowser();
    }
    base::ThreadPool::PostTask(
        FROM_HERE, kFileTraits,
        base::BindOnce(base::IgnoreResult(&base::DeleteFile),
                       WallpaperPath(profile_)));
    SendState();
  }

  void HandleUndo(const base::ListValue& args) {
    MastBlocks::Get().Undo();
    SendState(base::DictValue().Set("undone", true));
  }

  // ["addSite", categoryId, input]: a site of the user's own, in a built-in
  // category or one of theirs (design C33). Answers {site: "added" | "invalid"
  // | "listed", domain or where}.
  void HandleAddSite(const base::ListValue& args) {
    const MastBlocks::AddResult r =
        MastBlocks::Get().AddSite(StringArg(args, 0), StringArg(args, 1));
    SendState(r.error.empty()
                  ? base::DictValue().Set("site", "added").Set("domain", r.domain)
                  : base::DictValue().Set("site", r.error).Set("where", r.where));
  }

  // ["removeSite", categoryId, domain]: only a site the user added.
  void HandleRemoveSite(const base::ListValue& args) {
    MastBlocks::Get().RemoveSite(StringArg(args, 0), StringArg(args, 1));
    SendState();
  }

  // ["createCategory", name]: answers {created: id} (empty if the name was).
  void HandleCreateCategory(const base::ListValue& args) {
    SendState(base::DictValue().Set(
        "created", MastBlocks::Get().CreateCategory(StringArg(args))));
  }

  // ["renameCategory", id, name]: only the user's own.
  void HandleRenameCategory(const base::ListValue& args) {
    MastBlocks::Get().RenameCategory(StringArg(args, 0), StringArg(args, 1));
    SendState();
  }

  // ["deleteCategory", id]: only the user's own, while none of it is blocked.
  void HandleDeleteCategory(const base::ListValue& args) {
    SendState(base::DictValue().Set(
        "deleted", MastBlocks::Get().DeleteCategory(StringArg(args))));
  }

  // ["addShortcut", key, name, url, replaces]: Alt + key opens url in a new
  // tab (Shortcuts tab, MastShortcuts). The key is a free letter (IsShortcutKey,
  // not one another shortcut has); the url, a web address as typed. With
  // `replaces` (a key), it edits that shortcut in place.
  void HandleAddShortcut(const base::ListValue& args) {
    const std::string key = base::ToUpperASCII(StringArg(args, 0));
    std::string name(
        base::TrimWhitespaceASCII(StringArg(args, 1), base::TRIM_ALL));
    // Typed without a scheme ("google.com/search?q=..."): https, not the
    // fixer's http.
    std::string typed(
        base::TrimWhitespaceASCII(StringArg(args, 2), base::TRIM_ALL));
    if (!typed.empty() && typed.find("://") == std::string::npos) {
      typed = "https://" + typed;
    }
    const GURL url = url_formatter::FixupURL(typed, std::string());
    const std::string replaces = StringArg(args, 3);
    PrefService* prefs = profile_->GetPrefs();
    const base::ListValue& list = prefs->GetList(kShortcutsPref);
    bool taken = false;
    for (const base::Value& item : list) {
      const std::string* k = item.is_dict() ? item.GetDict().FindString("key")
                                            : nullptr;
      taken = taken || (k && *k == key && *k != replaces);
    }
    if (!IsShortcutKey(key) || taken) {
      return SendState(base::DictValue().Set("shortcut", "badKey"));
    }
    if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS() || url.host().empty()) {
      return SendState(base::DictValue().Set("shortcut", "badUrl"));
    }
    if (name.empty()) {
      name = url.host();
      if (name.starts_with("www.")) {
        name = name.substr(4);
      }
    }
    base::DictValue shortcut = base::DictValue()
                                   .Set("key", key)
                                   .Set("name", name.substr(0, 80))
                                   .Set("url", url.spec());
    // An edit keeps its place in the list.
    base::ListValue next;
    bool placed = false;
    for (const base::Value& item : list) {
      const std::string* k = item.is_dict() ? item.GetDict().FindString("key")
                                            : nullptr;
      if (!replaces.empty() && k && *k == replaces && !placed) {
        next.Append(shortcut.Clone());
        placed = true;
      } else {
        next.Append(item.Clone());
      }
    }
    if (!placed) {
      next.Append(std::move(shortcut));
    }
    prefs->SetList(kShortcutsPref, std::move(next));
    SendState(base::DictValue().Set("shortcut", "added").Set("key", key));
  }

  // ["removeShortcut", key].
  void HandleRemoveShortcut(const base::ListValue& args) {
    const std::string key = StringArg(args);
    PrefService* prefs = profile_->GetPrefs();
    base::ListValue next;
    for (const base::Value& item : prefs->GetList(kShortcutsPref)) {
      const std::string* k = item.is_dict() ? item.GetDict().FindString("key")
                                            : nullptr;
      if (!k || *k != key) {
        next.Append(item.Clone());
      }
    }
    prefs->SetList(kShortcutsPref, std::move(next));
    SendState();
  }

  void SendState(base::DictValue result = {}) {
    AllowJavascript();
    // Warm the blocked page's copy of the picture (it is read off the UI
    // thread, so a blocked page finds it ready).
    GetPageBackdrop(profile_);
    MastBlocks& blocks = MastBlocks::Get();
    base::DictValue active;
    for (const auto& [domain, block] : blocks.Active()) {
      active.Set(domain, BlockValue(block));
    }
    // Sites the user added to built-in categories, and their own categories
    // (design C33).
    base::DictValue additions;
    for (const auto& [id, sites] : blocks.Additions()) {
      base::ListValue list;
      for (const std::string& site : sites) {
        list.Append(site);
      }
      additions.Set(id, std::move(list));
    }
    base::ListValue own;
    for (const UserCategory& c : blocks.UserCategories()) {
      base::ListValue sites;
      for (const std::string& site : c.sites) {
        sites.Append(site);
      }
      own.Append(base::DictValue()
                     .Set("id", c.id)
                     .Set("name", c.name)
                     .Set("sites", std::move(sites)));
    }
    base::DictValue state;
    state.Set("categories", CategoriesValue().Clone());
    state.Set("additions", std::move(additions));
    state.Set("userCategories", std::move(own));
    state.Set("blocks", std::move(active));
    state.Set("lastDuration", blocks.last_duration());
    state.Set("canUndo", blocks.CanUndo());
    state.Set("stopped", blocks.StoppedToday());
    state.Set("now", Ms(base::Time::Now()));
    const MastBlocks::Distractions set = blocks.GetDistractions();
    base::ListValue set_categories;
    for (const std::string& id : set.categories) {
      set_categories.Append(id);
    }
    base::ListValue set_skipped;
    for (const std::string& key : set.skipped) {
      set_skipped.Append(key);
    }
    state.Set("distractions", base::DictValue()
                                  .Set("categories", std::move(set_categories))
                                  .Set("skipped", std::move(set_skipped))
                                  .Set("youtube", YouTubeModeId(set.youtube)));
    state.Set("desktop", DesktopValue());
    // A model still loading reports no sites yet; the observer repaints.
    state.Set("desktopReady", bookmarks_ && bookmarks_->loaded());
    state.Set("widgets",
              profile_->GetPrefs()->GetList(kDesktopWidgetsPref).Clone());
    state.Set("theme",
              base::DictValue()
                  .Set("palette", palette::Current().id)
                  .Set("css", PaletteCss())
                  .Set("wallpaper", std::max(0, profile_->GetPrefs()->GetInteger(
                                                    kWallpaperPref)))
                  .Set("wallpaperTop", WallpaperTop())
                  .Set("tone", profile_->GetPrefs()->GetString(
                                   kWallpaperTonePref))
                  .Set("tabBar",
                       profile_->GetPrefs()->GetString(kTabBarPref))
                  .Set("preset",
                       profile_->GetPrefs()->GetString(kWallpaperPresetPref))
                  .Set("presets", PresetsValue()));
    state.Set("palettes", PalettesValue());
    state.Set("shortcuts",
              profile_->GetPrefs()->GetList(kShortcutsPref).Clone());
    state.Set("result", std::move(result));
    CallJavascriptFunction("mastState", base::Value(std::move(state)));
  }

  const raw_ptr<Profile> profile_;
  const raw_ptr<bookmarks::BookmarkModel> bookmarks_;
  std::vector<ImportSource> sources_;  // the welcome page's last list
  base::ScopedObservation<bookmarks::BookmarkModel,
                          bookmarks::BookmarkModelObserver>
      observation_{this};
  base::WeakPtrFactory<MastHandler> weak_factory_{this};
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
        return path.empty() || path == "index.html" || path == "welcome" ||
               base::StartsWith(path, "wallpaper-") ||
               base::StartsWith(path, "preset-");
      }),
      base::BindRepeating(
          [](bool as_new_tab, PrefService* prefs,
             const base::FilePath& wallpaper, const std::string& path,
             content::WebUIDataSource::GotDataCallback callback) {
            // Customize's image, wallpaper-<version>.jpg (the version only
            // keeps a replaced image out of the cache).
            if (base::StartsWith(path, "wallpaper-")) {
              base::ThreadPool::PostTaskAndReplyWithResult(
                  FROM_HERE, kFileTraits,
                  base::BindOnce(&ReadWallpaper, wallpaper),
                  std::move(callback));
              return;
            }
            // A built-in picture, preset-<id>.jpg (Customize's thumbnails).
            if (base::StartsWith(path, "preset-")) {
              std::string id = path.substr(7);
              if (base::EndsWith(id, ".jpg")) {
                id.resize(id.size() - 4);
              }
              const WallpaperPreset* preset = FindWallpaperPreset(id);
              if (!preset) {
                std::move(callback).Run(nullptr);
                return;
              }
              base::ThreadPool::PostTaskAndReplyWithResult(
                  FROM_HERE, kFileTraits,
                  base::BindOnce(&ReadWallpaper, WallpaperPresetPath(*preset)),
                  std::move(callback));
              return;
            }
            // The welcome page, shown once (mast_prefs.h, kWelcomedPref), on
            // the picture.
            if (path == "welcome") {
              const WallpaperPreset* preset =
                  FindWallpaperPreset(prefs->GetString(kWallpaperPresetPref));
              std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
                  WelcomePageHtml(std::max(0, prefs->GetInteger(kWallpaperPref)),
                                  preset ? preset->credit : "")));
              return;
            }
            // New tabs open the desktop; chrome://mast is the full blocklist.
            std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
                as_new_tab ? DesktopPageHtml(std::max(
                                 0, prefs->GetInteger(kWallpaperPref)),
                                 WallpaperTop(),
                                 prefs->GetString(kWallpaperTonePref) == "light")
                           : StartPageHtml(false)));
          },
          as_new_tab, base::Unretained(profile->GetPrefs()),
          WallpaperPath(profile)));
  web_ui->AddMessageHandler(std::make_unique<MastHandler>(profile));
}

MastUI::~MastUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(MastUI)

}  // namespace mast
