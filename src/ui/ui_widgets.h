#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "raylib.h"
#include "ui_layout.h"
#include <stdbool.h>

// Custom raygui-style widgets

// Labeled float slider. Draws label text, reserves label space, draws slider.
// Uses standard row height (20) and label ratio (0.38).
void DrawLabeledSlider(UILayout* l, const char* label, float* value, float min, float max);

// Labeled int slider. Handles int-float-int conversion for raygui compatibility.
// Uses standard row height (20) and label ratio (0.38).
void DrawIntSlider(UILayout* l, const char* label, int* value, int min, int max);

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

// Accordion section header toggle.
// Draws a collapsible section header with [+]/[-] prefix.
// Parameters:
//   l        - Layout context
//   title    - Section title (displayed after expand indicator)
//   expanded - Expansion state, toggled on click
// Returns: current expanded state (for conditional content drawing)
bool DrawAccordionHeader(UILayout* l, const char* title, bool* expanded);

#endif // UI_WIDGETS_H
