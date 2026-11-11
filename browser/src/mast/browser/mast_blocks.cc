// Copyright 2026 The Mast Authors

#include "mast/browser/mast_blocks.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

#include "base/strings/strcat.h"
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
// The distraction set (MastBlocks::Distractions). Every category by default
// (design C28d: Block starts as "Block all"); the user unticks what they need.
// (Adult and gambling sites aren't categories: they're always blocked; see
// mast_feeds.h.)
constexpr char kDistractionsPref[] = "mast.distractions";
constexpr char kDistractionsSkippedPref[] = "mast.distractions_skipped";
constexpr char kDistractionsYouTubeMusicPref[] =
    "mast.distractions_youtube_music";

bool IsYouTubeService(const Service& s) {
  return std::ranges::find(s.domains, "youtube.com") != s.domains.end();
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

// Entries that aren't domains: feeds ("feed:…") and YouTube's Music only.
bool IsSpecialEntry(std::string_view entry) {
  return IsFeedKey(entry) || entry == kYouTubeMusicKey;
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
          (IsFeedKey(input) && IsListed(input)) || input == kYouTubeMusicKey
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
      if (!IsListed(domain) && !IsSpecialEntry(domain) &&
          std::ranges::find(custom, domain) == custom.end()) {
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
  const bool youtube = recent.before.contains(kYouTubeMusicKey);
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
  if (!recent_->added.empty()) {
    ScopedListPrefUpdate(p, kCustomSitesPref)->EraseIf([&](const base::Value& v) {
      return v.is_string() &&
             std::ranges::find(recent_->added, v.GetString()) != recent_->added.end();
    });
  }
  const bool youtube = recent_->before.contains(kYouTubeMusicKey);
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
  // Added again later, it starts out in the distraction set.
  ScopedListPrefUpdate(p, kDistractionsSkippedPref)
      ->EraseValue(base::Value(DistractionKey(kYourSitesId, domain)));
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
    d.youtube_music = p->GetBoolean(kDistractionsYouTubeMusicPref);
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
  for (const std::string& site : CustomSites()) {
    known_sites.insert(DistractionKey(kYourSitesId, site));
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
  p->SetBoolean(kDistractionsYouTubeMusicPref, d.youtube_music);
}

std::vector<std::string> MastBlocks::OpenDistractionDomains() const {
  const Distractions d = GetDistractions();
  const std::set<std::string> wanted(d.categories.begin(), d.categories.end());
  const std::set<std::string> skipped(d.skipped.begin(), d.skipped.end());
  std::vector<std::string> entries;
  for (const Category& c : Categories()) {
    if (!wanted.contains(c.id)) {
      continue;
    }
    for (const Service& s : c.services) {
      if (skipped.contains(DistractionKey(c.id, s.name))) {
        continue;
      }
      if (d.youtube_music && IsYouTubeService(s)) {
        entries.push_back(kYouTubeMusicKey);
      } else {
        entries.insert(entries.end(), s.domains.begin(), s.domains.end());
      }
    }
  }
  for (const std::string& site : CustomSites()) {
    if (!skipped.contains(DistractionKey(kYourSitesId, site))) {
      entries.push_back(site);
    }
  }
  // Only what's still open. Music only adds nothing while all of YouTube is
  // blocked.
  std::erase_if(entries, [&](const std::string& e) {
    return Of(e) || (e == kYouTubeMusicKey && Of("youtube.com"));
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

bool AnyBlockActive() {
  return MastBlocks::Get().AnyActive();
}

}  // namespace mast
