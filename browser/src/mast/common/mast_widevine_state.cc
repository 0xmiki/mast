// Copyright 2026 The Mast Authors

#include "mast/common/mast_widevine_state.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace mast {

namespace {

using RequestedList = base::RepeatingCallbackList<void(const void*)>;
RequestedList& Requested() {
  static base::NoDestructor<RequestedList> list;
  return *list;
}

WidevineInstaller& Installer() {
  static base::NoDestructor<WidevineInstaller> installer;
  return *installer;
}

}  // namespace

void ProtectedMediaRequested(const void* web_contents) {
  Requested().Notify(web_contents);
}

base::CallbackListSubscription OnProtectedMediaRequested(
    base::RepeatingCallback<void(const void*)> callback) {
  return Requested().Add(std::move(callback));
}

void SetWidevineInstaller(WidevineInstaller installer) {
  Installer() = std::move(installer);
}

bool InstallWidevine(base::OnceCallback<void(bool)> done) {
  if (!Installer()) {
    return false;
  }
  Installer().Run(std::move(done));
  return true;
}

}  // namespace mast
