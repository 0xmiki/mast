// Copyright 2026 The Mast Authors

#include "mast/browser/mast_blocks.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/web_contents.h"
#include "mast/browser/mast_blocklist.h"
#include "mast/browser/mast_feeds.h"
#include "url/gurl.h"

namespace mast {

namespace {

// {domain: {since: ms, until: ms}}; no "until" means forever.
constexpr char kBlocksPref[] = "mast.blocks";
// Before design C33, the user's own sites; now the category "yours"
// (MigrateCustomSites).
constexpr char kCustomSitesPref[] = "mast.custom_sites";
constexpr char kLastDurationPref[] = "mast.last_duration";
constexpr char kBlockedDayPref[] = "mast.stats.blocked_day";
constexpr char kBlockedCountPref[] = "mast.stats.blocked_count";
// The distraction set (MastBlocks::Distractions). Every category by default
// (design C28d); the user unticks what they need.
// (Adult and gambling sites aren't categories: they're always blocked; see
// mast_feeds.h.)
constexpr char kDistractionsPref[] = "mast.distractions";
constexpr char kDistractionsSkippedPref[] = "mast.distractions_skipped";
// Before C32, a bool for Music only; kDistractionsYouTubePref replaces it.
constexpr char kDistractionsYouTubeMusicPref[] =
    "mast.distractions_youtube_music";
// YouTubeModeId(): "site", "music" or "shorts"; empty until first set.
constexpr char kDistractionsYouTubePref[] = "mast.distractions_youtube";
// The user's own categories (design C33): [{id, name, sites: [domains]}].
constexpr char kUserCategoriesPref[] = "mast.user_categories";
// Sites the user added to built-in categories: {category id: [domains]}.
constexpr char kAdditionsPref[] = "mast.category_additions";
// A category's name, in bytes.
constexpr size_t kMaxCategoryName = 60;

bool IsYouTubeService(const Service& s) {
  return std::ranges::find(s.domains, "youtube.com") != s.domains.end();
}

const Category* BuiltIn(std::string_view id) {
  for (const Category& c : Categories()) {
    if (c.id == id) {
      return &c;
    }
  }
  return nullptr;
}

std::vector<std::string> StringList(const base::ListValue& list) {
  std::vector<std::string> out;
  for (const base::Value& v : list) {
    if (v.is_string()) {
      out.push_back(v.GetString());
    }
  }
  return out;
}

// Entries that aren't domains: feeds ("feed:…") and YouTube's Music only and
// Shorts.
bool IsSpecialEntry(std::string_view entry) {
  return IsFeedKey(entry) || entry == kYouTubeMusicKey ||
         entry == kYouTubeShortsKey;
}

bool IsYouTubeEntry(std::string_view entry) {
  return entry == kYouTubeMusicKey || entry == kYouTubeShortsKey;
}

std::vector<UserCategory> ReadUserCategories(const PrefService* p) {
  std::vector<UserCategory> out;
  for (const base::Value& v : p->GetList(kUserCategoriesPref)) {
    if (!v.is_dict()) {
      continue;
    }
    const std::string* id = v.GetDict().FindString("id");
    const std::string* name = v.GetDict().FindString("name");
    const base::ListValue* sites = v.GetDict().FindList("sites");
    if (id && name) {
      out.push_back({*id, *name,
                     sites ? StringList(*sites) : std::vector<std::string>()});
    }
  }
  return out;
}

void WriteUserCategories(PrefService* p,
                         const std::vector<UserCategory>& categories) {
  base::ListValue list;
  for (const UserCategory& c : categories) {
    base::ListValue sites;
    for (const std::string& s : c.sites) {
      sites.Append(s);
    }
    list.Append(base::DictValue()
                    .Set("id", c.id)
                    .Set("name", c.name)
                    .Set("sites", std::move(sites)));
  }
  p->SetList(kUserCategoriesPref, std::move(list));
}

std::map<std::string, std::vector<std::string>> ReadAdditions(
    const PrefService* p) {
  std::map<std::string, std::vector<std::string>> out;
  for (const auto [id, v] : p->GetDict(kAdditionsPref)) {
    if (v.is_list() && BuiltIn(id)) {
      out[id] = StringList(v.GetList());
    }
  }
  return out;
}

void WriteAdditions(
    PrefService* p,
    const std::map<std::string, std::vector<std::string>>& additions) {
  base::DictValue dict;
  for (const auto& [id, sites] : additions) {
    if (sites.empty()) {
      continue;
    }
    base::ListValue list;
    for (const std::string& s : sites) {
      list.Append(s);
    }
    dict.Set(id, std::move(list));
  }
  p->SetDict(kAdditionsPref, std::move(dict));
}

std::string CleanName(std::string_view name) {
  const std::string trimmed(base::TrimWhitespaceASCII(name, base::TRIM_ALL));
  std::string out;
  base::TruncateUTF8ToByteSize(trimmed, kMaxCategoryName, &out);
  return out;
}

bool InSet(const base::ListValue& list, std::string_view id) {
  for (const base::Value& v : list) {
    if (v.is_string() && v.GetString() == id) {
      return true;
    }
  }
  return false;
}

// The old "Your sites" list (mast.custom_sites, before design C33) becomes the
// user's own category "Your sites", in the set as those sites always were.
void MigrateCustomSites(PrefService* p) {
  const std::vector<std::string> old = StringList(p->GetList(kCustomSitesPref));
  if (old.empty()) {
    return;
  }
  std::vector<UserCategory> categories = ReadUserCategories(p);
  auto it = std::ranges::find(categories, kYourSitesId, &UserCategory::id);
  if (it == categories.end()) {
    categories.push_back({kYourSitesId, "Your sites", {}});
    it = std::prev(categories.end());
  }
  for (const std::string& d : old) {
    if (std::ranges::find(it->sites, d) == it->sites.end()) {
      it->sites.push_back(d);
    }
  }
  WriteUserCategories(p, categories);
  if (!InSet(p->GetList(kDistractionsPref), kYourSitesId)) {
    ScopedListPrefUpdate(p, kDistractionsPref)->Append(kYourSitesId);
  }
  p->ClearPref(kCustomSitesPref);
}

// A list entry: a domain, or a feed ("feed:gambling") that matches every
// domain the feed lists.
bool EntryMatches(std::string_view host, std::string_view entry) {
  if (IsFeedKey(entry)) {
    entry.remove_prefix(std::string_view(kFeedKeyPrefix).size());
    return MastFeeds::Get().Contains(entry, host);
  }
  return HostMatches(host, entry);
}

double Ms(base::Time t) {
  return static_cast<double>(t.InMillisecondsSinceUnixEpoch());
}

base::Time FromMs(double ms) {
  return base::Time::FromMillisecondsSinceUnixEpoch(ms);
}

std::string DayKey(base::Time t) {
  base::Time::Exploded e;
  t.LocalExplode(&e);
  return base::StringPrintf("%04d-%02d-%02d", e.year, e.month, e.day_of_month);
}

std::optional<Block> ParseBlock(const base::Value& v, base::Time now) {
  if (!v.is_dict()) {
    return std::nullopt;
  }
  Block block{FromMs(v.GetDict().FindDouble("since").value_or(0)),
              std::nullopt};
  if (std::optional<double> until = v.GetDict().FindDouble("until")) {
    block.until = FromMs(*until);
    if (*block.until <= now) {
      return std::nullopt;  // expired
    }
  }
  return block;
}

base::DictValue BlockToValue(const Block& block) {
  base::DictValue v;
  v.Set("since", Ms(block.since));
  if (block.until) {
    v.Set("until", Ms(*block.until));
  }
  return v;
}

}  // namespace

std::string_view YouTubeModeId(YouTubeMode mode) {
  switch (mode) {
    case YouTubeMode::kSite:
      return "site";
    case YouTubeMode::kMusic:
      return "music";
    case YouTubeMode::kShorts:
      return "shorts";
  }
}

std::optional<YouTubeMode> YouTubeModeFromId(std::string_view id) {
  for (YouTubeMode m :
       {YouTubeMode::kSite, YouTubeMode::kMusic, YouTubeMode::kShorts}) {
    if (YouTubeModeId(m) == id) {
      return m;
    }
  }
  return std::nullopt;
}

std::optional<base::Time> UntilFor(Duration duration, base::Time now) {
  switch (duration) {
    case Duration::kHour:
      return now + base::Hours(1);
    case Duration::kFourHours:
      return now + base::Hours(4);
    case Duration::kToday:
      // Next local midnight (stepping 36h from today's midnight survives DST).
      return (now.LocalMidnight() + base::Hours(36)).LocalMidnight();
    case Duration::kWeek:
      return now + base::Days(7);
    case Duration::kMonth:
      return now + base::Days(30);
    case Duration::kForever:
      return std::nullopt;
  }
}

std::string_view DurationId(Duration duration) {
  switch (duration) {
    case Duration::kHour:
      return "1h";
    case Duration::kFourHours:
      return "4h";
    case Duration::kToday:
      return "today";
    case Duration::kWeek:
      return "week";
    case Duration::kMonth:
      return "month";
    case Duration::kForever:
      return "forever";
  }
}

std::optional<Duration> DurationFromId(std::string_view id) {
  for (Duration d : kDurations) {
    if (DurationId(d) == id) {
      return d;
    }
  }
  return std::nullopt;
}

bool Outlasts(const std::optional<base::Time>& a,
              const std::optional<base::Time>& b) {
  return !a.has_value() || (b.has_value() && *a >= *b);
}

// static
MastBlocks& MastBlocks::Get() {
  // State lives in Local State; the only member is the short undo record.
  static MastBlocks instance;
  return instance;
}

// static
void MastBlocks::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kBlocksPref);
  registry->RegisterListPref(kCustomSitesPref);
  registry->RegisterStringPref(kLastDurationPref, "today");
  registry->RegisterStringPref(kBlockedDayPref, std::string());
  registry->RegisterIntegerPref(kBlockedCountPref, 0);
  base::ListValue distractions;
  for (const Category& c : Categories()) {
    distractions.Append(c.id);
  }
  registry->RegisterListPref(kDistractionsPref, std::move(distractions));
  registry->RegisterListPref(kDistractionsSkippedPref);
  registry->RegisterBooleanPref(kDistractionsYouTubeMusicPref, false);
  registry->RegisterStringPref(kDistractionsYouTubePref, std::string());
  registry->RegisterListPref(kUserCategoriesPref);
  registry->RegisterDictionaryPref(kAdditionsPref);
}

