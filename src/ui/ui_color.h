#ifndef UI_COLOR_H
#define UI_COLOR_H

#include "raylib.h"
#include "ui_common.h"
#include "ui_layout.h"
#include "render/color_config.h"

// Renders color mode controls (solid picker or rainbow sliders).
// Supports both waveform and spectrum panels through shared ColorConfig.
//
// For solid mode: renders color picker and alpha slider.
// For rainbow mode: renders hue range slider, saturation, and brightness sliders.
//
// Parameters:
//   hueRangeDragging - per-panel drag state for hue slider (0=none, 1=left, 2=right)
//
// Returns dropdown rect for deferred z-order drawing (dropdown must be drawn
// after all other controls so it appears on top).
Rectangle UIDrawColorControls(UILayout* l, PanelState* state, ColorConfig* color, int* hueRangeDragging);

#endif // UI_COLOR_H
