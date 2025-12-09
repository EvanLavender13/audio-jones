#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "raylib.h"
#include <stdbool.h>

// Custom raygui-style widgets

// Dual-handle hue range slider with rainbow gradient background.
// Allows selection of a hue range for rainbow color modes.
// Parameters:
//   bounds    - Widget bounds rectangle
//   hueStart  - Start of hue range (0-360), modified on drag
//   hueEnd    - End of hue range (0-360), modified on drag
//   dragging  - Drag state tracking (0=none, 1=left, 2=right)
// Returns: true if values changed
bool GuiHueRangeSlider(Rectangle bounds, float* hueStart, float* hueEnd, int* dragging);

// Beat intensity history graph.
// Displays a scrolling bar graph of recent beat intensities.
// Parameters:
//   bounds       - Widget bounds rectangle
//   history      - Circular buffer of intensity values (0.0-1.0)
//   historySize  - Size of the history buffer
//   currentIndex - Current write position in circular buffer
void GuiBeatGraph(Rectangle bounds, const float* history, int historySize, int currentIndex);

#endif // UI_WIDGETS_H
