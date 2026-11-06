// Copyright 2026 The Mast Authors

#ifndef MAST_UI_COLOR_MAST_PALETTE_H_
#define MAST_UI_COLOR_MAST_PALETTE_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "third_party/skia/include/core/SkColor.h"

// The Mast design tokens: near-black neutrals and one signal colour, from the
// palette study in design/mast-browser.pen. "onyx" is the default; the new tab
// page's Customize panel picks another (pref mast.theme.palette). One source
// for native UI (the colour mixers) and Mast's WebUI pages (mast_pages.cc).
namespace mast::palette {

struct Palette {
  std::string_view id;
  std::string_view name;
  SkColor bg;       // the darkest layer (two-row frame, tab strip)
  SkColor chrome;   // toolbar, the one-row frame, pages
  SkColor chrome2;  // fields, the active tab, bubbles, menus
  SkColor hover;    // an inactive tab under the pointer (one row)
  SkColor raised;   // hover
  SkColor raised2;  // pressed, selected
  SkColor border;   // the address field's focus hairline
  SkColor text;
  SkColor muted;
  SkColor faint;
  SkColor line;
};

constexpr SkColor Rgb(uint32_t rgb) {
  return SkColorSetRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// All dark: Chromium's light mode would need its own mixers ("paper" in the
// design is for later).
inline constexpr std::array<Palette, 5> kPalettes = {{
    {"onyx", "Onyx", Rgb(0x000000), Rgb(0x070707), Rgb(0x141414),
     Rgb(0x0F0F0F), Rgb(0x191919), Rgb(0x1E1E1E), Rgb(0x2A2A2A),
     Rgb(0xEDEDED), Rgb(0x9A9A9A), Rgb(0x5C5C5C), Rgb(0x171717)},
    {"graphite", "Graphite", Rgb(0x141414), Rgb(0x1C1C1C), Rgb(0x262626),
     Rgb(0x212121), Rgb(0x2C2C2C), Rgb(0x303030), Rgb(0x3C3C3C),
     Rgb(0xE9E7E3), Rgb(0xA29F99), Rgb(0x6F6C67), Rgb(0x2C2C2C)},
    {"ink", "Ink", Rgb(0x0B0E12), Rgb(0x10141A), Rgb(0x19202A),
     Rgb(0x141A22), Rgb(0x1E2631), Rgb(0x232C38), Rgb(0x2F3640),
     Rgb(0xE8EDF3), Rgb(0x9AA6B5), Rgb(0x5F6B7A), Rgb(0x1C232D)},
    {"espresso", "Espresso", Rgb(0x14110D), Rgb(0x1A1611), Rgb(0x262019),
     Rgb(0x201B15), Rgb(0x2C251C), Rgb(0x312920), Rgb(0x3C362F),
     Rgb(0xF0E8DC), Rgb(0xB0A493), Rgb(0x776C5E), Rgb(0x2A231B)},
    {"moss", "Moss", Rgb(0x0F120F), Rgb(0x141814), Rgb(0x1E231D),
     Rgb(0x191D18), Rgb(0x232922), Rgb(0x282E27), Rgb(0x343933),
     Rgb(0xE6EADF), Rgb(0xA0A898), Rgb(0x6A7264), Rgb(0x222821)},
}};

// The palette in use, process-wide (one per browser, not per profile yet).
// An inline function's static is one object in the whole binary.
inline size_t& CurrentIndex() {
  static size_t index = 0;
  return index;
}

inline const Palette& Current() {
  return kPalettes[CurrentIndex()];
}

// Unknown ids fall back to onyx. Returns whether the palette changed; the
// caller then repaints (mast/browser/mast_theme.h).
inline bool SetCurrent(std::string_view id) {
  size_t index = 0;
  for (size_t i = 0; i < kPalettes.size(); ++i) {
    if (kPalettes[i].id == id) {
      index = i;
    }
  }
  const bool changed = index != CurrentIndex();
  CurrentIndex() = index;
  return changed;
}

// What the one-row tab row sits on: the palette, or Customize's image. On an
// image the row's pills turn translucent and its text and icons follow the
// image's tone (the user picks Dark or Light; the page guesses on upload).
// Process-wide like the palette.
enum class Backdrop { kPalette, kDarkImage, kLightImage };

inline Backdrop& CurrentBackdropRef() {
  static Backdrop backdrop = Backdrop::kPalette;
  return backdrop;
}

inline Backdrop CurrentBackdrop() {
  return CurrentBackdropRef();
}

// Returns whether it changed (the caller rebuilds colours, mast_theme.h).
inline bool SetBackdrop(Backdrop backdrop) {
  const bool changed = backdrop != CurrentBackdropRef();
  CurrentBackdropRef() = backdrop;
  return changed;
}

// The signal colour: only for blocking and the state of being blocked. The
// same in every palette.
// Tone 70: lighter and softer than a saturated orange, so it doesn't glow on
// near-black (the move Chrome makes for its own dark-mode primary).
inline constexpr SkColor kEmber = SkColorSetRGB(0xFF, 0x8F, 0x63);
inline constexpr SkColor kEmber2 = SkColorSetRGB(0xFF, 0xA6, 0x83);
inline constexpr SkColor kOnEmber = SkColorSetRGB(0x3D, 0x10, 0x00);

}  // namespace mast::palette

#endif  // MAST_UI_COLOR_MAST_PALETTE_H_
