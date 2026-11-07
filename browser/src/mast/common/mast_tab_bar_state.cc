// Copyright 2026 The Mast Authors

#include "mast/common/mast_tab_bar_state.h"

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"

namespace mast {

namespace {

base::flat_map<const void*, uint32_t>& Colors() {
  static base::NoDestructor<base::flat_map<const void*, uint32_t>> colors;
  return *colors;
}

}  // namespace

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