PrefService* MastBlocks::prefs() const {
  PrefService* p =
      g_browser_process ? g_browser_process->local_state() : nullptr;
  static bool migrated = false;
  if (p && !migrated) {
    migrated = true;
    MigrateCustomSites(p);
  }
  return p;
}

std::optional<Block> MastBlocks::Of(std::string_view domain) const {
  PrefService* p = prefs();
  if (!p) {
    return std::nullopt;
  }
  const base::Value* v = p->GetDict(kBlocksPref).Find(domain);
  return v ? ParseBlock(*v, base::Time::Now()) : std::nullopt;
}

std::optional<BlockMatch> MastBlocks::ForUrl(const GURL& url) const {
  PrefService* p = prefs();
  if (!p || !url.SchemeIsHTTPOrHTTPS()) {
    return std::nullopt;
  }
  const std::string host = NormalizeHost(url.host());
  if (host.empty() || IsExcepted(host)) {
    return std::nullopt;
  }
  const base::Time now = base::Time::Now();
  std::optional<BlockMatch> best;
  for (const auto [domain, value] : p->GetDict(kBlocksPref)) {
    if (!EntryMatches(host, domain)) {
      continue;
    }
    std::optional<Block> block = ParseBlock(value, now);
    if (block && (!best || Outlasts(block->until, best->block.until))) {
      best = BlockMatch{domain, *block};
    }
  }
  return best;
}

