#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "config/app_configs.h"

// UI state (opaque)
typedef struct UIState UIState;

UIState* UIStateInit(void);
void UIStateUninit(UIState* state);

// Returns bottom Y position after panel
int UIDrawWaveformPanel(UIState* state, int startY, AppConfigs* configs);

#endif // UI_MAIN_H
