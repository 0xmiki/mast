// Copyright 2026 The Mast Authors

#include "mast/browser/import/mast_import.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/barrier_closure.h"
#include "base/base_paths.h"
#include "base/environment.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/browser/importer/profile_writer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_row.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/user_data_importer/common/imported_bookmark_entry.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "extensions/browser/extension_registry.h"
#include "mast/browser/desktop/mast_desktop.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "ui/gfx/native_ui_types.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_WIN) && !BUILDFLAG(IS_MAC)
#include "base/nix/xdg_util.h"
#endif

namespace mast {

namespace {

constexpr base::TaskTraits kFileTraits = {
    base::MayBlock(), base::TaskPriority::USER_VISIBLE,
    base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN};

// The most recent pages to bring over from a history: enough for the address
// bar to know your sites, without copying years of it.
constexpr int kMaxHistoryRows = 20000;

struct Browser {
  const char* key;
  const char* name;
  // Where it keeps its profiles: under the platform's base folder, or, when
  // it starts with "~/", the home folder (Linux: Snap and Flatpak installs).
  const char* folders[4];
};

// Where Chromium-family browsers keep their profiles.
base::FilePath BaseFolder() {
  base::FilePath base;
#if BUILDFLAG(IS_WIN)
  base::PathService::Get(base::DIR_LOCAL_APP_DATA, &base);
#elif BUILDFLAG(IS_MAC)
  base::PathService::Get(base::DIR_APP_DATA, &base);
#else
  // $XDG_CONFIG_HOME, else ~/.config.
  std::unique_ptr<base::Environment> env = base::Environment::Create();
  base = base::nix::GetXDGDirectory(env.get(), base::nix::kXdgConfigHomeEnvVar,
                                    base::nix::kDotConfigDir);
#endif
  return base;
}

base::FilePath BrowserFolder(const base::FilePath& base, std::string_view folder) {
  if (folder.starts_with("~/")) {
    base::FilePath home;
    base::PathService::Get(base::DIR_HOME, &home);
    return home.AppendASCII(folder.substr(2));
  }
  return base.AppendASCII(folder);
}

constexpr Browser kBrowsers[] = {
#if BUILDFLAG(IS_WIN)
    {"chrome", "Google Chrome", {"Google/Chrome/User Data"}},
    {"chrome-beta", "Chrome Beta", {"Google/Chrome Beta/User Data"}},
    {"brave", "Brave", {"BraveSoftware/Brave-Browser/User Data"}},
    {"edge", "Microsoft Edge", {"Microsoft/Edge/User Data"}},
    {"vivaldi", "Vivaldi", {"Vivaldi/User Data"}},
    {"chromium", "Chromium", {"Chromium/User Data"}},
#elif BUILDFLAG(IS_MAC)
    {"chrome", "Google Chrome", {"Google/Chrome"}},
    {"chrome-beta", "Chrome Beta", {"Google/Chrome Beta"}},
    {"brave", "Brave", {"BraveSoftware/Brave-Browser"}},
    {"edge", "Microsoft Edge", {"Microsoft Edge"}},
    {"arc", "Arc", {"Arc/User Data"}},
    {"vivaldi", "Vivaldi", {"Vivaldi"}},
    {"opera", "Opera", {"com.operasoftware.Opera"}},
    {"chromium", "Chromium", {"Chromium"}},
#else
    // The package's folder, then Flatpak's (~/.var/app/<app id>/config) and
    // Snap's. Ubuntu's Chromium is only a Snap.
    {"chrome", "Google Chrome",
     {"google-chrome", "~/.var/app/com.google.Chrome/config/google-chrome"}},
    {"chrome-beta", "Chrome Beta", {"google-chrome-beta"}},
    {"chrome-dev", "Chrome Dev",
     {"google-chrome-unstable",
      "~/.var/app/com.google.ChromeDev/config/google-chrome-unstable"}},
    {"brave", "Brave",
     {"BraveSoftware/Brave-Browser",
      "~/.var/app/com.brave.Browser/config/BraveSoftware/Brave-Browser",
      "~/snap/brave/current/.config/BraveSoftware/Brave-Browser"}},
    {"edge", "Microsoft Edge",
     {"microsoft-edge", "~/.var/app/com.microsoft.Edge/config/microsoft-edge"}},
    {"vivaldi", "Vivaldi",
     {"vivaldi", "~/.var/app/com.vivaldi.Vivaldi/config/vivaldi"}},
    {"opera", "Opera",
     {"opera", "~/.var/app/com.opera.Opera/config/opera",
      "~/snap/opera/current/.config/opera"}},
    {"chromium", "Chromium",
     {"chromium", "~/snap/chromium/common/chromium",
      "~/.var/app/org.chromium.Chromium/config/chromium",
      "~/.var/app/io.github.ungoogled_software.ungoogled_chromium/config/chromium"}},
#endif
};

std::optional<base::DictValue> ReadJson(const base::FilePath& path) {
  std::string text;
  if (!base::ReadFileToString(path, &text)) {
    return std::nullopt;
  }
  std::optional<base::Value> value =
      base::JSONReader::Read(text, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!value || !value->is_dict()) {
    return std::nullopt;
  }
  return std::move(*value).TakeDict();
}

// A bookmark tree read from a profile's "Bookmarks" file.
struct Node {
  std::u16string title;
  GURL url;  // empty for a folder
  std::vector<Node> children;
};

void ReadNodes(const base::ListValue* list, std::vector<Node>& out) {
  if (!list) {
    return;
  }
  for (const base::Value& v : *list) {
    if (!v.is_dict()) {
      continue;
    }
    const base::DictValue& d = v.GetDict();
    const std::string* type = d.FindString("type");
    const std::string* name = d.FindString("name");
    Node node;
    node.title = name ? base::UTF8ToUTF16(*name) : std::u16string();
    if (type && *type == "url") {
      const std::string* url = d.FindString("url");
      node.url = url ? GURL(*url) : GURL();
      if (!node.url.is_valid() || node.url.SchemeIs("javascript")) {
        continue;
      }
    } else {
      ReadNodes(d.FindList("children"), node.children);
    }
    out.push_back(std::move(node));
  }
}

int CountUrls(const std::vector<Node>& nodes) {
  int n = 0;
  for (const Node& node : nodes) {
    n += node.url.is_valid() ? 1 : CountUrls(node.children);
  }
  return n;
}

struct BookmarkTree {
  std::vector<Node> bar;    // the bookmarks bar's children
  std::vector<Node> other;  // "Other" and "Mobile" bookmarks
};

BookmarkTree ReadBookmarks(const base::FilePath& profile) {
  BookmarkTree tree;
  std::optional<base::DictValue> file = ReadJson(profile.AppendASCII("Bookmarks"));
  const base::DictValue* roots = file ? file->FindDict("roots") : nullptr;
  if (!roots) {
    return tree;
  }
  auto children = [&](const char* root) -> const base::ListValue* {
    const base::DictValue* d = roots->FindDict(root);
    return d ? d->FindList("children") : nullptr;
  };
  ReadNodes(children("bookmark_bar"), tree.bar);
  ReadNodes(children("other"), tree.other);
  ReadNodes(children("synced"), tree.other);
  return tree;
}

// The source's own Web Store extensions (not built-in or preinstalled ones).
std::vector<std::string> ReadExtensions(const base::FilePath& profile) {
  std::set<std::string> ids;
  for (const char* file : {"Secure Preferences", "Preferences"}) {
    std::optional<base::DictValue> prefs = ReadJson(profile.AppendASCII(file));
    const base::DictValue* settings =
        prefs ? prefs->FindDictByDottedPath("extensions.settings") : nullptr;
    if (!settings) {
      continue;
    }
    for (const auto [id, value] : *settings) {
      const base::DictValue* e = value.GetIfDict();
      if (!e || id.size() != 32) {
        continue;
      }
      // Location 1 is "internal": installed by the user, from the store.
      if (e->FindInt("location").value_or(0) == 1 &&
          e->FindBool("from_webstore").value_or(false) &&
          !e->FindBool("was_installed_by_default").value_or(false) &&
          !e->FindBool("was_installed_by_oem").value_or(false)) {
        ids.insert(id);
      }
    }
  }
  return std::vector<std::string>(ids.begin(), ids.end());
}

// The profiles in one browser folder that have bookmarks, history or
// extensions. `suffix` keeps ids unique when a browser is installed twice (a
// package and a Flatpak, say): "chromium/Default", then "chromium/Default@1".
void ScanBrowserFolder(const Browser& browser,
                       const base::FilePath& root,
                       const std::string& suffix,
                       std::vector<ImportSource>& sources) {
  // Profiles and their names from "Local State"; a browser without one
  // (Opera) keeps a single profile in the folder itself.
  std::vector<std::pair<base::FilePath, ImportSource>> found;
  std::optional<base::DictValue> local = ReadJson(root.AppendASCII("Local State"));
  const base::DictValue* cache =
      local ? local->FindDictByDottedPath("profile.info_cache") : nullptr;
  if (cache) {
    for (const auto [dir, info] : *cache) {
      ImportSource s;
      s.path = root.AppendASCII(dir);
      if (const base::DictValue* d = info.GetIfDict()) {
        if (const std::string* name = d->FindString("name")) {
          s.profile = *name;
        }
        s.last_used = base::Time::FromSecondsSinceUnixEpoch(
            d->FindDouble("active_time").value_or(0));
      }
      s.id = std::string(browser.key) + "/" + dir + suffix;
      found.emplace_back(s.path, std::move(s));
    }
  } else {
    ImportSource s;
    s.path = base::PathExists(root.AppendASCII("Bookmarks"))
                 ? root
                 : root.AppendASCII("Default");
    s.id = std::string(browser.key) + "/Default" + suffix;
    found.emplace_back(s.path, std::move(s));
  }
  for (auto& [path, s] : found) {
    s.browser = browser.name;
    const BookmarkTree tree = ReadBookmarks(path);
    s.bookmarks = CountUrls(tree.bar) + CountUrls(tree.other);
    s.history = base::PathExists(path.AppendASCII("History"));
    s.extensions = static_cast<int>(ReadExtensions(path).size());
    if (s.last_used.is_null()) {
      base::File::Info info;
      if (base::GetFileInfo(path.AppendASCII("Preferences"), &info)) {
        s.last_used = info.last_modified;
      }
    }
    if (s.bookmarks > 0 || s.history || s.extensions > 0) {
      sources.push_back(std::move(s));
    }
  }
}

// Every Chromium-family profile with bookmarks, history or extensions.
std::vector<ImportSource> ScanChromiumFamily() {
  std::vector<ImportSource> sources;
  const base::FilePath base = BaseFolder();
  for (const Browser& browser : kBrowsers) {
    for (size_t place = 0; place < std::size(browser.folders); ++place) {
      if (!browser.folders[place]) {
        break;
      }
      const base::FilePath root = BrowserFolder(base, browser.folders[place]);
      if (base::DirectoryExists(root)) {
        ScanBrowserFolder(browser, root,
                          place ? "@" + base::NumberToString(place) : std::string(),
                          sources);
      }
    }
  }
  return sources;
}

// The most recent pages of a profile's history. The file is copied first: the
// other browser may be running and holding it.
history::URLRows ReadHistory(const base::FilePath& profile) {
  history::URLRows rows;
  base::ScopedTempDir temp;
  if (!temp.CreateUniqueTempDir()) {
    return rows;
  }
  const base::FilePath copy = temp.GetPath().AppendASCII("History");
  if (!base::CopyFile(profile.AppendASCII("History"), copy)) {
    return rows;
  }
  sql::Database db(sql::Database::Tag("History"));
  if (!db.Open(copy)) {
    return rows;
  }
  sql::Statement s(db.GetUniqueStatement(
      "SELECT url, title, visit_count, typed_count, last_visit_time FROM urls "
      "WHERE hidden = 0 ORDER BY last_visit_time DESC LIMIT ?"));
  s.BindInt(0, kMaxHistoryRows);
  while (s.Step()) {
    GURL url(s.ColumnString(0));
    if (!url.SchemeIsHTTPOrHTTPS()) {
      continue;
    }
    history::URLRow row(url);
    row.set_title(s.ColumnString16(1));
    row.set_visit_count(std::max(1, s.ColumnInt(2)));
    row.set_typed_count(s.ColumnInt(3));
    row.set_last_visit(s.ColumnTime(4));
    rows.push_back(std::move(row));
  }
  return rows;
}

struct SourceData {
  BookmarkTree bookmarks;
  history::URLRows history;
  std::vector<std::string> extensions;
};

// Adds `nodes` under `parent`, folders and all. Returns how many sites.
int AddNodes(bookmarks::BookmarkModel* model,
             const bookmarks::BookmarkNode* parent,
             const std::vector<Node>& nodes) {
  int added = 0;
  for (const Node& node : nodes) {
    if (node.url.is_valid()) {
      model->AddURL(parent, parent->children().size(), node.title, node.url);
      ++added;
    } else if (!node.children.empty()) {
      const bookmarks::BookmarkNode* folder =
          model->AddFolder(parent, parent->children().size(), node.title);
      added += AddNodes(model, folder, node.children);
    }
  }
  return added;
}

// The bookmarks bar's sites go on the new tab page (skipping ones already
// there); its folders and every other bookmark go into "From <browser>".
void AddBookmarks(bookmarks::BookmarkModel* model,
                  const std::string& browser,
                  const BookmarkTree& tree,
                  ImportResult& result) {
  const bookmarks::BookmarkNode* desktop = DesktopFolder(model);
  std::vector<Node> rest;
  if (desktop) {
    std::set<GURL> there;
    for (const auto& child : desktop->children()) {
      there.insert(child->url());
    }
    for (const Node& node : tree.bar) {
      if (!node.url.is_valid()) {
        rest.push_back(node);
      } else if (there.insert(node.url).second) {
        model->AddURL(desktop, desktop->children().size(), node.title, node.url);
        ++result.on_new_tab;
      }
    }
  } else {
    rest = tree.bar;
  }
  rest.insert(rest.end(), tree.other.begin(), tree.other.end());
  int added = result.on_new_tab;
  if (!rest.empty()) {
    const bookmarks::BookmarkNode* folder =
        model->AddFolder(model->other_node(), model->other_node()->children().size(),
                         u"From " + base::UTF8ToUTF16(browser));
    added += AddNodes(model, folder, rest);
  }
  result.bookmarks = added;
}

// Installs from the Web Store without a prompt or a bubble afterwards: the user
// already chose them, in one click (Brave does the same).
class SilentInstall : public extensions::WebstoreInstallWithPrompt {
 public:
  using WebstoreInstallWithPrompt::WebstoreInstallWithPrompt;