std::map<std::string, Block> MastBlocks::Active() const {
  std::map<std::string, Block> active;
  if (PrefService* p = prefs()) {
    const base::Time now = base::Time::Now();
    for (const auto [domain, value] : p->GetDict(kBlocksPref)) {
      if (std::optional<Block> block = ParseBlock(value, now)) {
        active.emplace(domain, *block);
      }
    }
  }
  return active;
}

bool MastBlocks::AnyActive() const {
  PrefService* p = prefs();
  if (!p) {
    return false;
  }
  const base::Time now = base::Time::Now();
  for (const auto [domain, value] : p->GetDict(kBlocksPref)) {
    if (ParseBlock(value, now)) {
      return true;
    }
  }
  return false;
}

int MastBlocks::BlockSites(const std::vector<std::string>& domains,
                           std::optional<base::Time> until) {
  PrefService* p = prefs();
  if (!p) {
    return 0;
  }
  const base::Time now = base::Time::Now();
  Recent recent{now, {}, {}};
  {
    ScopedDictPrefUpdate blocks(p, kBlocksPref);
    for (const std::string& input : domains) {
      const std::string domain =
          (IsFeedKey(input) && IsListed(input)) || IsYouTubeEntry(input)
              ? input
              : CleanDomain(input);
      if (domain.empty() || recent.before.contains(domain)) {
        continue;
      }
      const std::optional<Block> current = Of(domain);
      if (current && Outlasts(current->until, until)) {
        continue;  // already blocked at least that long
      }
      recent.before.emplace(domain, current);
      blocks->Set(domain, BlockToValue({current ? current->since : now, until}));
      if (!IsSpecialEntry(domain) && WhereListed(domain).empty() &&
          std::ranges::find(recent.added, domain) == recent.added.end()) {
        recent.added.push_back(domain);
      }
    }
  }
  if (recent.before.empty()) {
    return 0;
  }
  AddToYourSites(recent.added);
  p->CommitPendingWrite();
  const int changed = static_cast<int>(recent.before.size());
  const bool youtube = recent.before.contains(kYouTubeMusicKey) ||
                       recent.before.contains(kYouTubeShortsKey);
  recent_ = std::move(recent);
  ReloadBlockedTabs(youtube);
  return changed;
}

