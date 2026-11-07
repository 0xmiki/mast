// Copyright 2026 The Mast Authors

#ifndef MAST_UI_COLOR_MAST_SITE_COLOR_H_
#define MAST_UI_COLOR_MAST_SITE_COLOR_H_

#include <algorithm>
#include <cmath>
#include <optional>

#include "third_party/skia/include/core/SkColor.h"

// Customize's "Match site" (Tab bar on websites): the one-row tab row takes the
// website's own colour. Everything in the row is built from that one colour the
// way Mast's palettes are (mast_palette.h): the same hue and chroma, stepped in
// lightness. The steps are taken in OKLCH, a perceptual space (equal steps look
// equal), so they read the same on any colour. Colours that wouldn't work fall
// back to the palette: too loud, too close to ember (which means blocked), or
// with no text colour that reads on it.
namespace mast::site_color {

struct Oklch {
  float l = 0;  // lightness, 0..1
  float c = 0;  // chroma (0 is grey; brand colours reach ~0.25)
  float h = 0;  // hue, degrees
};

inline float ToLinear(float c) {
  return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

inline float FromLinear(float c) {
  c = std::clamp(c, 0.0f, 1.0f);
  return c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1 / 2.4f) - 0.055f;
}

// OKLab (https://bottosson.github.io/posts/oklab/).
inline void ToOklab(SkColor color, float& l, float& a, float& b) {
  const float r = ToLinear(SkColorGetR(color) / 255.0f);
  const float g = ToLinear(SkColorGetG(color) / 255.0f);
  const float bl = ToLinear(SkColorGetB(color) / 255.0f);
  const float lc = std::cbrt(0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * bl);
  const float mc = std::cbrt(0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * bl);
  const float sc = std::cbrt(0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * bl);
  l = 0.2104542553f * lc + 0.7936177850f * mc - 0.0040720468f * sc;
  a = 1.9779984951f * lc - 2.4285922050f * mc + 0.4505937099f * sc;
  b = 0.0259040371f * lc + 0.7827717662f * mc - 0.8086757660f * sc;
}

inline SkColor FromOklab(float l, float a, float b, SkAlpha alpha = 0xFF) {
  const float lc = l + 0.3963377774f * a + 0.2158037573f * b;
  const float mc = l - 0.1055613458f * a - 0.0638541728f * b;
  const float sc = l - 0.0894841775f * a - 1.2914855480f * b;
  const float L = lc * lc * lc, M = mc * mc * mc, S = sc * sc * sc;
  const float r = 4.0767416621f * L - 3.3077115913f * M + 0.2309699292f * S;
  const float g = -1.2684380046f * L + 2.6097574011f * M - 0.3413193965f * S;
  const float bl = -0.0041960863f * L - 0.7034186147f * M + 1.7076147010f * S;
  auto u8 = [](float v) {
    return static_cast<U8CPU>(std::lround(FromLinear(v) * 255.0f));
  };
  return SkColorSetARGB(alpha, u8(r), u8(g), u8(bl));
}

inline Oklch ToOklch(SkColor color) {
  float l, a, b;
  ToOklab(color, l, a, b);
  Oklch o;
  o.l = l;
  o.c = std::hypot(a, b);
  o.h = std::fmod(std::atan2(b, a) * 180.0f / 3.14159265f + 360.0f, 360.0f);
  return o;
}

inline SkColor FromOklch(float l, float c, float h) {
  const float rad = h * 3.14159265f / 180.0f;
  return FromOklab(std::clamp(l, 0.0f, 1.0f), c * std::cos(rad),
                   c * std::sin(rad));
}

// How far apart two colours look (OKLab distance; ~0.02 is just noticeable).
inline float Distance(SkColor x, SkColor y) {
  float l1, a1, b1, l2, a2, b2;
  ToOklab(x, l1, a1, b1);
  ToOklab(y, l2, a2, b2);
  return std::sqrt((l1 - l2) * (l1 - l2) + (a1 - a2) * (a1 - a2) +
                   (b1 - b2) * (b1 - b2));
}

// WCAG contrast ratio, 1..21.
inline float Contrast(SkColor x, SkColor y) {
  auto lum = [](SkColor c) {
    return 0.2126f * ToLinear(SkColorGetR(c) / 255.0f) +
           0.7152f * ToLinear(SkColorGetG(c) / 255.0f) +
           0.0722f * ToLinear(SkColorGetB(c) / 255.0f);
  };
  const float a = lum(x), b = lum(y);
  return (std::max(a, b) + 0.05f) / (std::min(a, b) + 0.05f);
}

inline constexpr SkColor kDarkText = SkColorSetRGB(0x16, 0x16, 0x16);
inline constexpr SkColor kLightText = SkColorSetRGB(0xF2, 0xF2, 0xF2);
// Ember, the blocked colour (mast_palette.h's signal): a row near it would
// read as "blocked".
inline constexpr SkColor kEmber = SkColorSetRGB(0xFF, 0x8F, 0x63);

// The tab row built from a site's colour.
struct Row {
  bool light = false;  // a light colour: dark text and icons
  SkColor bar = 0;     // the row itself: the site's colour
  SkColor hover = 0;   // an inactive tab under the pointer
  SkColor pill = 0;    // the active tab
  SkColor field = 0;   // the address field
  SkColor field_hover = 0;
  SkColor raised = 0;  // light rows: a hovered suggestion
  SkColor raised2 = 0; // light rows: the selected suggestion
  SkColor border = 0;  // the address field's focus hairline
  SkColor text = 0;
  SkColor text2 = 0;   // inactive tabs, icons
  SkColor faint = 0;   // disabled icons
};

// The row for `site`, or nothing if the palette should be used instead.
inline std::optional<Row> RowFor(SkColor site) {
  site = SkColorSetA(site, 0xFF);
  const Oklch o = ToOklch(site);
  const float ember_h = ToOklch(kEmber).h;
  const float dh = std::min(std::abs(o.h - ember_h), 360 - std::abs(o.h - ember_h));
  const float on_dark = Contrast(kDarkText, site);
  const float on_light = Contrast(kLightText, site);
  if (o.c > 0.14f ||                  // too loud for a whole row
      (o.c > 0.06f && dh < 30) ||     // reads as ember, i.e. blocked
      std::max(on_dark, on_light) < 4.5f) {  // no text colour reads on it
    return std::nullopt;
  }
  Row row;
  row.light = on_dark > on_light;
  // Raised layers step away from the row: darker on a light row, lighter on a
  // dark one, as the palettes do.
  const float s = row.light ? -1 : 1;
  auto step = [&](float dl) { return FromOklch(o.l + s * dl, o.c, o.h); };
  row.bar = site;
  row.hover = step(0.035f);
  row.pill = step(0.075f);
  row.field = step(0.045f);
  row.field_hover = step(0.065f);
  row.raised = step(0.07f);
  row.raised2 = step(0.1f);
  row.border = step(0.16f);
  row.text = row.light ? kDarkText : kLightText;
  row.text2 = SkColorSetA(row.text, 0xBF);
  row.faint = SkColorSetA(row.text, 0x66);
  return row;
}

}  // namespace mast::site_color

#endif  // MAST_UI_COLOR_MAST_SITE_COLOR_H_
