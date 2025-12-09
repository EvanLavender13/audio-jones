#ifndef UI_PANEL_PRESET_H
#define UI_PANEL_PRESET_H

#include "render/waveform.h"
#include "audio/audio_config.h"
#include "config/effect_config.h"
#include "config/spectrum_bars_config.h"

// Preset panel state (opaque)
typedef struct PresetPanelState PresetPanelState;

PresetPanelState* PresetPanelInit(void);
void PresetPanelUninit(PresetPanelState* state);

// Draw preset panel, returns bottom Y position
int UIDrawPresetPanel(PresetPanelState* state, int startY,
                      WaveformConfig* waveforms, int* waveformCount,
                      EffectConfig* effects, AudioConfig* audio,
                      SpectrumConfig* spectrum);

#endif // UI_PANEL_PRESET_H