 private:
  ~SilentInstall() override = default;
  std::unique_ptr<extensions::InstallPromptData> CreateInstallPrompt()
      const override {
    return nullptr;
  }
  bool ShouldShowPostInstallUI() const override { return false; }
};

// Holds an import that waits on several asynchronous parts.
struct Job : base::RefCounted<Job> {
  ImportResult result;
  base::OnceCallback<void(ImportResult)> done;
  void Finish() { std::move(done).Run(result); }

 private:
  friend class base::RefCounted<Job>;
  ~Job() = default;
};

void InstallExtensions(Profile* profile,
                       const std::vector<std::string>& ids,
                       scoped_refptr<Job> job,
                       base::OnceClosure done) {
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile);
  std::vector<std::string> missing;
  for (const std::string& id : ids) {
    if (!registry || !registry->GetInstalledExtension(id)) {
      missing.push_back(id);
    }
  }
  if (missing.empty()) {
    std::move(done).Run();
    return;
  }
  base::RepeatingClosure each =
      base::BarrierClosure(missing.size(), std::move(done));
  for (const std::string& id : missing) {
    auto installer = base::MakeRefCounted<SilentInstall>(
        id, profile, gfx::NativeWindow(),
        base::BindOnce(
            [](scoped_refptr<Job> job, base::RepeatingClosure each, bool success,
               const std::string&, extensions::webstore_install::Result) {
              if (success) {
                ++job->result.extensions;
              }
              each.Run();
            },
            job, each));
    installer->BeginInstall();
  }
}

void ApplyChromiumFamily(base::WeakPtr<Profile> weak_profile,
                         std::string browser,
                         bool bookmarks,
                         bool history,
                         bool extensions,
                         scoped_refptr<Job> job,
                         SourceData data) {
  Profile* profile = weak_profile.get();
  if (!profile) {
    job->result.ok = false;
    job->Finish();
    return;
  }
  if (bookmarks) {
    bookmarks::BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile);
    if (model && model->loaded()) {
      AddBookmarks(model, browser, data.bookmarks, job->result);
    } else {
      job->result.ok = false;
    }
  }
  if (history && !data.history.empty()) {
    if (history::HistoryService* service = HistoryServiceFactory::GetForProfile(
            profile, ServiceAccessType::EXPLICIT_ACCESS)) {
      job->result.history = static_cast<int>(data.history.size());
      service->AddPagesWithDetails(data.history,
                                   history::SOURCE_OS_MIGRATION_IMPORTED);
    }
  }
  if (extensions && !data.extensions.empty()) {
    InstallExtensions(profile, data.extensions, job,
                      base::BindOnce(&Job::Finish, job));
    return;
  }
  job->Finish();
}

