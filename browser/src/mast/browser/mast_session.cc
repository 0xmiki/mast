// Copyright 2026 The Mast Authors

#include "mast/browser/mast_session.h"

#include <algorithm>
#include <initializer_list>
#include <set>
#include <utility>

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
#include "url/gurl.h"

namespace mast {

namespace {

constexpr char kStartPref[] = "mast.session.start";
constexpr char kEndPref[] = "mast.session.end";
constexpr char kLastMinutesPref[] = "mast.session.last_minutes";
constexpr char kCustomSitesPref[] = "mast.custom_sites";
// [{start: ms, end: ms}], newest last; trimmed to kMaxLog.
constexpr char kSessionLogPref[] = "mast.session_log";
constexpr char kBlockedDayPref[] = "mast.stats.blocked_day";
constexpr char kBlockedCountPref[] = "mast.stats.blocked_count";

constexpr size_t kMaxLog = 200;

double Ms(base::Time t) {
  return static_cast<double>(t.InMillisecondsSinceUnixEpoch());
}

std::string DayKey(base::Time t) {
  base::Time::Exploded e;
  t.LocalExplode(&e);
  return base::StringPrintf("%04d-%02d-%02d", e.year, e.month, e.day_of_month);
}

}  // namespace

// static
MastSession& MastSession::Get() {
  // Stateless (everything lives in Local State), so a plain static is fine.
  static MastSession instance;
  return instance;
}

// static
void MastSession::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kStartPref, base::Time());
  registry->RegisterTimePref(kEndPref, base::Time());
  registry->RegisterIntegerPref(kLastMinutesPref, 50);
  registry->RegisterListPref(kCustomSitesPref);
  registry->RegisterListPref(kSessionLogPref);
  registry->RegisterStringPref(kBlockedDayPref, std::string());
  registry->RegisterIntegerPref(kBlockedCountPref, 0);
}

PrefService* MastSession::prefs() const {
  return g_browser_process ? g_browser_process->local_state() : nullptr;
}

bool MastSession::IsActive() const {
  PrefService* p = prefs();
  return p && base::Time::Now() < p->GetTime(kEndPref);
}

base::Time MastSession::start_time() const {
  PrefService* p = prefs();
  return p ? p->GetTime(kStartPref) : base::Time();
}

base::Time MastSession::end_time() const {
  PrefService* p = prefs();
  return p ? p->GetTime(kEndPref) : base::Time();
}

bool MastSession::CanUndo() const {
  return IsActive() && base::Time::Now() - start_time() < kUndoWindow;
}

int MastSession::last_minutes() const {
  PrefService* p = prefs();
  return p ? p->GetInteger(kLastMinutesPref) : 50;
}

void MastSession::Start(int minutes) {
  PrefService* p = prefs();
  if (!p || IsActive()) {
    return;
  }
  minutes = std::clamp(minutes, 1, kMaxMinutes);
  const base::Time now = base::Time::Now();
  p->SetTime(kStartPref, now);
  p->SetTime(kEndPref, now + base::Minutes(minutes));
  p->SetInteger(kLastMinutesPref, minutes);
  {
    ScopedListPrefUpdate log(p, kSessionLogPref);
    log->Append(base::DictValue()
                    .Set("start", Ms(now))
                    .Set("end", Ms(now + base::Minutes(minutes))));
    while (log->size() > kMaxLog) {
      log->erase(log->begin());
    }
  }
  p->CommitPendingWrite();
  ReloadBlockedTabs();
}

void MastSession::Extend(int minutes) {
  PrefService* p = prefs();
  if (!p || !IsActive() || minutes <= 0) {
    return;
  }
  minutes = std::min(minutes, kMaxMinutes);
  p->SetTime(kEndPref, end_time() + base::Minutes(minutes));
  {
    ScopedListPrefUpdate log(p, kSessionLogPref);
    if (!log->empty() && log->back().is_dict()) {
      log->back().GetDict().Set("end", Ms(end_time()));
    }
  }
  p->CommitPendingWrite();
}

bool MastSession::Undo() {
  PrefService* p = prefs();
  if (!p || !CanUndo()) {
    return false;
  }
  p->SetTime(kEndPref, base::Time::Now());
  {
    // An undone session never happened: drop it from the stats.
    ScopedListPrefUpdate log(p, kSessionLogPref);
    if (!log->empty()) {
      log->erase(log->end() - 1);
    }
  }
  p->CommitPendingWrite();
  return true;
}

