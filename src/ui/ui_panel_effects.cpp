#include "raygui.h"

#include "ui_panel_effects.h"
#include "ui_widgets.h"
#include <math.h>

void UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectConfig* effects,
                        BeatDetector* beat)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    // Blur scale (int, 0-4 pixels)
    UILayoutRow(l, rowH);
    DrawText("Blur", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float blurFloat = (float)effects->baseBlurScale;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &blurFloat, 0.0f, 4.0f);
    effects->baseBlurScale = lroundf(blurFloat);

    UILayoutRow(l, rowH);
    DrawText("Half-life", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &effects->halfLife, 0.1f, 2.0f);

    // Beat sensitivity slider (threshold = mean + N * stddev)
    UILayoutRow(l, rowH);
    DrawText("Sens", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &effects->beatSensitivity, 1.0f, 3.0f);

    // Beat blur scale (int, 0-5 pixels)
    UILayoutRow(l, rowH);
    DrawText("Bloom", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float bloomFloat = (float)effects->beatBlurScale;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &bloomFloat, 0.0f, 5.0f);
    effects->beatBlurScale = lroundf(bloomFloat);

    UILayoutRow(l, rowH);
    DrawText("Chroma", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    float chromaFloat = (float)effects->chromaticMaxOffset;
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &chromaFloat, 0.0f, 50.0f);
    effects->chromaticMaxOffset = lroundf(chromaFloat);

    UILayoutRow(l, 40);
    GuiBeatGraph(UILayoutSlot(l, 1.0f), beat->graphHistory, BEAT_GRAPH_SIZE, beat->graphIndex);

    UILayoutGroupEnd(l);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }
}