// Where Chromium's importer writes Firefox's bookmarks and history. Its own
// writer puts bookmarks in the bookmarks bar, which Mast doesn't show; this one
// files them the way Chromium-family imports are (AddBookmarks above): the
// toolbar's sites on the new tab page, everything else in "From <browser>". It
// also counts what came over.
class MastProfileWriter : public ProfileWriter {
 public:
  MastProfileWriter(Profile* profile, scoped_refptr<Job> job)
      : ProfileWriter(profile), target_(profile), job_(std::move(job)) {}

  void AddBookmarks(
      const std::vector<user_data_importer::ImportedBookmarkEntry>& bookmarks,
      const std::u16string& top_level_folder_name) override {
    bookmarks::BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(target_);
    if (!model || !model->loaded() || bookmarks.empty()) {
      return;
    }
    const bookmarks::BookmarkNode* desktop = DesktopFolder(model);
    std::set<GURL> there;
    if (desktop) {
      for (const auto& child : desktop->children()) {
        there.insert(child->url());
      }
    }
    const bookmarks::BookmarkNode* from = nullptr;  // made on first use
    for (const user_data_importer::ImportedBookmarkEntry& e : bookmarks) {
      if (!e.is_folder && (!e.url.is_valid() || e.url.SchemeIs("javascript"))) {
        continue;
      }
      // A toolbar entry's path starts with the toolbar's own folder.
      std::vector<std::u16string> path = e.path;
      if (e.in_toolbar && !path.empty()) {
        path.erase(path.begin());
      }
      if (e.in_toolbar && path.empty() && !e.is_folder && desktop) {
        if (there.insert(e.url).second) {
          model->AddURL(desktop, desktop->children().size(), e.title, e.url);
          ++job_->result.on_new_tab;
          ++job_->result.bookmarks;
        }
        continue;
      }
      if (!from) {
        const bookmarks::BookmarkNode* other = model->other_node();
        from = model->AddFolder(other, other->children().size(),
                                u"From " + base::UTF8ToUTF16(job_->result.browser));
      }
      const bookmarks::BookmarkNode* parent = from;
      if (e.is_folder) {
        path.push_back(e.title);
      }
      for (const std::u16string& name : path) {
        const bookmarks::BookmarkNode* found = nullptr;
        for (const auto& child : parent->children()) {
          if (child->is_folder() && child->GetTitle() == name) {
            found = child.get();
            break;
          }
        }
        parent = found ? found
                       : model->AddFolder(parent, parent->children().size(), name);
      }
      if (!e.is_folder) {
        model->AddURL(parent, parent->children().size(), e.title, e.url);
        ++job_->result.bookmarks;
      }
    }
  }

