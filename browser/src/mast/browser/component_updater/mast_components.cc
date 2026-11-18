// Copyright 2026 The Mast Authors

#include "mast/browser/component_updater/mast_components.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/update_client_errors.h"
#include "mast/common/mast_widevine_state.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#endif

namespace component_updater {

namespace {

// Widevine's CRX id (widevine_cdm_component_installer.cc).
constexpr char kWidevineId[] = "oimompecagnajdejgnnjijobebaeigek";

// Registering is asynchronous (the installer first looks for a copy on disk),
// so a download asked for right away can find nothing registered yet.
constexpr int kTries = 10;
constexpr base::TimeDelta kRetryDelay = base::Seconds(1);

bool registered = false;

}  // namespace

// Downloads Widevine on the user's word. A friend of OnDemandUpdater
// (component_updater_service.h, mast-hooks.patch), the way other one-off
// downloads (speech, translation) are.
class MastWidevineUpdater {
 public:
  static void Install(ComponentUpdateService* cus,
                      base::OnceCallback<void(bool)> done) {
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
    if (!registered) {
      registered = true;
      RegisterWidevineCdmComponent(cus);
    }
    Try(cus, kTries, std::move(done));
#else
    std::move(done).Run(false);
#endif
  }

 private:
  static void Try(ComponentUpdateService* cus,
                  int tries,
                  base::OnceCallback<void(bool)> done) {
    cus->GetOnDemandUpdater().OnDemandUpdate(
        kWidevineId, OnDemandUpdater::Priority::FOREGROUND,
        base::BindOnce(&MastWidevineUpdater::Done, base::Unretained(cus),
                       tries, std::move(done)));
  }

  static void Done(ComponentUpdateService* cus,
                   int tries,
                   base::OnceCallback<void(bool)> done,
                   update_client::Error error) {
    // Not registered yet, or the service is busy: try again shortly.
    if ((error == update_client::Error::INVALID_ARGUMENT ||
         error == update_client::Error::UPDATE_IN_PROGRESS) &&
        tries > 1) {
      base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&MastWidevineUpdater::Try, base::Unretained(cus),
                         tries - 1, std::move(done)),
          kRetryDelay);
      return;
    }
    std::move(done).Run(error == update_client::Error::NONE);
  }
};

void MastRegisterComponents(ComponentUpdateService* cus) {
  PrefService* local_state = g_browser_process->local_state();
  if (local_state &&
      local_state->GetInteger(mast::kWidevinePref) == mast::kWidevineOn) {
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
    // Said yes before: keep it updated (the updater checks every few hours).
    registered = true;
    RegisterWidevineCdmComponent(cus);
#endif
  }
  // The service lives as long as the browser process.
  mast::SetWidevineInstaller(
      base::BindRepeating(&MastWidevineUpdater::Install, base::Unretained(cus)));
}

}  // namespace component_updater
