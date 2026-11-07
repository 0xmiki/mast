// Copyright 2026 The Mast Authors

#include "mast/browser/mast_feeds.h"

#include <utility>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"

namespace mast {

namespace {

constexpr const char* kFeedIds[] = {kAdultFeed, "gambling"};

base::flat_set<std::string, std::less<>> ReadFeed(const base::FilePath& path) {
  std::string text;
  if (!base::ReadFileToString(path, &text)) {
    LOG(WARNING) << "Mast: no blocklist feed at " << path;
    return {};
  }
  std::vector<std::string> domains;
  for (std::string_view line : base::SplitStringPiece(
           text, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (!line.starts_with('#')) {
      domains.push_back(base::ToLowerASCII(line));
    }
  }
  // The files are sorted already; flat_set sorts and de-duplicates anyway.
  return base::flat_set<std::string, std::less<>>(std::move(domains));
}

}  // namespace

std::string FeedKey(std::string_view id) {
  return std::string(kFeedKeyPrefix) + std::string(id);
}

bool IsFeedKey(std::string_view entry) {
  return entry.starts_with(kFeedKeyPrefix);
}

// static
MastFeeds& MastFeeds::Get() {
  static base::NoDestructor<MastFeeds> instance;
  return *instance;
}

MastFeeds::MastFeeds() = default;
MastFeeds::~MastFeeds() = default;

void MastFeeds::Load() {
  if (loading_ || ready_) {
    return;
  }
  loading_ = true;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce([] {
        base::FilePath dir;
        base::PathService::Get(base::DIR_ASSETS, &dir);
        dir = dir.AppendASCII("mast-feeds");
        std::vector<Feed> feeds;
        for (const char* id : kFeedIds) {
          feeds.push_back(
              {id, ReadFeed(dir.AppendASCII(std::string(id) + ".txt"))});
        }
        return feeds;
      }),
      base::BindOnce(&MastFeeds::OnLoaded, base::Unretained(this)));
}

void MastFeeds::OnLoaded(std::vector<Feed> feeds) {
  feeds_ = std::move(feeds);
  loading_ = false;
  ready_ = true;
  for (base::OnceClosure& callback : std::exchange(waiting_, {})) {
    std::move(callback).Run();
  }
}

void MastFeeds::WhenReady(base::OnceClosure callback) {
  if (ready_) {
    std::move(callback).Run();
    return;
  }
  waiting_.push_back(std::move(callback));
  Load();
}

bool MastFeeds::Contains(std::string_view id, std::string_view host) const {
  for (const Feed& feed : feeds_) {
    if (feed.id != id) {
      continue;
    }
    // "a.b.example.com" -> try it, then "b.example.com", "example.com", "com".
    for (std::string_view h = host; !h.empty();) {
      if (feed.domains.contains(h)) {
        return true;
      }
      const size_t dot = h.find('.');
      if (dot == std::string_view::npos) {
        break;
      }
      h.remove_prefix(dot + 1);
    }
  }
  return false;
}

bool IsAlwaysBlocked(std::string_view host) {
  return MastFeeds::Get().Contains(kAdultFeed, host);
}

}  // namespace mast