  void AddHistoryPage(const history::URLRows& page,
                      history::VisitSource visit_source) override {
    job_->result.history += static_cast<int>(page.size());
    ProfileWriter::AddHistoryPage(page, visit_source);
  }

 private:
  ~MastProfileWriter() override = default;

  const raw_ptr<Profile> target_;
  scoped_refptr<Job> job_;
};

// Firefox, through Chromium's importer (a separate process, as in Settings).
class FirefoxImport : public importer::ImporterProgressObserver {
 public:
  static void Start(Profile* profile,
                    const user_data_importer::SourceProfile& source,
                    uint16_t items,
                    scoped_refptr<Job> job) {
    // Owns itself until the import ends.
    auto* self = new FirefoxImport(job);
    self->host_ = new ExternalProcessImporterHost();
    self->host_->set_observer(self);
    self->host_->set_headless();
    self->host_->StartImportSettings(source, profile, items,
                                     new MastProfileWriter(profile, job));
  }

  void ImportStarted() override { started_ = true; }
  void ImportItemStarted(user_data_importer::ImportItem item) override {}
  void ImportItemEnded(user_data_importer::ImportItem item) override {}
  void ImportEnded() override {
    host_->set_observer(nullptr);
    // Without a window to ask in, the importer gives up before it starts when
    // Firefox is running (it holds its profile's lock): say so, instead of
    // reporting an import that never happened.
    if (!started_) {
      job_->result.ok = false;
      job_->result.close_first = true;
    }
    job_->Finish();
    delete this;
  }

