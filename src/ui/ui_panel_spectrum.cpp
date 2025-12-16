#include "raygui.h"

#include "ui_panel_spectrum.h"
#include "ui_color.h"
#include "ui_widgets.h"
#include <math.h>

Rectangle UIDrawSpectrumPanel(UILayout* l, PanelState* state, SpectrumConfig* config)
{
    UILayoutGroupBegin(l, NULL);

    // Enable toggle
    UILayoutRow(l, 20);
    GuiToggle(UILayoutSlot(l, 1.0f), "Enabled", &config->enabled);

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    // Geometry
    DrawLabeledSlider(l, "Radius", &config->innerRadius, 0.05f, 0.4f, NULL);
    DrawLabeledSlider(l, "Height", &config->barHeight, 0.1f, 0.5f, NULL);
    DrawLabeledSlider(l, "Width", &config->barWidth, 0.3f, 1.0f, NULL);

    // Dynamics
    DrawLabeledSlider(l, "Smooth", &config->smoothing, 0.0f, 0.95f, NULL);
    DrawLabeledSlider(l, "Min dB", &config->minDb, 0.0f, 40.0f, "dB");
    DrawLabeledSlider(l, "Max dB", &config->maxDb, 20.0f, 60.0f, "dB");

    // Rotation
    DrawLabeledSlider(l, "Rotation", &config->rotationSpeed, -0.05f, 0.05f, "rad");
    DrawLabeledSlider(l, "Offset", &config->rotationOffset, 0.0f, 2.0f * PI, "rad");

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }

    Rectangle dropdownRect = UIDrawColorControls(l, state, &config->color,
                                                  &state->spectrumHueRangeDragging);

    UILayoutGroupEnd(l);
    return dropdownRect;
}
