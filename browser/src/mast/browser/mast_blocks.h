// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_BLOCKS_H_
#define MAST_BROWSER_MAST_BLOCKS_H_

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/time/time.h"

class GURL;
class PrefRegistrySimple;
class PrefService;

namespace mast {

// How long a block lasts. `until` is empty for forever.
struct Block {
  base::Time since;
  std::optional<base::Time> until;
  bool forever() const { return !until.has_value(); }
};

struct BlockMatch {
  std::string domain;  // the blocked list entry, e.g. "youtube.com"
  Block block;
};

// The choices offered everywhere (new tab page and toolbar bubble).
enum class Duration { kHour, kFourHours, kToday, kWeek, kMonth, kForever };
inline constexpr Duration kDurations[] = {
    Duration::kHour, Duration::kFourHours, Duration::kToday,
    Duration::kWeek, Duration::kMonth,     Duration::kForever};
std::optional<base::Time> UntilFor(Duration duration, base::Time now);
std::string_view DurationId(Duration duration);
std::optional<Duration> DurationFromId(std::string_view id);

// Is `a` at least as long as `b`? Forever outlasts everything.
bool Outlasts(const std::optional<base::Time>& a,
              const std::optional<base::Time>& b);

// Per-site blocks. A block can be lengthened (up to forever) but never
// shortened or lifted. The only way back is a short undo right after blocking.
//
// State lives in Local State (outside every profile), so clearing browsing
// data or deleting a profile can't lift a block.
class MastBlocks {
 public:
  static constexpr base::TimeDelta kUndoWindow = base::Seconds(10);

  static MastBlocks& Get();
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  // The block in force on exactly this list entry.
  std::optional<Block> Of(std::string_view domain) const;
  // The block in force for a URL: the longest among matching entries.
  std::optional<BlockMatch> ForUrl(const GURL& url) const;
  bool IsBlocked(const GURL& url) const { return ForUrl(url).has_value(); }
  // Every block in force, by list entry.
  std::map<std::string, Block> Active() const;
  bool AnyActive() const;

  // Blocks `domains` until `until` (empty = forever). Entries already blocked
  // at least that long keep their block. Domains not on any list join the
  // user's own sites. Returns how many entries changed.
  int BlockSites(const std::vector<std::string>& domains,
                 std::optional<base::Time> until);
  bool CanUndo() const;
  // Only inside kUndoWindow after BlockSites(). Returns whether it undid.
  bool Undo();

  std::string last_duration() const;
  void set_last_duration(std::string_view id);

  std::vector<std::string> CustomSites() const;
  // Adding is always allowed. Returns the cleaned domain, or empty if invalid.
  std::string AddSite(const std::string& input);
  // Only sites that aren't blocked can be removed.
  bool RemoveSite(const std::string& domain);

  // Called by the navigation throttle each time it shows the block page.
  void RecordBlocked();
  int StoppedToday() const;

 private:
  struct Recent {
    base::Time at;
    std::map<std::string, std::optional<Block>> before;
    std::vector<std::string> added;
  };

  PrefService* prefs() const;
  // Tabs already showing a blocked site switch to the block page.
  void ReloadBlockedTabs();

  std::optional<Recent> recent_;
};

// Shorthand for hooks in upstream code: true while any site is blocked.
bool AnyBlockActive();

}  // namespace mast

#endif  // MAST_BROWSER_MAST_BLOCKS_H_
