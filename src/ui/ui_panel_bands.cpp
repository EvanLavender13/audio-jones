#include "raygui.h"

#include "ui_panel_bands.h"
#include "ui_widgets.h"
#include <stddef.h>

void UIDrawBandsPanel(UILayout* l, PanelState* state, BandEnergies* bands,
                      BandConfig* config)
{
    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    // Band meter widget
    UILayoutRow(l, 36);
    GuiBandMeter(UILayoutSlot(l, 1.0f), bands, config);

    // Per-band sensitivity sliders
    DrawLabeledSlider(l, "Bass", &config->bassSensitivity, 0.5f, 2.0f);
    DrawLabeledSlider(l, "Mid", &config->midSensitivity, 0.5f, 2.0f);
    DrawLabeledSlider(l, "Treb", &config->trebSensitivity, 0.5f, 2.0f);

    UILayoutGroupEnd(l);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }
}
