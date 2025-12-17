#include "raygui.h"

#include "ui_panel_effects.h"
#include "ui_color.h"
#include "ui_widgets.h"
#include <stddef.h>

static const char* LFO_WAVEFORM_OPTIONS = "Sine;Triangle;Saw;Square;S&&H";

EffectsPanelDropdowns UIDrawEffectsPanel(UILayout* l, PanelState* state, EffectConfig* effects)
{
    const int rowH = 20;
    EffectsPanelDropdowns dropdowns = {{0, 0, 0, 0}};

    // Disable controls if any dropdown is open
    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_DISABLED);
    }

    UILayoutGroupBegin(l, NULL);

    DrawIntSlider(l, "Blur", &effects->baseBlurScale, 0, 4, NULL);
    DrawLabeledSlider(l, "Half-life", &effects->halfLife, 0.1f, 2.0f, "s");
    DrawIntSlider(l, "Bloom", &effects->beatBlurScale, 0, 5, NULL);
    DrawIntSlider(l, "Chroma", &effects->chromaticMaxOffset, 0, 50, "px");
    DrawLabeledSlider(l, "Zoom", &effects->feedbackZoom, 0.9f, 1.0f, NULL);
    DrawLabeledSlider(l, "Rotation", &effects->feedbackRotation, 0.0f, 0.02f, "rad");
    DrawLabeledSlider(l, "Desat", &effects->feedbackDesaturate, 0.0f, 0.2f, NULL);
    DrawIntSlider(l, "Kaleido", &effects->kaleidoSegments, 1, 12, NULL);

    // Voronoi section
    DrawLabeledSlider(l, "Voronoi", &effects->voronoiIntensity, 0.0f, 1.0f, NULL);
    if (effects->voronoiIntensity > 0.0f) {
        DrawLabeledSlider(l, "V.Scale", &effects->voronoiScale, 5.0f, 50.0f, NULL);
        DrawLabeledSlider(l, "V.Speed", &effects->voronoiSpeed, 0.1f, 2.0f, NULL);
        DrawLabeledSlider(l, "V.Edge", &effects->voronoiEdgeWidth, 0.01f, 0.1f, NULL);
    }

    // Physarum section
    UILayoutRow(l, rowH);
    GuiToggle(UILayoutSlot(l, 1.0f), "Physarum", &effects->physarum.enabled);

    if (effects->physarum.enabled) {
        DrawIntSlider(l, "P.Agents", &effects->physarum.agentCount, 10000, 1000000, NULL);
        DrawLabeledSlider(l, "P.Sensor", &effects->physarum.sensorDistance, 1.0f, 100.0f, "px");
        DrawLabeledSlider(l, "P.Angle", &effects->physarum.sensorAngle, 0.0f, 6.28f, "rad");
        DrawLabeledSlider(l, "P.Turn", &effects->physarum.turningAngle, 0.0f, 6.28f, "rad");
        DrawLabeledSlider(l, "P.Step", &effects->physarum.stepSize, 0.1f, 100.0f, "px");
        DrawLabeledSlider(l, "P.Deposit", &effects->physarum.depositAmount, 0.01f, 5.0f, NULL);
        DrawLabeledSlider(l, "P.Decay", &effects->physarum.decayHalfLife, 0.1f, 5.0f, "s");
        DrawIntSlider(l, "P.Diffuse", &effects->physarum.diffusionScale, 0, 4, NULL);
        DrawLabeledSlider(l, "P.Boost", &effects->physarum.boostIntensity, 0.0f, 2.0f, NULL);
        DrawLabeledSlider(l, "P.Sense", &effects->physarum.accumSenseBlend, 0.0f, 1.0f, NULL);
        dropdowns.physarumColor = UIDrawColorControls(l, state, &effects->physarum.color,
                                                       &state->physarumHueDragging);
        UILayoutRow(l, rowH);
        GuiToggle(UILayoutSlot(l, 1.0f), "P.Debug", &effects->physarum.debugOverlay);
    }

    // LFO section
    UILayoutRow(l, rowH);
    GuiToggle(UILayoutSlot(l, 1.0f), "Rotation LFO", &effects->rotationLFO.enabled);

    if (effects->rotationLFO.enabled) {
        DrawLabeledSlider(l, "Rate", &effects->rotationLFO.rate, 0.01f, 1.0f, "Hz");

        // Waveform dropdown
        UILayoutRow(l, rowH);
        DrawText("Wave", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, 0.38f);
        dropdowns.lfoWaveform = UILayoutSlot(l, 1.0f);
    }

    UILayoutGroupEnd(l);

    if (AnyDropdownOpen(state)) {
        GuiSetState(STATE_NORMAL);
    }

    return dropdowns;
}