std::vector<std::string> MastSession::CustomSites() const {
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

bool MastSession::AddSite(const std::string& input) {
  PrefService* p = prefs();
  const std::string domain = CleanDomain(input);
  if (!p || domain.empty()) {
    return false;
  }
  const auto& presets = PresetSites();
  const std::vector<std::string> custom = CustomSites();
  if (std::ranges::find(presets, domain) != presets.end() ||
      std::ranges::find(custom, domain) != custom.end()) {
    return false;
  }
  ScopedListPrefUpdate(p, kCustomSitesPref)->Append(domain);
  if (IsActive()) {
    ReloadBlockedTabs();
  }
  return true;
}

bool MastSession::RemoveSite(const std::string& domain) {
  PrefService* p = prefs();
  if (!p || IsActive()) {
    return false;
  }
  ScopedListPrefUpdate(p, kCustomSitesPref)->EraseIf([&](const base::Value& v) {
    return v.is_string() && v.GetString() == domain;
  });
  return true;
}

bool MastSession::IsBlocked(const GURL& url) const {
  if (!url.SchemeIsHTTPOrHTTPS() || !IsActive()) {
    return false;
  }
  const std::string host = NormalizeHost(url.host());
  for (const std::string& domain : PresetSites()) {
    if (HostMatches(host, domain)) {
      return true;
    }
  }
  for (const std::string& domain : CustomSites()) {
    if (HostMatches(host, domain)) {
      return true;
    }
  }
  return false;
}

void MastSession::RecordBlocked() {
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

// Focus time inside [from, to) across the session log.
base::TimeDelta FocusBetween(const base::ListValue& log,
                             base::Time from,
                             base::Time to) {
  base::TimeDelta total;
  for (const base::Value& v : log) {
    if (!v.is_dict()) {
      continue;
    }
    const base::Time start = base::Time::FromMillisecondsSinceUnixEpoch(
        v.GetDict().FindDouble("start").value_or(0));
    const base::Time end = base::Time::FromMillisecondsSinceUnixEpoch(
        v.GetDict().FindDouble("end").value_or(0));
    const base::Time a = std::max(start, from);
    const base::Time b = std::min(end, to);
    if (b > a) {
      total += b - a;
    }
  }
  return total;
}

std::vector<DayFocus> MastSession::Week() const {
  static constexpr const char* kDays[] = {"Sun", "Mon", "Tue", "Wed",
                                          "Thu", "Fri", "Sat"};
  std::vector<DayFocus> week;
  PrefService* p = prefs();
  const base::Time now = base::Time::Now();
  const base::Time today = now.LocalMidnight();
  for (int i = 6; i >= 0; --i) {
    // Midnight i days ago (stepping by 12h-offset days survives DST shifts).
    const base::Time day =
        (today - base::Days(i) + base::Hours(12)).LocalMidnight();
    const base::Time next = i == 0 ? now : (day + base::Hours(36)).LocalMidnight();
    base::Time::Exploded e;
    day.LocalExplode(&e);
    week.push_back({kDays[e.day_of_week],
                    p ? FocusBetween(p->GetList(kSessionLogPref), day, next)
                      : base::TimeDelta()});
  }
  return week;
}

TodayStats MastSession::Today() const {
  TodayStats stats;
  PrefService* p = prefs();
  if (!p) {
    return stats;
  }
  const base::Time now = base::Time::Now();
  const base::Time midnight = now.LocalMidnight();
  for (const base::Value& v : p->GetList(kSessionLogPref)) {
    if (!v.is_dict()) {
      continue;
    }
    const base::Time start = base::Time::FromMillisecondsSinceUnixEpoch(
        v.GetDict().FindDouble("start").value_or(0));
    const base::Time end = base::Time::FromMillisecondsSinceUnixEpoch(
        v.GetDict().FindDouble("end").value_or(0));
    const base::Time from = std::max(start, midnight);
    const base::Time to = std::min(end, now);
    if (to > from) {
      stats.focus += to - from;
    }
    if (start >= midnight) {
      stats.sessions++;
    }
  }
  if (p->GetString(kBlockedDayPref) == DayKey(now)) {
    stats.blocked = p->GetInteger(kBlockedCountPref);
  }
  return stats;
}

void MastSession::ReloadBlockedTabs() {
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

bool IsSessionActive() {
  return MastSession::Get().IsActive();
}

}  // namespace mast
