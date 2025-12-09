#include "raygui.h"

#include "ui_panel_waveform.h"
#include "ui_color.h"
#include "ui_widgets.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Preset colors for new waveforms
static const Color presetColors[] = {
    {255, 255, 255, 255},  // White
    {230, 41, 55, 255},    // Red
    {0, 228, 48, 255},     // Green
    {0, 121, 241, 255},    // Blue
    {253, 249, 0, 255},    // Yellow
    {255, 0, 255, 255},    // Magenta
    {255, 161, 0, 255},    // Orange
    {102, 191, 255, 255}   // Sky blue
};

WaveformPanelState* WaveformPanelInit(void)
{
    WaveformPanelState* state = (WaveformPanelState*)calloc(1, sizeof(WaveformPanelState));
    return state;
}

void WaveformPanelUninit(WaveformPanelState* state)
{
    free(state);
}

void UIDrawWaveformListGroup(UILayout* l, WaveformPanelState* wfState, WaveformConfig* waveforms,
                             int* waveformCount, int* selectedWaveform)
{
    const int rowH = 20;
    const int listHeight = 80;

    UILayoutGroupBegin(l, NULL);

    UILayoutRow(l, rowH);
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton(UILayoutSlot(l, 1.0f), "New") != 0) {
        waveforms[*waveformCount] = WaveformConfig{};
        waveforms[*waveformCount].color.solid = presetColors[*waveformCount % 8];
        *selectedWaveform = *waveformCount;
        (*waveformCount)++;
    }
    GuiSetState(STATE_NORMAL);

    UILayoutRow(l, listHeight);
    static char itemNames[MAX_WAVEFORMS][16];
    const char* listItems[MAX_WAVEFORMS];
    for (int i = 0; i < *waveformCount; i++) {
        (void)snprintf(itemNames[i], sizeof(itemNames[i]), "Waveform %d", i + 1);
        listItems[i] = itemNames[i];
    }
    int focus = -1;
    GuiListViewEx(UILayoutSlot(l, 1.0f), listItems, *waveformCount,
                  &wfState->scrollIndex, selectedWaveform, &focus);

    UILayoutGroupEnd(l);
}

Rectangle UIDrawWaveformSettingsGroup(UILayout* l, PanelState* state,
                                       WaveformConfig* sel, int selectedIndex)
{
    const int rowH = 20;

    UILayoutGroupBegin(l, TextFormat("Waveform %d", selectedIndex + 1));

    // Geometry
    DrawLabeledSlider(l, "Radius", &sel->radius, 0.05f, 0.45f);
    DrawLabeledSlider(l, "Height", &sel->amplitudeScale, 0.05f, 0.5f);
    DrawIntSlider(l, "Thickness", &sel->thickness, 1, 25);
    DrawLabeledSlider(l, "Smooth", &sel->smoothness, 0.0f, 100.0f);

    // Rotation (uses TextFormat for dynamic label)
    UILayoutRow(l, rowH);
    DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, 0.38f);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);

    DrawLabeledSlider(l, "Offset", &sel->rotationOffset, 0.0f, 2.0f * PI);

    Rectangle dropdownRect = UIDrawColorControls(l, state, &sel->color,
                                                  &state->waveformHueRangeDragging);

    UILayoutGroupEnd(l);
    return dropdownRect;
}
