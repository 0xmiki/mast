// Copyright 2026 The Mast Authors

#include "mast/browser/mast_prefs.h"

#include <utility>

#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace mast {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  // No tab-search button at the start of the tab strip. Ctrl+Shift+A still
  // opens tab search, and anyone who pins it again keeps it.
  registry->SetDefaultPrefValue(prefs::kTabSearchPinnedToTabstrip,
                                base::Value(false));

  base::ListValue widgets;
  widgets.Append("clock");
  widgets.Append("distractions");
  registry->RegisterListPref(kDesktopWidgetsPref, std::move(widgets));
}

}  // namespace mast
