// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_COLOR_MAST_CHROME_COLOR_MIXER_H_
#define MAST_BROWSER_UI_COLOR_MAST_CHROME_COLOR_MIXER_H_

namespace ui {
class ColorProvider;
struct ColorProviderKey;
}  // namespace ui

namespace mast {

// Pins the browser's surfaces to the Mast design (frame, toolbar, tabs, omnibox,
// menus, dialogs). A postprocessing mixer, so it wins regardless of the order of
// upstream mixers. Skipped for custom themes and high contrast.
void AddMastChromeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderKey& key);

}  // namespace mast

#endif  // MAST_BROWSER_UI_COLOR_MAST_CHROME_COLOR_MIXER_H_
