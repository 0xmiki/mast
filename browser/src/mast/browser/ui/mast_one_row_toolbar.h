// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_ONE_ROW_TOOLBAR_H_
#define MAST_BROWSER_UI_MAST_ONE_ROW_TOOLBAR_H_

class BrowserWindowInterface;

namespace mast {

// Called once a browser window and its toolbar exist (from
// EmbedderBrowserWindowFeatures::InitPostWindowConstruction). In the one-row
// window the toolbar reads: back, address (fixed width), forward, menu.
void OnBrowserWindowReady(BrowserWindowInterface* browser);

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_ONE_ROW_TOOLBAR_H_
