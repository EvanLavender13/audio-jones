#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "waveform.h"
#include "beat.h"
#include "ui_layout.h"
#include <stdbool.h>

// UI state (opaque)
typedef struct UIState UIState;

UIState* UIStateInit(void);
void UIStateUninit(UIState* state);

// Panel layout - call at frame start, then panels auto-stack
void UIBeginPanels(UIState* state, int startY);

// Panels - call in any order, they stack vertically
void UIDrawWaveformPanel(UIState* state, WaveformConfig* waveforms,
                         int* waveformCount, int* selectedWaveform,
                         float* halfLife, float* baseBlurScale,
                         float* beatBlurScale, BeatDetector* beat);

void UIDrawPresetPanel(UIState* state, WaveformConfig* waveforms,
                       int* waveformCount, float* halfLife,
                       float* baseBlurScale, float* beatBlurScale);

#endif // UI_H
