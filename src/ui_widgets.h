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

#endif // UI_WIDGETS_H
