// Copyright 2026 The Mast Authors

#include "mast/ui/color/mast_ref_color_mixer.h"

#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"

namespace mast {

void AddMastRefColorMixer(ui::ColorProvider* provider,
                          const ui::ColorProviderKey& key) {
  // A user-picked colour or grayscale theme still works as in Chromium.
  using Source = ui::ColorProviderKey::UserColorSource;
  if (key.user_color.has_value() && key.user_color_source != Source::kBaseline) {
    return;
  }
  ui::ColorMixer& mixer = provider->AddMixer();

  // Ember. Dark mode uses tone 80 as "primary" and tone 20 as text on it.
  mixer[ui::kColorRefPrimary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[ui::kColorRefPrimary10] = {SkColorSetRGB(0x16, 0x08, 0x03)};
  mixer[ui::kColorRefPrimary20] = {SkColorSetRGB(0x1A, 0x0B, 0x05)};
  mixer[ui::kColorRefPrimary25] = {SkColorSetRGB(0x3A, 0x14, 0x07)};
  mixer[ui::kColorRefPrimary30] = {SkColorSetRGB(0x5C, 0x22, 0x10)};
  mixer[ui::kColorRefPrimary40] = {SkColorSetRGB(0xAB, 0x2F, 0x0C)};
  mixer[ui::kColorRefPrimary50] = {SkColorSetRGB(0xC9, 0x3A, 0x12)};
  mixer[ui::kColorRefPrimary60] = {SkColorSetRGB(0xE2, 0x47, 0x1C)};
  mixer[ui::kColorRefPrimary70] = {SkColorSetRGB(0xF2, 0x58, 0x2B)};
  mixer[ui::kColorRefPrimary80] = {SkColorSetRGB(0xFF, 0x6A, 0x3D)};
  mixer[ui::kColorRefPrimary90] = {SkColorSetRGB(0xFF, 0xD3, 0xC4)};
  mixer[ui::kColorRefPrimary95] = {SkColorSetRGB(0xFF, 0xE9, 0xE1)};
  mixer[ui::kColorRefPrimary99] = {SkColorSetRGB(0xFF, 0xF8, 0xF6)};
  mixer[ui::kColorRefPrimary100] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};

  // Tertiary follows the accent.
  mixer[ui::kColorRefTertiary0] = {ui::kColorRefPrimary0};
  mixer[ui::kColorRefTertiary10] = {ui::kColorRefPrimary10};
  mixer[ui::kColorRefTertiary20] = {ui::kColorRefPrimary20};
  mixer[ui::kColorRefTertiary30] = {ui::kColorRefPrimary30};
  mixer[ui::kColorRefTertiary40] = {ui::kColorRefPrimary40};
  mixer[ui::kColorRefTertiary50] = {ui::kColorRefPrimary50};
  mixer[ui::kColorRefTertiary60] = {ui::kColorRefPrimary60};
  mixer[ui::kColorRefTertiary70] = {ui::kColorRefPrimary70};
  mixer[ui::kColorRefTertiary80] = {ui::kColorRefPrimary80};
  mixer[ui::kColorRefTertiary90] = {ui::kColorRefPrimary90};
  mixer[ui::kColorRefTertiary95] = {ui::kColorRefPrimary95};
  mixer[ui::kColorRefTertiary99] = {ui::kColorRefPrimary99};
  mixer[ui::kColorRefTertiary100] = {ui::kColorRefPrimary100};

  // Secondary: the demo's cool greys (Chromium's is blue). Selected rows,
  // tonal buttons and chips read as raised grey, with ember reserved for accents.
  mixer[ui::kColorRefSecondary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[ui::kColorRefSecondary10] = {SkColorSetRGB(0x14, 0x15, 0x18)};
  mixer[ui::kColorRefSecondary12] = {SkColorSetRGB(0x1B, 0x1C, 0x20)};
  mixer[ui::kColorRefSecondary15] = {SkColorSetRGB(0x22, 0x24, 0x2A)};
  mixer[ui::kColorRefSecondary20] = {SkColorSetRGB(0x27, 0x29, 0x30)};
  mixer[ui::kColorRefSecondary25] = {SkColorSetRGB(0x2A, 0x2C, 0x33)};
  mixer[ui::kColorRefSecondary30] = {SkColorSetRGB(0x33, 0x35, 0x3D)};
  mixer[ui::kColorRefSecondary35] = {SkColorSetRGB(0x3B, 0x3D, 0x46)};
  mixer[ui::kColorRefSecondary40] = {SkColorSetRGB(0x4E, 0x50, 0x59)};
  mixer[ui::kColorRefSecondary50] = {SkColorSetRGB(0x6B, 0x6E, 0x78)};
  mixer[ui::kColorRefSecondary60] = {SkColorSetRGB(0x7F, 0x81, 0x8C)};
  mixer[ui::kColorRefSecondary70] = {SkColorSetRGB(0x9A, 0x9C, 0xA5)};
  mixer[ui::kColorRefSecondary80] = {SkColorSetRGB(0xC2, 0xC3, 0xCC)};
  mixer[ui::kColorRefSecondary90] = {SkColorSetRGB(0xDA, 0xDB, 0xE5)};
  mixer[ui::kColorRefSecondary95] = {SkColorSetRGB(0xEC, 0xEC, 0xEF)};
  mixer[ui::kColorRefSecondary99] = {SkColorSetRGB(0xF9, 0xFA, 0xFC)};
  mixer[ui::kColorRefSecondary100] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
}

}  // namespace mast
