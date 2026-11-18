// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_COMPONENT_UPDATER_MAST_COMPONENTS_H_
#define MAST_BROWSER_COMPONENT_UPDATER_MAST_COMPONENTS_H_

namespace component_updater {

class ComponentUpdateService;

// Mast's component registration, in place of Chromium's
// (RegisterComponentsForUpdate, chrome/browser/component_updater/
// registration.cc; mast-hooks.patch). Google's update server is reached for
// one component only, Widevine, and only once the user said yes on a page that
// needed it (mast/common/mast_widevine_state.h). Nothing else is registered:
// ungoogled-chromium's dead update address kept those from ever downloading
// anyway, and with the real address back they would all call Google.
// Compiled into //chrome/browser/component_updater:impl.
void MastRegisterComponents(ComponentUpdateService* cus);

}  // namespace component_updater

#endif  // MAST_BROWSER_COMPONENT_UPDATER_MAST_COMPONENTS_H_
