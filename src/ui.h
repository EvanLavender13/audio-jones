#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "waveform.h"
#include <stdbool.h>

// Layout helper - eliminates manual coordinate math
typedef struct {
    int x, y;           // Container origin
    int width;          // Container width
    int padding;        // Inner padding
    int spacing;        // Between rows
    int rowHeight;      // Current row height
    int slotX;          // Current X within row (for multi-column)
} UILayout;

UILayout UILayoutBegin(int x, int y, int width, int padding, int spacing);
void UILayoutRow(UILayout* l, int height);
Rectangle UILayoutSlot(UILayout* l, float widthRatio);
int UILayoutEnd(UILayout* l);

// UI state (opaque)
typedef struct UIState UIState;

UIState* UIStateInit(void);
void UIStateUninit(UIState* state);

// Panel layout - call at frame start, then panels auto-stack
void UIBeginPanels(UIState* state, int startY);

// Panels - call in any order, they stack vertically
void UIDrawWaveformPanel(UIState* state, WaveformConfig* waveforms,
                         int* waveformCount, int* selectedWaveform,
                         float* halfLife);

void UIDrawPresetPanel(UIState* state, WaveformConfig* waveforms,
                       int* waveformCount, float* halfLife);

#endif // UI_H
