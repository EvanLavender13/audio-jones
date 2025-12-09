#ifndef UI_PANEL_SPECTRUM_H
#define UI_PANEL_SPECTRUM_H

#include "raylib.h"
#include "ui_common.h"
#include "../ui_layout.h"
#include "../spectrum_config.h"

// Renders spectrum bar controls (geometry, dynamics, rotation, color).
// Stateless - relies on PanelState for dropdown coordination only.
//
// Returns dropdown rect for deferred z-order drawing (dropdown must be drawn
// after all other controls so it appears on top).
Rectangle UIDrawSpectrumPanel(UILayout* l, PanelState* state, SpectrumConfig* config);

#endif // UI_PANEL_SPECTRUM_H
