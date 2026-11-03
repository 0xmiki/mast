// Copyright 2026 The Mast Authors

#ifndef MAST_UI_COLOR_MAST_PALETTE_H_
#define MAST_UI_COLOR_MAST_PALETTE_H_

#include "third_party/skia/include/core/SkColor.h"

// The Mast design tokens (Rams proposal: warm neutral greys, one signal colour).
// One source for native UI; WebUI pages use the same values in mast_pages.cc.
namespace mast::palette {

inline constexpr SkColor kBg = SkColorSetRGB(0x14, 0x14, 0x14);       // frame, tab strip
inline constexpr SkColor kChrome = SkColorSetRGB(0x1C, 0x1C, 0x1C);   // toolbar, active tab, pages
inline constexpr SkColor kChrome2 = SkColorSetRGB(0x26, 0x26, 0x26);  // fields (omnibox)
inline constexpr SkColor kRaised = SkColorSetRGB(0x2C, 0x2C, 0x2C);   // hover
inline constexpr SkColor kRaised2 = SkColorSetRGB(0x30, 0x30, 0x30);  // pressed, selected
inline constexpr SkColor kText = SkColorSetRGB(0xE9, 0xE7, 0xE3);
inline constexpr SkColor kMuted = SkColorSetRGB(0xA2, 0x9F, 0x99);
inline constexpr SkColor kFaint = SkColorSetRGB(0x6F, 0x6C, 0x67);
inline constexpr SkColor kLine = SkColorSetRGB(0x2C, 0x2C, 0x2C);
// The signal colour: only for blocking and the state of being blocked.
inline constexpr SkColor kEmber = SkColorSetRGB(0xFF, 0x6A, 0x3D);
inline constexpr SkColor kEmber2 = SkColorSetRGB(0xFF, 0x8F, 0x5E);
inline constexpr SkColor kOnEmber = SkColorSetRGB(0x1A, 0x0D, 0x07);

}  // namespace mast::palette

#endif  // MAST_UI_COLOR_MAST_PALETTE_H_
