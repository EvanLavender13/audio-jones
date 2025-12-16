#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdbool.h>
#include "raylib.h"

// Shared state for dropdown coordination across all panels.
// Only one dropdown can be open at a time; controls behind open dropdowns are disabled.
typedef struct PanelState {
    bool colorModeDropdownOpen;
    bool spectrumColorModeDropdownOpen;
    bool channelModeDropdownOpen;
    bool lfoWaveformDropdownOpen;
    bool physarumColorModeDropdownOpen;
    int waveformHueRangeDragging;  // 0=none, 1=left handle, 2=right handle
    int spectrumHueRangeDragging;  // 0=none, 1=left handle, 2=right handle
    int physarumHueDragging;       // 0=none, 1=left handle, 2=right handle
} PanelState;

// Returns true if any dropdown is currently expanded.
// Use to disable controls that would appear behind the dropdown.
bool AnyDropdownOpen(PanelState* state);

// Draw a deferred dropdown if section is visible and rect is valid.
// Toggles openState on click and updates value. Returns true if dropdown was drawn.
bool DrawDeferredDropdown(Rectangle rect, bool sectionVisible, const char* options,
                          int* value, bool* openState);

#endif // UI_COMMON_H
