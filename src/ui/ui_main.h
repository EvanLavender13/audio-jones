#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "raylib.h"
#include "render/waveform.h"
#include "audio/audio_config.h"
#include "config/effect_config.h"
#include "config/spectrum_bars_config.h"
#include "analysis/beat.h"
#include "ui_layout.h"
#include <stdbool.h>

// UI state (opaque)
typedef struct UIState UIState;

UIState* UIStateInit(void);
void UIStateUninit(UIState* state);

// Returns bottom Y position after panel
int UIDrawWaveformPanel(UIState* state, int startY, WaveformConfig* waveforms,
                        int* waveformCount, int* selectedWaveform,
                        EffectConfig* effects, AudioConfig* audio,
                        SpectrumConfig* spectrum, BeatDetector* beat);

#endif // UI_MAIN_H
