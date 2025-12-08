#include "ui_common.h"

bool AnyDropdownOpen(PanelState* state)
{
    return state->colorModeDropdownOpen ||
           state->spectrumColorModeDropdownOpen ||
           state->channelModeDropdownOpen;
}
