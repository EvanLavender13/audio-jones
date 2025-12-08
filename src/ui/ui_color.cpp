#include "raygui.h"

#include "ui_color.h"
#include "../ui_widgets.h"
#include <math.h>

Rectangle UIDrawColorControls(UILayout* l, PanelState* state, ColorConfig* color, int* hueRangeDragging)
{
    const int rowH = 20;
    const int colorPickerSize = 62;
    const float labelRatio = 0.38f;

    // Color mode dropdown (reserve space, return rect for deferred draw)
    UILayoutRow(l, rowH);
    DrawText("Mode", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    Rectangle dropdownRect = UILayoutSlot(l, 1.0f);

    // Disable controls behind dropdown when open
    bool anyDropdownOpen = AnyDropdownOpen(state);
    if (anyDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    if (color->mode == COLOR_MODE_SOLID) {
        UILayoutRow(l, colorPickerSize);
        DrawText("Color", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        const Rectangle colorSlot = UILayoutSlot(l, 1.0f);
        GuiColorPicker({colorSlot.x, colorSlot.y, colorSlot.width - 24, colorSlot.height}, NULL, &color->solid);

        UILayoutRow(l, rowH);
        DrawText("Alpha", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        float alpha = color->solid.a / 255.0f;
        GuiColorBarAlpha(UILayoutSlot(l, 1.0f), NULL, &alpha);
        color->solid.a = (unsigned char)(alpha * 255.0f);
    } else {
        // Rainbow controls
        UILayoutRow(l, rowH);
        DrawText("Hue", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        float hueEnd = fminf(color->rainbowHue + color->rainbowRange, 360.0f);
        if (!anyDropdownOpen) {
            GuiHueRangeSlider(UILayoutSlot(l, 1.0f), &color->rainbowHue, &hueEnd, hueRangeDragging);
            color->rainbowRange = hueEnd - color->rainbowHue;
        } else {
            int noDrag = 0;
            GuiHueRangeSlider(UILayoutSlot(l, 1.0f), &color->rainbowHue, &hueEnd, &noDrag);
        }

        UILayoutRow(l, rowH);
        DrawText("Sat", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &color->rainbowSat, 0.0f, 1.0f);

        UILayoutRow(l, rowH);
        DrawText("Bright", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &color->rainbowVal, 0.0f, 1.0f);
    }

    if (anyDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }

    return dropdownRect;
}
