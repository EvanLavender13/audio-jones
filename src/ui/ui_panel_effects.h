#ifndef UI_PANEL_EFFECTS_H
#define UI_PANEL_EFFECTS_H

#include "ui_common.h"
#include "../ui_layout.h"
#include "../effects_config.h"
#include "../beat.h"

// Renders effects controls (blur, half-life, beat sensitivity, bloom, chroma, beat graph).
// Stateless - relies on PanelState for dropdown coordination only.
void UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectsConfig* effects,
                        BeatDetector* beat);

#endif // UI_PANEL_EFFECTS_H
