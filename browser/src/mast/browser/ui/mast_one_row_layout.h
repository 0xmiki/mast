// Copyright 2026 The Mast Authors

#ifndef MAST_BROWSER_UI_MAST_ONE_ROW_LAYOUT_H_
#define MAST_BROWSER_UI_MAST_ONE_ROW_LAYOUT_H_

#include <algorithm>

#include "mast/common/mast_features.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

// Geometry for the one-row window, used by Chromium's tabbed browser layout
// (browser_view_tabbed_layout_impl.cc). Header-only: that file's target only
// needs //mast/common.
namespace mast {

// The row is the tab strip's height: the frame paints behind exactly that, and
// the toolbar centres its buttons in it (a taller row left an unpainted band).
inline int OneRowHeight(const views::View* tab_strip,
                        const views::View* toolbar) {
  const int tabs = tab_strip->GetPreferredSize().height();
  return tabs > 0 ? tabs : toolbar->GetPreferredSize().height();
}

// The toolbar's slice of the row, measured from the bottom-left of the row
// (so y is negative). The tab strip step computes it, while the window's
// caption areas are still known; the top container step uses it right after,
// in the same layout pass, once the layout has moved below the row.
inline gfx::Rect& PendingToolbarSlice() {
  static gfx::Rect slice;
  return slice;
}

// The tabs take the row up to where the toolbar starts. `row` is the tab
// strip's usual bounds (between the window's caption areas); `client_x` is the
// left of the area being laid out.
inline void FitTabStripInRow(gfx::Rect& row,
                             int client_x,
                             const views::View* tab_strip,
                             const views::View* toolbar) {
  const int height = OneRowHeight(tab_strip, toolbar);
  const int toolbar_width =
      std::min(toolbar->GetPreferredSize().width(), row.width());
  row.set_width(row.width() - toolbar_width);
  row.set_height(height);
  PendingToolbarSlice() =
      gfx::Rect(row.right() - client_x, -height, toolbar_width, height);
}

// The top container is exactly the toolbar's slice, so its background never
// covers the tabs. `client_area` is the area below the row, in the top
// container's parent coordinates. Empty if the tab strip step didn't run.
inline gfx::Rect TakeOneRowToolbarSlice(const gfx::Rect& client_area) {
  gfx::Rect slice = PendingToolbarSlice();
  PendingToolbarSlice() = gfx::Rect();
  if (!slice.IsEmpty()) {
    slice.Offset(client_area.OffsetFromOrigin());
  }
  return slice;
}

// The suggestion list opens wider than the narrow address bar: same right
// edge, growing left, never past the left of `window` (screen coordinates).
inline void WidenOmniboxPopup(gfx::Rect& bounds, const gfx::Rect& window) {
  if (!IsOneRowWindow() ||
      !base::FeatureList::IsEnabled(kMastWideOmniboxPopup) ||
      bounds.width() >= kWideOmniboxPopupWidth) {
    return;
  }
  const int right = bounds.right();
  const int left = std::max(window.x(), right - kWideOmniboxPopupWidth);
  bounds.set_x(left);
  bounds.set_width(right - left);
}

}  // namespace mast

#endif  // MAST_BROWSER_UI_MAST_ONE_ROW_LAYOUT_H_
