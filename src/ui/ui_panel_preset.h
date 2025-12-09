#ifndef UI_PANEL_PRESET_H
#define UI_PANEL_PRESET_H

#include "../waveform.h"
#include "../audio_config.h"
#include "../effects_config.h"
#include "../spectrum_config.h"

// Preset panel state (opaque)
typedef struct PresetPanelState PresetPanelState;

PresetPanelState* PresetPanelInit(void);
void PresetPanelUninit(PresetPanelState* state);

// Draw preset panel, returns bottom Y position
int UIDrawPresetPanel(PresetPanelState* state, int startY,
                      WaveformConfig* waveforms, int* waveformCount,
                      EffectsConfig* effects, AudioConfig* audio,
                      SpectrumConfig* spectrum);

#endif // UI_PANEL_PRESET_H
