#include "ui_common.h"
#include "raygui.h"

bool AnyDropdownOpen(PanelState* state)
{
    return state->colorModeDropdownOpen ||
           state->spectrumColorModeDropdownOpen ||
           state->channelModeDropdownOpen ||
           state->lfoWaveformDropdownOpen ||
           state->physarumColorModeDropdownOpen;
}

bool DrawDeferredDropdown(Rectangle rect, bool sectionVisible, const char* options,
                          int* value, bool* openState)
{
    if (!sectionVisible || rect.width <= 0) {
        return false;
    }

    if (GuiDropdownBox(rect, options, value, *openState) != 0) {
        *openState = !*openState;
    }
    return true;
}
