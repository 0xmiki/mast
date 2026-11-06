// Copyright 2026 The Mast Authors

#ifndef MAST_UI_COLOR_MAST_PALETTE_H_
#define MAST_UI_COLOR_MAST_PALETTE_H_

#include "third_party/skia/include/core/SkColor.h"

// The Mast design tokens: "onyx" (near-black neutrals, one signal colour),
// chosen in the palette study in design/mast-browser.pen.
// One source for native UI; WebUI pages use the same values in mast_pages.cc.
namespace mast::palette {

inline constexpr SkColor kBg = SkColorSetRGB(0x00, 0x00, 0x00);       // frame, tab strip
inline constexpr SkColor kChrome = SkColorSetRGB(0x07, 0x07, 0x07);   // toolbar, pages
inline constexpr SkColor kChrome2 = SkColorSetRGB(0x14, 0x14, 0x14);  // fields, active tab, bubbles
inline constexpr SkColor kRaised = SkColorSetRGB(0x19, 0x19, 0x19);   // hover
inline constexpr SkColor kRaised2 = SkColorSetRGB(0x1E, 0x1E, 0x1E);  // pressed, selected
inline constexpr SkColor kText = SkColorSetRGB(0xED, 0xED, 0xED);
inline constexpr SkColor kMuted = SkColorSetRGB(0x9A, 0x9A, 0x9A);
inline constexpr SkColor kFaint = SkColorSetRGB(0x5C, 0x5C, 0x5C);
inline constexpr SkColor kLine = SkColorSetRGB(0x17, 0x17, 0x17);
// The signal colour: only for blocking and the state of being blocked.
inline constexpr SkColor kEmber = SkColorSetRGB(0xFF, 0x6A, 0x3D);
inline constexpr SkColor kEmber2 = SkColorSetRGB(0xFF, 0x8F, 0x5E);
inline constexpr SkColor kOnEmber = SkColorSetRGB(0x1A, 0x0D, 0x07);

}  // namespace mast::palette

#endif  // MAST_UI_COLOR_MAST_PALETTE_H_