 private:
  explicit FirefoxImport(scoped_refptr<Job> job) : job_(std::move(job)) {}
  ~FirefoxImport() override = default;

  scoped_refptr<Job> job_;
  raw_ptr<ExternalProcessImporterHost> host_ = nullptr;
  bool started_ = false;
};

// Chromium's list of Firefox profiles, kept alive while it's detected and
// while an import from it runs.
ImporterList& Firefoxes() {
  static base::NoDestructor<ImporterList> list;
  return *list;
}

}  // namespace

void FindImportSources(
    base::OnceCallback<void(std::vector<ImportSource>)> callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, kFileTraits, base::BindOnce(&ScanChromiumFamily),
      base::BindOnce(
          [](base::OnceCallback<void(std::vector<ImportSource>)> callback,
             std::vector<ImportSource> sources) {
            // Then Firefox, from Chromium's own detection.
            Firefoxes().DetectSourceProfiles(
                g_browser_process->GetApplicationLocale(),
                /*include_interactive_profiles=*/false,
                base::BindOnce(
                    [](base::OnceCallback<void(std::vector<ImportSource>)> callback,
                       std::vector<ImportSource> sources) {
                      ImporterList& list = Firefoxes();
                      for (size_t i = 0; i < list.count(); ++i) {
                        const user_data_importer::SourceProfile& p =
                            list.GetSourceProfileAt(i);
                        if (p.importer_type != user_data_importer::TYPE_FIREFOX) {
                          continue;
                        }
                        ImportSource s;
                        s.id = "firefox/" + base::NumberToString(i);
                        s.browser = base::UTF16ToUTF8(p.importer_name);
                        s.profile = base::UTF16ToUTF8(p.profile);
                        s.path = p.source_path;
                        s.firefox = true;
                        s.firefox_index = static_cast<int>(i);
                        s.bookmarks = -1;
                        s.history = true;
                        sources.push_back(std::move(s));
                      }
                      std::stable_sort(sources.begin(), sources.end(),
                                       [](const ImportSource& a,
                                          const ImportSource& b) {
                                         return a.last_used > b.last_used;
                                       });
                      std::move(callback).Run(std::move(sources));
                    },
                    std::move(callback), std::move(sources)));
          },
          std::move(callback)));
}

