#include "raygui.h"

#include "ui_panel_spectrum.h"
#include "ui_color.h"
#include <math.h>

Rectangle UIDrawSpectrumPanel(UILayout* l, PanelState* state, SpectrumConfig* config)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    UILayoutGroupBegin(l, NULL);

    // Enable toggle
    UILayoutRow(l, rowH);
    GuiCheckBox(UILayoutSlot(l, 1.0f), "Enabled", &config->enabled);

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    // Geometry sliders
    UILayoutRow(l, rowH);
    DrawText("Radius", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->innerRadius, 0.05f, 0.4f);

    UILayoutRow(l, rowH);
    DrawText("Height", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->barHeight, 0.1f, 0.5f);

    UILayoutRow(l, rowH);
    DrawText("Width", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->barWidth, 0.3f, 1.0f);

    // Dynamics
    UILayoutRow(l, rowH);
    DrawText("Smooth", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->smoothing, 0.0f, 0.95f);

    UILayoutRow(l, rowH);
    DrawText("Min dB", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->minDb, 0.0f, 40.0f);

    UILayoutRow(l, rowH);
    DrawText("Max dB", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->maxDb, 20.0f, 60.0f);

    // Rotation
    UILayoutRow(l, rowH);
    DrawText(TextFormat("Rot %.3f", config->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->rotationSpeed, -0.05f, 0.05f);

    UILayoutRow(l, rowH);
    DrawText("Offset", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->rotationOffset, 0.0f, 2.0f * PI);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }

    // Color controls (reuse extracted function)
    Rectangle dropdownRect = UIDrawColorControls(l, state, &config->color,
                                                  &state->spectrumHueRangeDragging);

    UILayoutGroupEnd(l);
    return dropdownRect;
}
