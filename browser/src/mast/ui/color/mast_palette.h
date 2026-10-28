// Copyright 2026 The Mast Authors

#ifndef MAST_UI_COLOR_MAST_PALETTE_H_
#define MAST_UI_COLOR_MAST_PALETTE_H_

#include "third_party/skia/include/core/SkColor.h"

// The Mast design tokens (mirrors the demo's app.css). One source for native UI;
// WebUI pages get the same values through the colour pipeline and mast_pages.cc.
namespace mast::palette {

inline constexpr SkColor kBg = SkColorSetRGB(0x0C, 0x0D, 0x0F);        // frame
inline constexpr SkColor kChrome = SkColorSetRGB(0x14, 0x15, 0x18);    // toolbar
inline constexpr SkColor kChrome2 = SkColorSetRGB(0x1B, 0x1C, 0x20);   // omnibox
inline constexpr SkColor kRaised = SkColorSetRGB(0x22, 0x24, 0x2A);    // hover
inline constexpr SkColor kRaised2 = SkColorSetRGB(0x2A, 0x2C, 0x33);   // pressed
inline constexpr SkColor kText = SkColorSetRGB(0xEC, 0xEC, 0xEF);
inline constexpr SkColor kMuted = SkColorSetRGB(0x9A, 0x9C, 0xA5);
inline constexpr SkColor kFaint = SkColorSetRGB(0x6B, 0x6E, 0x78);
inline constexpr SkColor kLine = SkColorSetARGB(0x12, 0xFF, 0xFF, 0xFF);
inline constexpr SkColor kEmber = SkColorSetRGB(0xFF, 0x6A, 0x3D);
inline constexpr SkColor kEmber2 = SkColorSetRGB(0xFF, 0x8F, 0x5E);
inline constexpr SkColor kOnEmber = SkColorSetRGB(0x1A, 0x0B, 0x05);

}  // namespace mast::palette

#endif  // MAST_UI_COLOR_MAST_PALETTE_H_
