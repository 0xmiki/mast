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

// YouTube, music only (design C24): a block on this entry leaves YouTube open
// for music (mixes and songs) and blocks every other video and Shorts. Not a
// domain, so it never blocks youtube.com by itself; mast_youtube.cc and the
// navigation throttle act on it. Blocked like any entry: picked in the list,
// lengthened but never shortened.
inline constexpr char kYouTubeMusicKey[] = "music:youtube.com";
// YouTube, Shorts only (design C32): YouTube stays open, Shorts don't (the
// feed's shelves are hidden and /shorts/ addresses get the blocked page).
inline constexpr char kYouTubeShortsKey[] = "shorts:youtube.com";

// What the distraction set does to YouTube while it's in the set.
enum class YouTubeMode { kSite, kMusic, kShorts };
std::string_view YouTubeModeId(YouTubeMode mode);
std::optional<YouTubeMode> YouTubeModeFromId(std::string_view id);

// True for youtube.com and www./m. (not music.youtube.com, which stays open).
bool IsYouTubeHost(std::string_view host);

// "video/YouTube": how the distraction set names a site, by its category id
// and its name (the domain, for a site the user added).
std::string DistractionKey(std::string_view category_id,
                           std::string_view service_name);
// The user's own category that sites blocked from the toolbar join (and that
// the old "Your sites" list became).
inline constexpr char kYourSitesId[] = "yours";

// A category of the user's own (design C33): a name and its sites. Ids are
// "yours" or "u<n>", never a built-in category's.
struct UserCategory {
  std::string id;
  std::string name;
  std::vector<std::string> sites;
};

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
  // The YouTube "Music only" block in force, unless the whole site is blocked
  // (that wins).
  std::optional<Block> YouTubeMusicOnly() const;
  // The YouTube "Shorts" block in force, unless the whole site is blocked.
  // (Music only hides Shorts too; mast_youtube.cc runs both.)
  std::optional<Block> YouTubeShortsOnly() const;

  // Blocks `domains` until `until` (empty = forever). Entries already blocked
  // at least that long keep their block. Domains not in any category join
  // the user's own "Your sites". Returns how many entries changed.
  int BlockSites(const std::vector<std::string>& domains,
                 std::optional<base::Time> until);
  bool CanUndo() const;
  // Only inside kUndoWindow after BlockSites(). Returns whether it undid.
  bool Undo();

  std::string last_duration() const;
  void set_last_duration(std::string_view id);

  // The user's own categories, in the order they were made.
  std::vector<UserCategory> UserCategories() const;
  // Sites the user added to built-in categories, by category id.
  std::map<std::string, std::vector<std::string>> Additions() const;

  // Adds a site to a category, built-in or the user's own (design C33).
  // Adding is always allowed. A domain lives in one category only: one
  // that's listed somewhere already isn't added again.
  struct AddResult {
    std::string domain;  // the cleaned domain, once added
    std::string error;   // "invalid", "listed" (see where) or "unknown"
    std::string where;   // the category it's listed in already
  };
  AddResult AddSite(const std::string& category_id, const std::string& input);
  // Only sites the user added, and only while they aren't blocked. Mast's own
  // sites are never removed, only left out of the set.
  bool RemoveSite(const std::string& category_id, const std::string& domain);

  // A new category of the user's own, in the distraction set from the start.
  // Returns its id, or empty if the name is.
  std::string CreateCategory(const std::string& name);
  bool RenameCategory(const std::string& id, const std::string& name);
  // Only the user's own, and only while none of its sites is blocked.
  bool DeleteCategory(const std::string& id);

  // Called by the navigation throttle each time it shows the block page.
  void RecordBlocked();
  int StoppedToday() const;

  // "Distractions": what Block blocks, from the new tab's dock and from the
  // blocklist panel alike (design C28d). Every category is in it until the
  // user takes it out.
  struct Distractions {
    // Category ids from blocklist.json, e.g. "social".
    std::vector<std::string> categories;
    // Sites left out inside those categories, by DistractionKey() (e.g.
    // "video/Twitch"). The user's own sites are always in, unless left out
    // as "yours/<domain>", so a site added later starts out in the set.
    std::vector<std::string> skipped;
    // YouTube, while it's in the set: the whole site, Music only (C24), or
    // just Shorts (C32).
    YouTubeMode youtube = YouTubeMode::kSite;
  };
  Distractions GetDistractions() const;
  // Unknown categories and sites are dropped.
  void SetDistractions(const Distractions& distractions);
  // The entries Block would block that aren't blocked yet: blocking the set
  // adds what's still open and never touches a block already running.
  std::vector<std::string> OpenDistractionDomains() const;

 private:
  struct Recent {
    base::Time at;
    std::map<std::string, std::optional<Block>> before;
    std::vector<std::string> added;
  };

  PrefService* prefs() const;
  // Tabs already showing a blocked site switch to the block page; with
  // `youtube`, YouTube tabs reload too (Music only or Shorts started or was
  // undone).
  void ReloadBlockedTabs(bool youtube = false);
  // The category a domain is listed in already (built-in, added there by the
  // user, or the user's own): its name, or empty.
  std::string WhereListed(std::string_view domain) const;
  // Domains blocked from outside any category (the toolbar's bubble) join the
  // user's own "Your sites", made if it isn't there.
  void AddToYourSites(const std::vector<std::string>& domains);
  void RemoveFromYourSites(const std::vector<std::string>& domains);

  std::optional<Recent> recent_;
};

}  // namespace mast

#endif  // MAST_BROWSER_MAST_BLOCKS_H_
