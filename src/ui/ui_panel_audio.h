#ifndef UI_PANEL_AUDIO_H
#define UI_PANEL_AUDIO_H

#include "raylib.h"
#include "ui_common.h"
#include "ui_layout.h"
#include "../audio_config.h"

// Renders audio channel mode dropdown.
// Stateless - relies on PanelState for dropdown coordination only.
//
// Returns dropdown rect for deferred z-order drawing (dropdown must be drawn
// after all other controls so it appears on top).
Rectangle UIDrawAudioPanel(UILayout* l, PanelState* state, AudioConfig* audio);

#endif // UI_PANEL_AUDIO_H
