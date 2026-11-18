// Copyright 2026 The Mast Authors

#ifndef MAST_COMMON_MAST_WIDEVINE_STATE_H_
#define MAST_COMMON_MAST_WIDEVINE_STATE_H_

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"

namespace mast {

// Widevine, Google's player for protected audio and video (Spotify, Netflix),
// downloaded from Google only once the user says yes on a page that needs it
// (mast/browser/ui/mast_widevine.h). Base-only, so content (where a page asks
// for protected media) and the component updater (which downloads it) can
// both reach it.

// Local State, an int: kWidevineAsk until the user answers.
inline constexpr char kWidevinePref[] = "mast.widevine";
enum WidevineChoice { kWidevineAsk = 0, kWidevineOn = 1, kWidevineOff = 2 };

// A page in `web_contents` (a content::WebContents) started using protected
// media (EME): content/browser/media/key_system_support_impl.cc. Called on the
// UI thread, while the page is alive.
void ProtectedMediaRequested(const void* web_contents);
base::CallbackListSubscription OnProtectedMediaRequested(
    base::RepeatingCallback<void(const void*)> callback);

// Downloads Widevine now (and keeps it updated); `done` runs with whether it
// worked. Set by the component updater (mast_components.cc) at startup.
using WidevineInstaller =
    base::RepeatingCallback<void(base::OnceCallback<void(bool)>)>;
void SetWidevineInstaller(WidevineInstaller installer);
// False, and `done` never runs, if there's no installer.
bool InstallWidevine(base::OnceCallback<void(bool)> done);

}  // namespace mast

#endif  // MAST_COMMON_MAST_WIDEVINE_STATE_H_
