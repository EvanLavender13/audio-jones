#include "raygui.h"

#include "ui_panel_effects.h"
#include "ui_widgets.h"
#include <stddef.h>

void UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectConfig* effects,
                        BeatDetector* beat)
{
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

    UILayoutRow(l, 40);
    GuiBeatGraph(UILayoutSlot(l, 1.0f), beat->graphHistory, BEAT_GRAPH_SIZE, beat->graphIndex);

    UILayoutGroupEnd(l);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }
}
