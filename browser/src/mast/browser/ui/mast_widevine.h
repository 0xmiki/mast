// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_WIDEVINE_H_
#define MAST_BROWSER_UI_MAST_WIDEVINE_H_

#include "base/callback_list.h"
#include "base/no_destructor.h"

namespace mast {

// Widevine on request, the way Brave does it. Mast doesn't download it on its
// own. The first time a page needs it (Spotify, Netflix: they use protected
// media, EME), that tab asks: download it from Google, or don't ask again.
// Once it's downloaded, the tab offers a restart: on Linux, Chromium only
// loads Widevine at startup (the hint file, component_widevine_cdm_hint_file_
// linux.h). The answer is kept in Local State (kWidevinePref); a yes also
// keeps it updated from then on (mast_components.cc).
class MastWidevine {
 public:
  // Once, from the first normal window (mast_one_row_toolbar.cc).
  static void Start();

  MastWidevine(const MastWidevine&) = delete;
  MastWidevine& operator=(const MastWidevine&) = delete;

 private:
  friend class base::NoDestructor<MastWidevine>;

  MastWidevine();
  ~MastWidevine();

  void OnProtectedMedia(const void* web_contents);

  base::CallbackListSubscription subscription_;
};

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_WIDEVINE_H_
