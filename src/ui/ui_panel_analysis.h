#ifndef UI_PANEL_ANALYSIS_H
#define UI_PANEL_ANALYSIS_H

#include "ui_layout.h"
#include "analysis/beat.h"
#include "analysis/bands.h"
#include "config/band_config.h"

// Renders analysis visualizations: beat detection graph and band energy meter.
void UIDrawAnalysisPanel(UILayout* l, BeatDetector* beat, BandEnergies* bands,
                         BandConfig* config);

#endif // UI_PANEL_ANALYSIS_H
