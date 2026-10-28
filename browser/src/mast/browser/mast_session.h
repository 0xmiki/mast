// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_SESSION_H_
#define MAST_BROWSER_MAST_SESSION_H_

#include <string>
#include <vector>

#include "base/time/time.h"

class GURL;
class PrefRegistrySimple;
class PrefService;

namespace mast {

// Today's numbers for the new tab page.
struct TodayStats {
  base::TimeDelta focus;  // deep work so far today, including a running session
  int blocked = 0;        // blocked page loads today
  int sessions = 0;       // sessions started today
};

// One day of the week strip on the new tab page.
struct DayFocus {
  std::string label;  // "Mon"
  base::TimeDelta focus;
};

// The browser-wide deep-work session. While it runs, every site on the
// blocklist is blocked. It can be extended but never shortened or stopped,
// apart from a short undo right after starting.
//
// State lives in Local State (outside every profile), so clearing browsing
// data or deleting a profile can't end a session.
class MastSession {
 public:
  static constexpr base::TimeDelta kUndoWindow = base::Seconds(10);

  static MastSession& Get();
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  bool IsActive() const;
  base::Time start_time() const;
  base::Time end_time() const;
  bool CanUndo() const;
  int last_minutes() const;

  // No-ops while a session is already running.
  void Start(int minutes);
  // Sessions only ever get longer.
  void Extend(int minutes);
  // Only inside kUndoWindow after Start(). Returns whether it undid.
  bool Undo();

  std::vector<std::string> CustomSites() const;
  // Adding is always allowed (it only makes things stricter).
  bool AddSite(const std::string& input);
  // Removing only between sessions.
  bool RemoveSite(const std::string& domain);

  bool IsBlocked(const GURL& url) const;

  // Called by the navigation throttle each time it shows the block page.
  void RecordBlocked();
  TodayStats Today() const;
  // The last 7 days, oldest first, ending with today.
  std::vector<DayFocus> Week() const;

 private:
  PrefService* prefs() const;
  // Tabs already showing a blocked site switch to the block page.
  void ReloadBlockedTabs();
};

// Shorthand for hooks in upstream code.
bool IsSessionActive();

}  // namespace mast

#endif  // MAST_BROWSER_MAST_SESSION_H_
