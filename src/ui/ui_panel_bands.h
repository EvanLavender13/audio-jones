#ifndef UI_PANEL_BANDS_H
#define UI_PANEL_BANDS_H

#include "ui_common.h"
#include "ui_layout.h"
#include "config/band_config.h"
#include "analysis/bands.h"

// Renders band energy meters with per-band sensitivity sliders.
// Shows real-time bass/mid/treb levels with color coding.
void UIDrawBandsPanel(UILayout* l, PanelState* state, BandEnergies* bands,
                      BandConfig* config);

#endif // UI_PANEL_BANDS_H
