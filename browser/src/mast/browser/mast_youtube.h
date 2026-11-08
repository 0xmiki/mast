// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_YOUTUBE_H_
#define MAST_BROWSER_MAST_YOUTUBE_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace mast {

// YouTube, music only (design C24; the block entry kYouTubeMusicKey). While
// it's in force, every YouTube page in this tab runs resources/youtube_music.js
// in Mast's own isolated world (where an extension's content script would run,
// but built in): it hides everything but music and sends other videos and
// Shorts to the blocked page, which the navigation throttle shows. Attached to
// every tab by the throttle.
class MastYouTube : public content::WebContentsObserver,
                    public content::WebContentsUserData<MastYouTube> {
 public:
  MastYouTube(const MastYouTube&) = delete;
  MastYouTube& operator=(const MastYouTube&) = delete;
  ~MastYouTube() override;

  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* navigation) override;
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;

 private:
  friend class content::WebContentsUserData<MastYouTube>;
  explicit MastYouTube(content::WebContents* contents);

  // Runs the script in the page if Music only is on and it's a YouTube page.
  // The script runs once per page (it checks), so calling twice is harmless.
  void MaybeInject(content::RenderFrameHost* frame);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace mast

#endif  // MAST_BROWSER_MAST_YOUTUBE_H_
