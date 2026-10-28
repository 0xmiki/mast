// Copyright 2026 The Mast Authors

#include "mast/browser/mast_session.h"

#include <algorithm>
#include <set>

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
  p->CommitPendingWrite();
}

bool MastSession::Undo() {
  PrefService* p = prefs();
  if (!p || !CanUndo()) {
    return false;
  }
  p->SetTime(kEndPref, base::Time::Now());
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