bool MastBlocks::CanUndo() const {
  return recent_ && base::Time::Now() - recent_->at < kUndoWindow;
}

bool MastBlocks::Undo() {
  PrefService* p = prefs();
  if (!p || !CanUndo()) {
    return false;
  }
  {
    ScopedDictPrefUpdate blocks(p, kBlocksPref);
    for (const auto& [domain, before] : recent_->before) {
      if (before) {
        blocks->Set(domain, BlockToValue(*before));
      } else {
        blocks->Remove(domain);
      }
    }
  }
  RemoveFromYourSites(recent_->added);
  const bool youtube = recent_->before.contains(kYouTubeMusicKey) ||
                       recent_->before.contains(kYouTubeShortsKey);
  recent_.reset();
  p->CommitPendingWrite();
  if (youtube) {
    ReloadBlockedTabs(/*youtube=*/true);  // the filter goes away
  }
  return true;
}

std::string MastBlocks::last_duration() const {
  PrefService* p = prefs();
  return p ? p->GetString(kLastDurationPref) : "today";
}

void MastBlocks::set_last_duration(std::string_view id) {
  if (PrefService* p = prefs(); p && DurationFromId(id)) {
    p->SetString(kLastDurationPref, std::string(id));
  }
}

std::vector<UserCategory> MastBlocks::UserCategories() const {
  PrefService* p = prefs();
  return p ? ReadUserCategories(p) : std::vector<UserCategory>();
}

std::map<std::string, std::vector<std::string>> MastBlocks::Additions() const {
  PrefService* p = prefs();
  return p ? ReadAdditions(p)
           : std::map<std::string, std::vector<std::string>>();
}

