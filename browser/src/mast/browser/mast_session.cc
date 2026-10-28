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
constexpr char kShortcutsPref[] = "mast.shortcuts";

constexpr size_t kMaxLog = 200;
constexpr size_t kMaxShortcuts = 10;

double Ms(base::Time t) {
  return static_cast<double>(t.InMillisecondsSinceUnixEpoch());
}

std::string DayKey(base::Time t) {
  base::Time::Exploded e;
  t.LocalExplode(&e);
  return base::StringPrintf("%04d-%02d-%02d", e.year, e.month, e.day_of_month);
}

base::ListValue DefaultShortcuts() {
  base::ListValue list;
  for (const auto& [title, url] : std::initializer_list<std::pair<const char*, const char*>>{
           {"Gmail", "https://mail.google.com/"},
           {"Calendar", "https://calendar.google.com/"},
           {"Docs", "https://docs.google.com/"},
           {"GitHub", "https://github.com/"},
           {"Wikipedia", "https://www.wikipedia.org/"},
       }) {
    list.Append(base::DictValue().Set("title", title).Set("url", url));
  }
  return list;
}

constexpr int kMaxMinutes = 12 * 60;

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
  registry->RegisterListPref(kShortcutsPref, DefaultShortcuts());
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

std::vector<Shortcut> MastSession::Shortcuts() const {
  std::vector<Shortcut> out;
  if (PrefService* p = prefs()) {
    for (const base::Value& v : p->GetList(kShortcutsPref)) {
      if (!v.is_dict()) {
        continue;
      }
      const std::string* title = v.GetDict().FindString("title");
      const std::string* url = v.GetDict().FindString("url");
      if (url) {
        out.push_back({title ? *title : std::string(), *url});
      }
    }
  }
  return out;
}

bool MastSession::AddShortcut(const std::string& input) {
  PrefService* p = prefs();
  const std::string domain = CleanDomain(input);
  if (!p || domain.empty() || Shortcuts().size() >= kMaxShortcuts) {
    return false;
  }
  const std::string url = "https://" + domain + "/";
  for (const Shortcut& s : Shortcuts()) {
    if (s.url == url) {
      return false;
    }
  }
  // "news.example.co.uk" -> "News"; good enough for a tile label.
  std::string title = domain.substr(0, domain.find('.'));
  if (!title.empty()) {
    title[0] = base::ToUpperASCII(title[0]);
  }
  ScopedListPrefUpdate(p, kShortcutsPref)
      ->Append(base::DictValue().Set("title", title).Set("url", url));
  return true;
}

bool MastSession::RemoveShortcut(const std::string& url) {
  PrefService* p = prefs();
  if (!p) {
    return false;
  }
  ScopedListPrefUpdate(p, kShortcutsPref)->EraseIf([&](const base::Value& v) {
    const std::string* u = v.is_dict() ? v.GetDict().FindString("url") : nullptr;
    return u && *u == url;
  });
  return true;
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
