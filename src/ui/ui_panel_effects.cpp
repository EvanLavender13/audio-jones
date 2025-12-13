#include "raygui.h"

#include "ui_panel_effects.h"
#include "ui_widgets.h"
#include <stddef.h>

static const char* LFO_WAVEFORM_OPTIONS = "Sine;Triangle;Saw;Square;S&&H";

Rectangle UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectConfig* effects,
                              BeatDetector* beat)
{
    const int rowH = 20;
    Rectangle dropdownRect = {0, 0, 0, 0};

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    DrawIntSlider(l, "Blur", &effects->baseBlurScale, 0, 4);
    DrawLabeledSlider(l, "Half-life", &effects->halfLife, 0.1f, 2.0f);
    DrawIntSlider(l, "Bloom", &effects->beatBlurScale, 0, 5);
    DrawIntSlider(l, "Chroma", &effects->chromaticMaxOffset, 0, 50);
    DrawLabeledSlider(l, "Zoom", &effects->feedbackZoom, 0.9f, 1.0f);
    DrawLabeledSlider(l, "Rotation", &effects->feedbackRotation, 0.0f, 0.02f);
    DrawLabeledSlider(l, "Desat", &effects->feedbackDesaturate, 0.0f, 0.2f);

    // LFO section
    UILayoutRow(l, rowH);
    GuiCheckBox(UILayoutSlot(l, 1.0f), "Rotation LFO", &effects->rotationLFO.enabled);

    if (effects->rotationLFO.enabled) {
        DrawLabeledSlider(l, "Rate", &effects->rotationLFO.rate, 0.01f, 1.0f);

        // Waveform dropdown
        UILayoutRow(l, rowH);
        DrawText("Wave", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, 0.38f);
        dropdownRect = UILayoutSlot(l, 1.0f);
    }

    UILayoutRow(l, 40);
    GuiBeatGraph(UILayoutSlot(l, 1.0f), beat->graphHistory, BEAT_GRAPH_SIZE, beat->graphIndex);

    UILayoutGroupEnd(l);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }

    return dropdownRect;
}