void RunImport(Profile* profile,
               const ImportSource& source,
               bool bookmarks,
               bool history,
               bool extensions,
               base::OnceCallback<void(ImportResult)> callback) {
  auto job = base::MakeRefCounted<Job>();
  job->result.browser = source.browser;
  job->done = std::move(callback);
  if (source.firefox) {
    ImporterList& list = Firefoxes();
    if (source.firefox_index < 0 ||
        static_cast<size_t>(source.firefox_index) >= list.count()) {
      job->result.ok = false;
      job->Finish();
      return;
    }
    uint16_t items = 0;
    if (bookmarks) {
      items |= user_data_importer::FAVORITES;
    }
    if (history) {
      items |= user_data_importer::HISTORY;
    }
    FirefoxImport::Start(profile, list.GetSourceProfileAt(source.firefox_index),
                         items, job);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, kFileTraits,
      base::BindOnce(
          [](base::FilePath path, bool bookmarks, bool history, bool extensions) {
            SourceData data;
            if (bookmarks) {
              data.bookmarks = ReadBookmarks(path);
            }
            if (history) {
              data.history = ReadHistory(path);
            }
            if (extensions) {
              data.extensions = ReadExtensions(path);
            }
            return data;
          },
          source.path, bookmarks, history, extensions),
      base::BindOnce(&ApplyChromiumFamily, profile->GetWeakPtr(), source.browser, bookmarks,
                     history, extensions, job));
}

}  // namespace mast