std::string MastBlocks::WhereListed(std::string_view domain) const {
  if (std::string name = CategoryNameOf(domain); !name.empty()) {
    return name;
  }
  for (const auto& [id, sites] : Additions()) {
    if (std::ranges::find(sites, domain) != sites.end()) {
      return BuiltIn(id)->name;
    }
  }
  for (const UserCategory& c : UserCategories()) {
    if (std::ranges::find(c.sites, domain) != c.sites.end()) {
      return c.name;
    }
  }
  return std::string();
}

MastBlocks::AddResult MastBlocks::AddSite(const std::string& category_id,
                                          const std::string& input) {
  AddResult r;
  PrefService* p = prefs();
  const std::string domain = CleanDomain(input);
  if (!p || domain.empty()) {
    r.error = "invalid";
    return r;
  }
  if (std::string where = WhereListed(domain); !where.empty()) {
    r.error = "listed";
    r.where = std::move(where);
    return r;
  }
  if (BuiltIn(category_id)) {
    auto additions = ReadAdditions(p);
    additions[category_id].push_back(domain);
    WriteAdditions(p, additions);
  } else {
    std::vector<UserCategory> categories = ReadUserCategories(p);
    auto it = std::ranges::find(categories, category_id, &UserCategory::id);
    if (it == categories.end()) {
      r.error = "unknown";
      return r;
    }
    it->sites.push_back(domain);
    WriteUserCategories(p, categories);
  }
  // Added again after a removal: in the set like the rest of its category.
  ScopedListPrefUpdate(p, kDistractionsSkippedPref)
      ->EraseValue(base::Value(DistractionKey(category_id, domain)));
  r.domain = domain;
  return r;
}

bool MastBlocks::RemoveSite(const std::string& category_id,
                            const std::string& domain) {
  PrefService* p = prefs();
  if (!p || Of(domain)) {
    return false;
  }
  bool removed = false;
  if (BuiltIn(category_id)) {
    auto additions = ReadAdditions(p);
    removed = std::erase(additions[category_id], domain) > 0;
    if (removed) {
      WriteAdditions(p, additions);
    }
  } else {
    std::vector<UserCategory> categories = ReadUserCategories(p);
    auto it = std::ranges::find(categories, category_id, &UserCategory::id);
    removed = it != categories.end() && std::erase(it->sites, domain) > 0;
    if (removed) {
      WriteUserCategories(p, categories);
    }
  }
  if (removed) {
    ScopedListPrefUpdate(p, kDistractionsSkippedPref)
        ->EraseValue(base::Value(DistractionKey(category_id, domain)));
  }
  return removed;
}

std::string MastBlocks::CreateCategory(const std::string& name) {
  PrefService* p = prefs();
  const std::string clean = CleanName(name);
  if (!p || clean.empty()) {
    return std::string();
  }
  std::vector<UserCategory> categories = ReadUserCategories(p);
  int last = 0;
  for (const UserCategory& c : categories) {
    int n = 0;
    if (c.id.starts_with("u") &&
        base::StringToInt(std::string_view(c.id).substr(1), &n)) {
      last = std::max(last, n);
    }
  }
  std::string id = base::StrCat({"u", base::NumberToString(last + 1)});
  categories.push_back({id, clean, {}});
  WriteUserCategories(p, categories);
  ScopedListPrefUpdate(p, kDistractionsPref)->Append(id);
  return id;
}

bool MastBlocks::RenameCategory(const std::string& id,
                                const std::string& name) {
  PrefService* p = prefs();
  const std::string clean = CleanName(name);
  if (!p || clean.empty()) {
    return false;
  }
  std::vector<UserCategory> categories = ReadUserCategories(p);
  auto it = std::ranges::find(categories, id, &UserCategory::id);
  if (it == categories.end()) {
    return false;
  }
  it->name = clean;
  WriteUserCategories(p, categories);
  return true;
}

