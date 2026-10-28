// Copyright 2026 The Mast Authors

#ifndef MAST_UI_COLOR_MAST_REF_COLOR_MIXER_H_
#define MAST_UI_COLOR_MAST_REF_COLOR_MIXER_H_

namespace ui {
class ColorProvider;
struct ColorProviderKey;
}  // namespace ui

namespace mast {

// Replaces Chromium's baseline (blue) accent palette with Mast's ember, and its
// secondary palette with cool greys, so nothing derives a blue or Material You
// tint. Runs right after ui::AddRefColorMixer; neutrals come from Chromium's own
// "dark neutrals 26" set, which already matches the Mast design.
void AddMastRefColorMixer(ui::ColorProvider* provider,
                          const ui::ColorProviderKey& key);

}  // namespace mast

#endif  // MAST_UI_COLOR_MAST_REF_COLOR_MIXER_H_
