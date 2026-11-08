// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_FEEDS_H_
#define MAST_BROWSER_MAST_FEEDS_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace mast {

// Large maintained blocklists shipped beside the browser as data files
// (<exe dir>/mast-feeds/<id>.txt, made by remote/build-feeds.py):
//   - "adult" and "gambling": always blocked, in every window, outside the
//     blocklist (the blocklist panel says so in one line). A feed can also back
//     a category's last row (list entry FeedKey(id)); none does now.
// Each line is a domain that also covers its subdomains.
inline constexpr char kAdultFeed[] = "adult";
inline constexpr char kGamblingFeed[] = "gambling";
inline constexpr char kFeedKeyPrefix[] = "feed:";

// "gambling" -> "feed:gambling", the feed's entry in the blocks and the list.
std::string FeedKey(std::string_view id);
// True for "feed:..." entries; they never collide with domains (no ':' in hosts).
bool IsFeedKey(std::string_view entry);

class MastFeeds {
 public:
  static MastFeeds& Get();

  MastFeeds(const MastFeeds&) = delete;
  MastFeeds& operator=(const MastFeeds&) = delete;

  // Starts reading the files (off the UI thread) if that hasn't started yet.
  void Load();
  // The files have been read (or found missing: then every feed is empty).
  bool ready() const { return ready_; }
  // Runs `callback` once ready (right away if it already is).
  void WhenReady(base::OnceClosure callback);

  // True if `host` (normalized, lower case) or a parent domain is in feed `id`.
  // False until ready.
  bool Contains(std::string_view id, std::string_view host) const;

 private:
  struct Feed {
    std::string id;
    base::flat_set<std::string, std::less<>> domains;
  };

  friend class base::NoDestructor<MastFeeds>;

  MastFeeds();
  ~MastFeeds();
  void OnLoaded(std::vector<Feed> feeds);

  bool loading_ = false;
  bool ready_ = false;
  std::vector<Feed> feeds_;
  std::vector<base::OnceClosure> waiting_;
};

// True if `host` is on an always-on list (adult, gambling).
bool IsAlwaysBlocked(std::string_view host);

}  // namespace mast

#endif  // MAST_BROWSER_MAST_FEEDS_H_