bool MastBlocks::DeleteCategory(const std::string& id) {
  PrefService* p = prefs();
  if (!p) {
    return false;
  }
  std::vector<UserCategory> categories = ReadUserCategories(p);
  auto it = std::ranges::find(categories, id, &UserCategory::id);
  if (it == categories.end() ||
      std::ranges::any_of(it->sites,
                          [&](const std::string& d) { return Of(d).has_value(); })) {
    return false;
  }
  categories.erase(it);
  WriteUserCategories(p, categories);
  const std::string prefix = id + "/";
  ScopedListPrefUpdate(p, kDistractionsPref)->EraseValue(base::Value(id));
  ScopedListPrefUpdate(p, kDistractionsSkippedPref)
      ->EraseIf([&](const base::Value& v) {
        return v.is_string() && v.GetString().starts_with(prefix);
      });
  return true;
}

void MastBlocks::AddToYourSites(const std::vector<std::string>& domains) {
  PrefService* p = prefs();
  if (!p || domains.empty()) {
    return;
  }
  std::vector<UserCategory> categories = ReadUserCategories(p);
  auto it = std::ranges::find(categories, kYourSitesId, &UserCategory::id);
  if (it == categories.end()) {
    categories.push_back({kYourSitesId, "Your sites", {}});
    it = std::prev(categories.end());
    if (!InSet(p->GetList(kDistractionsPref), kYourSitesId)) {
      ScopedListPrefUpdate(p, kDistractionsPref)->Append(kYourSitesId);
    }
  }
  for (const std::string& d : domains) {
    if (std::ranges::find(it->sites, d) == it->sites.end()) {
      it->sites.push_back(d);
    }
  }
  WriteUserCategories(p, categories);
}

void MastBlocks::RemoveFromYourSites(const std::vector<std::string>& domains) {
  PrefService* p = prefs();
  if (!p || domains.empty()) {
    return;
  }
  std::vector<UserCategory> categories = ReadUserCategories(p);
  auto it = std::ranges::find(categories, kYourSitesId, &UserCategory::id);
  if (it == categories.end()) {
    return;
  }
  std::erase_if(it->sites, [&](const std::string& d) {
    return std::ranges::find(domains, d) != domains.end();
  });
  WriteUserCategories(p, categories);
}

void MastBlocks::RecordBlocked() {
  PrefService* p = prefs();
  if (!p) {
    return;
  }
  const std::string today = DayKey(base::Time::Now());
  if (p->GetString(kBlockedDayPref) != today) {
    p->SetString(kBlockedDayPref, today);
    p->SetInteger(kBlockedCountPref, 0);
  }
  p->SetInteger(kBlockedCountPref, p->GetInteger(kBlockedCountPref) + 1);
}

int MastBlocks::StoppedToday() const {
  PrefService* p = prefs();
  if (!p || p->GetString(kBlockedDayPref) != DayKey(base::Time::Now())) {
    return 0;
  }
  return p->GetInteger(kBlockedCountPref);
}

void MastBlocks::ReloadBlockedTabs(bool youtube) {
  std::set<content::WebContents*> seen;
  std::unique_ptr<content::RenderWidgetHostIterator> widgets =
      content::RenderWidgetHost::GetRenderWidgetHosts();
  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    content::RenderViewHost* view = content::RenderViewHost::From(widget);
    if (!view) {
      continue;
    }
    content::WebContents* contents =
        content::WebContents::FromRenderViewHost(view);
    if (!contents || !seen.insert(contents).second) {
      continue;
    }
    const GURL& url = contents->GetLastCommittedURL();
    if (IsBlocked(url) || (youtube && url.SchemeIsHTTPOrHTTPS() &&
                           IsYouTubeHost(url.host()))) {
      contents->GetController().Reload(content::ReloadType::NORMAL,
                                       /*check_for_repost=*/false);
    }
  }
}

MastBlocks::Distractions MastBlocks::GetDistractions() const {
  Distractions d;
  if (PrefService* p = prefs()) {
    d.categories = StringList(p->GetList(kDistractionsPref));
    d.skipped = StringList(p->GetList(kDistractionsSkippedPref));
    d.youtube = YouTubeModeFromId(p->GetString(kDistractionsYouTubePref))
                    .value_or(p->GetBoolean(kDistractionsYouTubeMusicPref)
                                  ? YouTubeMode::kMusic
                                  : YouTubeMode::kSite);
  }
  return d;
}

