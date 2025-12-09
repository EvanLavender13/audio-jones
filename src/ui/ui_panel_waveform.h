#ifndef UI_PANEL_WAVEFORM_H
#define UI_PANEL_WAVEFORM_H

#include "raylib.h"
#include "ui_common.h"
#include "../ui_layout.h"
#include "../waveform.h"

// Waveform panel state (list scroll position)
typedef struct WaveformPanelState {
    int scrollIndex;
} WaveformPanelState;

WaveformPanelState* WaveformPanelInit(void);
void WaveformPanelUninit(WaveformPanelState* state);

// Renders waveform list (New button + list view).
void UIDrawWaveformListGroup(UILayout* l, WaveformPanelState* wfState, WaveformConfig* waveforms,
                             int* waveformCount, int* selectedWaveform);

// Renders selected waveform settings (radius, height, thickness, etc. + color controls).
// Returns dropdown rect for deferred z-order drawing.
Rectangle UIDrawWaveformSettingsGroup(UILayout* l, PanelState* state, WaveformConfig* sel, int selectedIndex);

#endif // UI_PANEL_WAVEFORM_H
