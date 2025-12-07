#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "waveform.h"
#include "audio_config.h"
#include "effects_config.h"
#include "beat.h"
#include "ui_layout.h"
#include <stdbool.h>

// UI state (opaque)
typedef struct UIState UIState;

UIState* UIStateInit(void);
void UIStateUninit(UIState* state);

// Returns bottom Y position after panel
int UIDrawWaveformPanel(UIState* state, WaveformConfig* waveforms,
                        int* waveformCount, int* selectedWaveform,
                        EffectsConfig* effects, AudioConfig* audio, BeatDetector* beat);

#endif // UI_H
