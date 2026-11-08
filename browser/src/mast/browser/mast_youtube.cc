// Copyright 2026 The Mast Authors

#include "mast/browser/mast_youtube.h"

#include <optional>
#include <string>

#include "base/functional/callback_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mast/browser/mast_blocks.h"
#include "url/gurl.h"

namespace mast {

namespace {

// resources/youtube_music.js, embedded by remote/embed-pages.py.
constexpr char kYouTubeMusicJs[] =
#include "mast/browser/resources/youtube_music_js.inc"
    ;

}  // namespace

MastYouTube::MastYouTube(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      content::WebContentsUserData<MastYouTube>(*contents) {}

MastYouTube::~MastYouTube() = default;

void MastYouTube::DidFinishNavigation(content::NavigationHandle* navigation) {
  // As early as possible on a new YouTube page, so its feed is filtered before
  // it paints. In-page navigations keep the script already running.
  if (navigation->IsInPrimaryMainFrame() && navigation->HasCommitted() &&
      !navigation->IsSameDocument() && !navigation->IsErrorPage()) {
    MaybeInject(navigation->GetRenderFrameHost());
  }
}

void MastYouTube::DOMContentLoaded(content::RenderFrameHost* render_frame_host) {
  // A second chance, in case the first ran before the page could take it.
  if (render_frame_host->IsInPrimaryMainFrame()) {
    MaybeInject(render_frame_host);
  }
}

void MastYouTube::MaybeInject(content::RenderFrameHost* frame) {
  if (!frame) {
    return;
  }
  const GURL& url = frame->GetLastCommittedURL();
  if (!url.SchemeIsHTTPOrHTTPS() || !IsYouTubeHost(url.host())) {
    return;
  }
  const std::optional<Block> music = MastBlocks::Get().YouTubeMusicOnly();
  if (!music) {
    return;
  }
  std::string script(kYouTubeMusicJs);
  base::ReplaceSubstringsAfterOffset(
      &script, 0, "{{UNTIL}}",
      music->until
          ? base::NumberToString(music->until->InMillisecondsSinceUnixEpoch())
          : std::string("null"));
  frame->ExecuteJavaScriptInIsolatedWorld(base::UTF8ToUTF16(script),
                                          base::NullCallback(),
                                          ISOLATED_WORLD_ID_CHROME_INTERNAL);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(MastYouTube);

}  // namespace mast