void MastBlocks::SetDistractions(const Distractions& d) {
  PrefService* p = prefs();
  if (!p) {
    return;
  }
  std::set<std::string> known_categories;
  std::set<std::string> known_sites;
  for (const Category& c : Categories()) {
    known_categories.insert(c.id);
    for (const Service& s : c.services) {
      known_sites.insert(DistractionKey(c.id, s.name));
    }
  }
  for (const auto& [id, sites] : ReadAdditions(p)) {
    for (const std::string& site : sites) {
      known_sites.insert(DistractionKey(id, site));
    }
  }
  for (const UserCategory& c : ReadUserCategories(p)) {
    known_categories.insert(c.id);
    for (const std::string& site : c.sites) {
      known_sites.insert(DistractionKey(c.id, site));
    }
  }
  // Sets, so a repeated id is stored once.
  std::set<std::string> categories;
  for (const std::string& id : d.categories) {
    if (known_categories.contains(id)) {
      categories.insert(id);
    }
  }
  std::set<std::string> skipped;
  for (const std::string& key : d.skipped) {
    if (known_sites.contains(key)) {
      skipped.insert(key);
    }
  }
  base::ListValue category_list;
  for (const std::string& id : categories) {
    category_list.Append(id);
  }
  base::ListValue skipped_list;
  for (const std::string& key : skipped) {
    skipped_list.Append(key);
  }
  p->SetList(kDistractionsPref, std::move(category_list));
  p->SetList(kDistractionsSkippedPref, std::move(skipped_list));
  p->SetString(kDistractionsYouTubePref, std::string(YouTubeModeId(d.youtube)));
  p->ClearPref(kDistractionsYouTubeMusicPref);
}

std::vector<std::string> MastBlocks::DistractionEntries() const {
  const Distractions d = GetDistractions();
  const std::set<std::string> wanted(d.categories.begin(), d.categories.end());
  const std::set<std::string> skipped(d.skipped.begin(), d.skipped.end());
  std::vector<std::string> entries;
  const auto sites = [&](std::string_view id,
                         const std::vector<std::string>& domains) {
    for (const std::string& site : domains) {
      if (!skipped.contains(DistractionKey(id, site))) {
        entries.push_back(site);
      }
    }
  };
  const std::map<std::string, std::vector<std::string>> additions =
      Additions();
  for (const Category& c : Categories()) {
    if (!wanted.contains(c.id)) {
      continue;
    }
    for (const Service& s : c.services) {
      if (skipped.contains(DistractionKey(c.id, s.name))) {
        continue;
      }
      if (IsYouTubeService(s) && d.youtube != YouTubeMode::kSite) {
        entries.push_back(d.youtube == YouTubeMode::kMusic ? kYouTubeMusicKey
                                                           : kYouTubeShortsKey);
      } else {
        entries.insert(entries.end(), s.domains.begin(), s.domains.end());
      }
    }
    if (const auto it = additions.find(c.id); it != additions.end()) {
      sites(c.id, it->second);
    }
  }
  for (const UserCategory& c : UserCategories()) {
    if (wanted.contains(c.id)) {
      sites(c.id, c.sites);
    }
  }
  // Music only and Shorts add nothing while all of YouTube is blocked.
  std::erase_if(entries, [&](const std::string& e) {
    return IsYouTubeEntry(e) && Of("youtube.com");
  });
  return entries;
}

std::string DistractionKey(std::string_view category_id,
                           std::string_view service_name) {
  return base::StrCat({category_id, "/", service_name});
}

bool IsYouTubeHost(std::string_view host) {
  return NormalizeHost(host) == "youtube.com";
}

std::optional<Block> MastBlocks::YouTubeMusicOnly() const {
  if (Of("youtube.com")) {
    return std::nullopt;  // the whole site is blocked
  }
  return Of(kYouTubeMusicKey);
}

std::optional<Block> MastBlocks::YouTubeShortsOnly() const {
  if (Of("youtube.com")) {
    return std::nullopt;
  }
  return Of(kYouTubeShortsKey);
}

}  // namespace mast
