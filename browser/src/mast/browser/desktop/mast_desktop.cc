// Copyright 2026 The Mast Authors

#include "mast/browser/desktop/mast_desktop.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/power_bookmarks/core/suggested_save_location_provider.h"
#include "url/gurl.h"

namespace mast {

namespace {

constexpr char kDesktopMetaKey[] = "mast_desktop";

// What the folder is called wherever bookmarks are listed (the bookmark
// manager): it is what the new tab page shows. Early builds called it
// "Desktop", which read as the computer's desktop; those get renamed.
constexpr char16_t kFolderTitle[] = u"New tab";
constexpr char16_t kOldFolderTitle[] = u"Desktop";

const bookmarks::BookmarkNode* Find(const bookmarks::BookmarkNode* node) {
  std::string value;
  if (node->GetMetaInfo(kDesktopMetaKey, &value) && value == "1") {
    return node;
  }
  for (const auto& child : node->children()) {
    if (child->is_folder()) {
      if (const bookmarks::BookmarkNode* found = Find(child.get())) {
        return found;
      }
    }
  }
  return nullptr;
}

class DesktopSaveLocationProvider
    : public power_bookmarks::SuggestedSaveLocationProvider {
 public:
  explicit DesktopSaveLocationProvider(bookmarks::BookmarkModel* model)
      : model_(model) {}
  DesktopSaveLocationProvider(const DesktopSaveLocationProvider&) = delete;
  DesktopSaveLocationProvider& operator=(const DesktopSaveLocationProvider&) =
      delete;
  ~DesktopSaveLocationProvider() override = default;

  // power_bookmarks::SuggestedSaveLocationProvider:
  const bookmarks::BookmarkNode* GetSuggestion(const GURL& url) override {
    return url.SchemeIsHTTPOrHTTPS() ? DesktopFolder(model_) : nullptr;
  }
  // Zero: if someone moves a new bookmark out of Desktop, the next one still
  // goes to Desktop (a non-zero backoff would silently send it elsewhere).
  base::TimeDelta GetBackoffTime() override { return base::TimeDelta(); }
  std::string GetFeatureNameForMetrics() override { return "MastDesktop"; }
  void OnSuggestionRejected() override {}

 private:
  raw_ptr<bookmarks::BookmarkModel> model_;
};

}  // namespace

const bookmarks::BookmarkNode* FindDesktopFolder(
    const bookmarks::BookmarkModel* model) {
  if (!model || !model->loaded()) {
    return nullptr;
  }
  return Find(model->root_node());
}

const bookmarks::BookmarkNode* DesktopFolder(bookmarks::BookmarkModel* model) {
  if (const bookmarks::BookmarkNode* found = FindDesktopFolder(model)) {
    if (found->GetTitle() == kOldFolderTitle) {
      model->SetTitle(found, kFolderTitle,
                      bookmarks::metrics::BookmarkEditSource::kOther);
    }
    return found;
  }
  if (!model || !model->loaded()) {
    return nullptr;
  }
  bookmarks::BookmarkNode::MetaInfoMap meta;
  meta[kDesktopMetaKey] = "1";
  const bookmarks::BookmarkNode* other = model->other_node();
  return model->AddFolder(other, other->children().size(), kFolderTitle, &meta);
}

std::unique_ptr<power_bookmarks::SuggestedSaveLocationProvider>
CreateDesktopSaveLocationProvider(bookmarks::BookmarkModel* model) {
  return std::make_unique<DesktopSaveLocationProvider>(model);
}

}  // namespace mast
