#include "raygui.h"

#include "ui_panel_audio.h"
#include <stddef.h>

Rectangle UIDrawAudioPanel(UILayout* l, PanelState* state, AudioConfig* /* audio */)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    UILayoutRow(l, rowH);
    DrawText("Channel", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    Rectangle dropdownRect = UILayoutSlot(l, 1.0f);

    UILayoutGroupEnd(l);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }

    return dropdownRect;
}
