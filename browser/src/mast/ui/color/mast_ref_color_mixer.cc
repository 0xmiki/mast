// Copyright 2026 The Mast Authors

#include "mast/ui/color/mast_ref_color_mixer.h"

#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"
#include "ui/color/color_transform.h"

namespace mast {

void AddMastRefColorMixer(ui::ColorProvider* provider,
                          const ui::ColorProviderKey& key) {
  // A user-picked colour or grayscale theme still works as in Chromium.
  using Source = ui::ColorProviderKey::UserColorSource;
  if (key.user_color.has_value() && key.user_color_source != Source::kBaseline) {
    return;
  }
  ui::ColorMixer& mixer = provider->AddMixer();

  // Primary is neutral: Chromium's accent (default buttons, toggles, links,
  // focus) reads as warm off-white on graphite. The signal colour is reserved
  // for Mast's own blocking UI, which sets it explicitly. Dark mode uses tone
  // 80 as "primary" and tone 20 as text on it.
  mixer[ui::kColorRefPrimary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[ui::kColorRefPrimary10] = {SkColorSetRGB(0x14, 0x14, 0x14)};
  mixer[ui::kColorRefPrimary20] = {SkColorSetRGB(0x1C, 0x1C, 0x1C)};
  mixer[ui::kColorRefPrimary25] = {SkColorSetRGB(0x26, 0x26, 0x26)};
  mixer[ui::kColorRefPrimary30] = {SkColorSetRGB(0x3B, 0x3A, 0x38)};
  mixer[ui::kColorRefPrimary40] = {SkColorSetRGB(0x5F, 0x5C, 0x57)};
  mixer[ui::kColorRefPrimary50] = {SkColorSetRGB(0x7A, 0x77, 0x71)};
  mixer[ui::kColorRefPrimary60] = {SkColorSetRGB(0x96, 0x93, 0x8D)};
  mixer[ui::kColorRefPrimary70] = {SkColorSetRGB(0xB8, 0xB5, 0xAF)};
  mixer[ui::kColorRefPrimary80] = {SkColorSetRGB(0xE9, 0xE7, 0xE3)};
  mixer[ui::kColorRefPrimary90] = {SkColorSetRGB(0xF1, 0xEF, 0xEB)};
  mixer[ui::kColorRefPrimary95] = {SkColorSetRGB(0xF6, 0xF5, 0xF2)};
  mixer[ui::kColorRefPrimary99] = {SkColorSetRGB(0xFC, 0xFB, 0xFA)};
  mixer[ui::kColorRefPrimary100] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};

  // Tertiary follows primary.
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

  // Secondary: warm neutral greys (Chromium's is blue). Selected rows, tonal
  // buttons and chips read as raised grey; the signal colour stays for blocking.
  mixer[ui::kColorRefSecondary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[ui::kColorRefSecondary10] = {SkColorSetRGB(0x14, 0x14, 0x14)};
  mixer[ui::kColorRefSecondary12] = {SkColorSetRGB(0x1C, 0x1C, 0x1C)};
  mixer[ui::kColorRefSecondary15] = {SkColorSetRGB(0x22, 0x22, 0x21)};
  mixer[ui::kColorRefSecondary20] = {SkColorSetRGB(0x26, 0x26, 0x26)};
  mixer[ui::kColorRefSecondary25] = {SkColorSetRGB(0x2C, 0x2C, 0x2C)};
  mixer[ui::kColorRefSecondary30] = {SkColorSetRGB(0x33, 0x33, 0x32)};
  mixer[ui::kColorRefSecondary35] = {SkColorSetRGB(0x3B, 0x3A, 0x38)};
  mixer[ui::kColorRefSecondary40] = {SkColorSetRGB(0x4E, 0x4C, 0x49)};
  mixer[ui::kColorRefSecondary50] = {SkColorSetRGB(0x6F, 0x6C, 0x67)};
  mixer[ui::kColorRefSecondary60] = {SkColorSetRGB(0x86, 0x83, 0x7E)};
  mixer[ui::kColorRefSecondary70] = {SkColorSetRGB(0xA2, 0x9F, 0x99)};
  mixer[ui::kColorRefSecondary80] = {SkColorSetRGB(0xC6, 0xC3, 0xBE)};
  mixer[ui::kColorRefSecondary90] = {SkColorSetRGB(0xDD, 0xDA, 0xD5)};
  mixer[ui::kColorRefSecondary95] = {SkColorSetRGB(0xE9, 0xE7, 0xE3)};
  mixer[ui::kColorRefSecondary99] = {SkColorSetRGB(0xF8, 0xF7, 0xF5)};
  mixer[ui::kColorRefSecondary100] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
}

}  // namespace mast
