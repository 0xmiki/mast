// Copyright 2026 The Mast Authors

#include "mast/common/mast_tab_bar_state.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace mast {

namespace {

base::flat_map<const void*, uint32_t>& Colors() {
  static base::NoDestructor<base::flat_map<const void*, uint32_t>> colors;
  return *colors;
}

base::flat_map<const void*, float>& Opacities() {
  static base::NoDestructor<base::flat_map<const void*, float>> opacities;
  return *opacities;
}

base::RepeatingCallbackList<void(const void*)>& Revealed() {
  static base::NoDestructor<base::RepeatingCallbackList<void(const void*)>>
      revealed;
  return *revealed;
}

using TypedList =
    base::RepeatingCallbackList<void(const void*, const std::u16string&)>;
TypedList& Typed() {
  static base::NoDestructor<TypedList> typed;
  return *typed;
}

}  // namespace

void NewTabTyped(const void* web_contents, const std::u16string& text) {
  Typed().Notify(web_contents, text);
}

base::CallbackListSubscription OnNewTabTyped(
    base::RepeatingCallback<void(const void*, const std::u16string&)>
        callback) {
  return Typed().Add(std::move(callback));
}

float WallpaperOpacity(const void* browser_view) {
  const auto it = Opacities().find(browser_view);
  return it == Opacities().end() ? 1.0f : it->second;
}

void SetWallpaperOpacity(const void* browser_view,
                         std::optional<float> opacity) {
  if (opacity && *opacity < 1.0f) {
    Opacities()[browser_view] = *opacity < 0.0f ? 0.0f : *opacity;
  } else {
    Opacities().erase(browser_view);
  }
}

void WallpaperRevealed(const void* web_contents) {
  Revealed().Notify(web_contents);
}

base::CallbackListSubscription OnWallpaperRevealed(
    base::RepeatingCallback<void(const void*)> callback) {
  return Revealed().Add(std::move(callback));
}

std::optional<uint32_t> TabBarColor(const void* browser_view) {
  const auto it = Colors().find(browser_view);
  if (it == Colors().end()) {
    return std::nullopt;
  }
  return it->second;
}

void SetTabBarColor(const void* browser_view, std::optional<uint32_t> color) {
  if (color) {
    Colors()[browser_view] = *color;
  } else {
    Colors().erase(browser_view);
  }
}

bool TabBarColorInUse(uint32_t color) {
  for (const auto& [view, c] : Colors()) {
    if (c == color) {
      return true;
    }
  }
  return false;
}

}  // namespace mast
