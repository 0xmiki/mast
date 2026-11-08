// Copyright 2026 The Mast Authors

#include "mast/browser/mast_blocks.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

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
constexpr char kCustomSitesPref[] = "mast.custom_sites";
constexpr char kLastDurationPref[] = "mast.last_duration";
constexpr char kBlockedDayPref[] = "mast.stats.blocked_day";
constexpr char kBlockedCountPref[] = "mast.stats.blocked_count";
constexpr char kDistractionsPref[] = "mast.distractions";
// The version of the default blocks this install has applied (0: none yet).
constexpr char kDefaultBlocksPref[] = "mast.default_blocks";
constexpr int kDefaultBlocksVersion = 1;
// Categories blocked forever out of the box.
constexpr const char* kDefaultBlockedCategories[] = {"gambling"};

// Blocked by "Block distractions" until someone changes the set. Shopping,
// games, trading, dating and messaging stay open: people often need them.
// (Adult sites aren't a category: they're always blocked; see mast_feeds.h.)
constexpr const char* kDefaultDistractions[] = {
    "social", "video",  "streaming", "news",  "forums",
    "entertainment", "sports", "gambling"};

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
  for (const char* id : kDefaultDistractions) {
    distractions.Append(id);
  }
  registry->RegisterListPref(kDistractionsPref, std::move(distractions));
  registry->RegisterIntegerPref(kDefaultBlocksPref, 0);
}

void MastBlocks::ApplyDefaultBlocks() {
  PrefService* p = prefs();
  if (!p || p->GetInteger(kDefaultBlocksPref) >= kDefaultBlocksVersion) {
    return;
  }
  // Written directly, not through BlockSites(): no undo, no "just blocked"
  // toast, and nothing joins the user's own sites.
  const base::Time now = base::Time::Now();
  {
    ScopedDictPrefUpdate blocks(p, kBlocksPref);
    for (const Category& c : Categories()) {
      if (std::ranges::none_of(kDefaultBlockedCategories,
                               [&](const char* id) { return c.id == id; })) {
        continue;
      }
      for (const Service& s : c.services) {
        for (const std::string& domain : s.domains) {
          const std::optional<Block> current = Of(domain);
          if (current && current->forever()) {
            continue;
          }
          blocks->Set(domain, BlockToValue({current ? current->since : now,
                                            std::nullopt}));
        }
      }
    }
  }
  p->SetInteger(kDefaultBlocksPref, kDefaultBlocksVersion);
  p->CommitPendingWrite();
  ReloadBlockedTabs();
}

PrefService* MastBlocks::prefs() const {
  return g_browser_process ? g_browser_process->local_state() : nullptr;
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
  std::vector<std::string> custom = CustomSites();
  {
    ScopedDictPrefUpdate blocks(p, kBlocksPref);
    for (const std::string& input : domains) {
      const std::string domain =
          IsFeedKey(input) && IsListed(input) ? input : CleanDomain(input);
      if (domain.empty() || recent.before.contains(domain)) {
        continue;
      }
      const std::optional<Block> current = Of(domain);
      if (current && Outlasts(current->until, until)) {
        continue;  // already blocked at least that long
      }
      recent.before.emplace(domain, current);
      blocks->Set(domain, BlockToValue({current ? current->since : now, until}));
      if (!IsListed(domain) && std::ranges::find(custom, domain) == custom.end()) {
        custom.push_back(domain);
        recent.added.push_back(domain);
      }
    }
  }
  if (recent.before.empty()) {
    return 0;
  }
  if (!recent.added.empty()) {
    ScopedListPrefUpdate list(p, kCustomSitesPref);
    for (const std::string& d : recent.added) {
      list->Append(d);
    }
  }
  p->CommitPendingWrite();
  const int changed = static_cast<int>(recent.before.size());
  recent_ = std::move(recent);
  ReloadBlockedTabs();
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
  if (!recent_->added.empty()) {
    ScopedListPrefUpdate(p, kCustomSitesPref)->EraseIf([&](const base::Value& v) {
      return v.is_string() &&
             std::ranges::find(recent_->added, v.GetString()) != recent_->added.end();
    });
  }
  recent_.reset();
  p->CommitPendingWrite();
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

std::vector<std::string> MastBlocks::CustomSites() const {
  std::vector<std::string> sites;
  if (PrefService* p = prefs()) {
    for (const base::Value& v : p->GetList(kCustomSitesPref)) {
      if (v.is_string()) {
        sites.push_back(v.GetString());
      }
    }
  }
  return sites;
}

std::string MastBlocks::AddSite(const std::string& input) {
  PrefService* p = prefs();
  const std::string domain = CleanDomain(input);
  if (!p || domain.empty() || IsListed(domain)) {
    return std::string();
  }
  const std::vector<std::string> custom = CustomSites();
  if (std::ranges::find(custom, domain) == custom.end()) {
    ScopedListPrefUpdate(p, kCustomSitesPref)->Append(domain);
  }
  return domain;
}

bool MastBlocks::RemoveSite(const std::string& domain) {
  PrefService* p = prefs();
  if (!p || Of(domain)) {
    return false;
  }
  ScopedListPrefUpdate(p, kCustomSitesPref)->EraseIf([&](const base::Value& v) {
    return v.is_string() && v.GetString() == domain;
  });
  return true;
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

void MastBlocks::ReloadBlockedTabs() {
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
    if (IsBlocked(contents->GetLastCommittedURL())) {
      contents->GetController().Reload(content::ReloadType::NORMAL,
                                       /*check_for_repost=*/false);
    }
  }
}

std::vector<std::string> MastBlocks::DistractionCategories() const {
  std::vector<std::string> ids;
  if (PrefService* p = prefs()) {
    for (const base::Value& v : p->GetList(kDistractionsPref)) {
      if (v.is_string()) {
        ids.push_back(v.GetString());
      }
    }
  }
  return ids;
}

void MastBlocks::SetDistraction(std::string_view category_id, bool on) {
  PrefService* p = prefs();
  const bool known = std::ranges::any_of(
      Categories(), [&](const Category& c) { return c.id == category_id; });
  if (!p || !known) {
    return;
  }
  ScopedListPrefUpdate update(p, kDistractionsPref);
  update.Get().EraseValue(base::Value(category_id));
  if (on) {
    update.Get().Append(category_id);
  }
}

std::vector<std::string> MastBlocks::DistractionDomains() const {
  const std::vector<std::string> ids = DistractionCategories();
  const std::set<std::string> wanted(ids.begin(), ids.end());
  std::vector<std::string> domains;
  for (const Category& c : Categories()) {
    if (!wanted.contains(c.id)) {
      continue;
    }
    for (const Service& s : c.services) {
      domains.insert(domains.end(), s.domains.begin(), s.domains.end());
    }
  }
  const std::vector<std::string> custom = CustomSites();
  domains.insert(domains.end(), custom.begin(), custom.end());
  return domains;
}

bool AnyBlockActive() {
  return MastBlocks::Get().AnyActive();
}

}  // namespace mast
