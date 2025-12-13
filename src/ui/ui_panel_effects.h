#ifndef UI_PANEL_EFFECTS_H
#define UI_PANEL_EFFECTS_H

#include "ui_common.h"
#include "ui_layout.h"
#include "config/effect_config.h"
#include "analysis/beat.h"

// Renders effects controls (blur, half-life, beat sensitivity, bloom, chroma, beat graph).
// Returns the LFO waveform dropdown rect for deferred drawing.
// Stateless - relies on PanelState for dropdown coordination only.
Rectangle UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectConfig* effects,
                              BeatDetector* beat);

#endif // UI_PANEL_EFFECTS_H
