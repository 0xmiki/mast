// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_BLOCK_BUBBLE_H_
#define MAST_BROWSER_UI_MAST_BLOCK_BUBBLE_H_

#include <memory>

#include "base/functional/callback_forward.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class View;
}  // namespace views

namespace mast {

// "Block this site", a row in the page-info bubble (the icon at the start of
// the address bar). Pressing it closes page info and opens the block bubble
// from the same icon: choose how long, then a 10-second undo.
// `close_page_info` closes the page-info bubble.
std::unique_ptr<views::View> CreateBlockSiteRow(
    content::WebContents* web_contents,
    base::RepeatingClosure close_page_info);

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_BLOCK_BUBBLE_H_
