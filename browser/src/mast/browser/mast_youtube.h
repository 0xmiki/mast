// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_MAST_YOUTUBE_H_
#define MAST_BROWSER_MAST_YOUTUBE_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace mast {

// YouTube, Music only (design C24, the block entry kYouTubeMusicKey) and
// Shorts (C32, kYouTubeShortsKey). While either is in force, every YouTube
// page in this tab runs resources/youtube_music.js in Mast's own isolated
// world (where an extension's content script would run, but built in). Shorts:
// it hides Shorts and sends them to the blocked page. Music only: that, and it
// hides everything but music and sends other videos to the blocked page too.
// The navigation throttle shows the page. Attached to every tab by the
// throttle.
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

  // Runs the script in the page if Music only or Shorts is on and it's a
  // YouTube page.
  // The script runs once per page (it checks), so calling twice is harmless.
  void MaybeInject(content::RenderFrameHost* frame);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace mast

#endif  // MAST_BROWSER_MAST_YOUTUBE_H_
