#ifndef UI_PANEL_PRESET_H
#define UI_PANEL_PRESET_H

#include "config/app_configs.h"

// Preset panel state (opaque)
typedef struct PresetPanelState PresetPanelState;

PresetPanelState* PresetPanelInit(void);
void PresetPanelUninit(PresetPanelState* state);

// Draw preset panel, returns bottom Y position
int UIDrawPresetPanel(PresetPanelState* state, int startY, AppConfigs* configs);

#endif // UI_PANEL_PRESET_H
