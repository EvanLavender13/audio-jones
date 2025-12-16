#ifndef UI_PANEL_EFFECTS_H
#define UI_PANEL_EFFECTS_H

#include "ui_common.h"
#include "ui_layout.h"
#include "config/effect_config.h"

typedef struct EffectsPanelDropdowns {
    Rectangle lfoWaveform;
    Rectangle physarumColor;
} EffectsPanelDropdowns;

// Renders effects controls (blur, half-life, bloom, chroma, feedback, physarum).
// Returns dropdown rects for deferred drawing.
// Stateless - relies on PanelState for dropdown coordination only.
EffectsPanelDropdowns UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectConfig* effects);

#endif // UI_PANEL_EFFECTS_H
