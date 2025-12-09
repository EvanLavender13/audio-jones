#include "raygui.h"

#include "ui_panel_spectrum.h"
#include "ui_color.h"
#include "ui_widgets.h"
#include <math.h>

Rectangle UIDrawSpectrumPanel(UILayout* l, PanelState* state, SpectrumConfig* config)
{
    const int rowH = 20;

    UILayoutGroupBegin(l, NULL);

    // Enable toggle
    UILayoutRow(l, rowH);
    GuiCheckBox(UILayoutSlot(l, 1.0f), "Enabled", &config->enabled);

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    // Geometry
    DrawLabeledSlider(l, "Radius", &config->innerRadius, 0.05f, 0.4f);
    DrawLabeledSlider(l, "Height", &config->barHeight, 0.1f, 0.5f);
    DrawLabeledSlider(l, "Width", &config->barWidth, 0.3f, 1.0f);

    // Dynamics
    DrawLabeledSlider(l, "Smooth", &config->smoothing, 0.0f, 0.95f);
    DrawLabeledSlider(l, "Min dB", &config->minDb, 0.0f, 40.0f);
    DrawLabeledSlider(l, "Max dB", &config->maxDb, 20.0f, 60.0f);

    // Rotation (uses TextFormat for dynamic label)
    UILayoutRow(l, rowH);
    DrawText(TextFormat("Rot %.3f", config->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, 0.38f);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &config->rotationSpeed, -0.05f, 0.05f);

    DrawLabeledSlider(l, "Offset", &config->rotationOffset, 0.0f, 2.0f * PI);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }

    Rectangle dropdownRect = UIDrawColorControls(l, state, &config->color,
                                                  &state->spectrumHueRangeDragging);

    UILayoutGroupEnd(l);
    return dropdownRect;
}
