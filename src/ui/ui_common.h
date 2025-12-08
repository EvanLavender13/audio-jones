#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdbool.h>

// Shared state for dropdown coordination across all panels.
// Only one dropdown can be open at a time; controls behind open dropdowns are disabled.
typedef struct PanelState {
    bool colorModeDropdownOpen;
    bool spectrumColorModeDropdownOpen;
    bool channelModeDropdownOpen;
    int waveformHueRangeDragging;  // 0=none, 1=left handle, 2=right handle
    int spectrumHueRangeDragging;  // 0=none, 1=left handle, 2=right handle
} PanelState;

// Returns true if any dropdown is currently expanded.
// Use to disable controls that would appear behind the dropdown.
bool AnyDropdownOpen(PanelState* state);

#endif // UI_COMMON_H
