#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui.h"
#include "ui_widgets.h"
#include "preset.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// UI State

struct UIState {
    // Panel layout
    int panelY;

    // Waveform panel state
    int waveformScrollIndex;
    bool colorModeDropdownOpen;
    int hueRangeDragging;  // 0=none, 1=left handle, 2=right handle

    // Preset panel state
    char presetFiles[MAX_PRESET_FILES][PRESET_PATH_MAX];
    int presetFileCount;
    int selectedPreset;
    int presetScrollIndex;
    int prevSelectedPreset;
    char presetName[PRESET_NAME_MAX];
    bool presetNameEditMode;
};

void UIBeginPanels(UIState* state, int startY)
{
    state->panelY = startY;
}

UIState* UIStateInit(void)
{
    UIState* state = (UIState*)calloc(1, sizeof(UIState));
    if (!state) {
        return NULL;
    }

    state->selectedPreset = -1;
    state->prevSelectedPreset = -1;
    strncpy(state->presetName, "Default", PRESET_NAME_MAX);
    state->presetFileCount = PresetListFiles("presets", state->presetFiles, MAX_PRESET_FILES);

    return state;
}

void UIStateUninit(UIState* state)
{
    free(state);
}

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

static void DrawWaveformListGroup(UILayout* l, UIState* state, WaveformConfig* waveforms,
                                   int* waveformCount, int* selectedWaveform)
{
    const int rowH = 20;
    const int listHeight = 80;

    UILayoutGroupBegin(l, "Waveforms");

    UILayoutRow(l, rowH);
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton(UILayoutSlot(l, 1.0f), "New")) {
        waveforms[*waveformCount] = WaveformConfig{};
        waveforms[*waveformCount].color = presetColors[*waveformCount % 8];
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
                  &state->waveformScrollIndex, selectedWaveform, &focus);

    UILayoutGroupEnd(l);
}

static Rectangle DrawWaveformSettingsGroup(UILayout* l, UIState* state,
                                            WaveformConfig* sel, int selectedIndex)
{
    const int rowH = 20;
    const int colorPickerSize = 62;
    const float labelRatio = 0.38f;

    UILayoutGroupBegin(l, TextFormat("Waveform %d", selectedIndex + 1));

    UILayoutRow(l, rowH);
    DrawText("Radius", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->radius, 0.05f, 0.45f);

    UILayoutRow(l, rowH);
    DrawText("Height", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->amplitudeScale, 0.05f, 0.5f);

    UILayoutRow(l, rowH);
    DrawText("Thickness", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->thickness, 1.0f, 10.0f);

    UILayoutRow(l, rowH);
    DrawText("Smooth", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->smoothness, 0.0f, 50.0f);

    UILayoutRow(l, rowH);
    DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);

    UILayoutRow(l, rowH);
    DrawText("Offset", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rotationOffset, 0.0f, 2.0f * PI);

    // Color mode - reserve space, draw dropdown last for z-order
    UILayoutRow(l, rowH);
    DrawText("Mode", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    Rectangle dropdownRect = UILayoutSlot(l, 1.0f);

    // Disable controls behind dropdown when open
    if (state->colorModeDropdownOpen) {
        GuiSetState(STATE_DISABLED);
    }

    if (sel->colorMode == COLOR_MODE_SOLID) {
        UILayoutRow(l, colorPickerSize);
        DrawText("Color", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        Rectangle colorSlot = UILayoutSlot(l, 1.0f);
        GuiColorPicker({colorSlot.x, colorSlot.y, colorSlot.width - 24, colorSlot.height}, NULL, &sel->color);

        UILayoutRow(l, rowH);
        DrawText("Alpha", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        float alpha = sel->color.a / 255.0f;
        GuiColorBarAlpha(UILayoutSlot(l, 1.0f), NULL, &alpha);
        sel->color.a = (unsigned char)(alpha * 255.0f);
    } else {
        // Hue range slider (convert between hue+range and start/end)
        UILayoutRow(l, rowH);
        DrawText("Hue", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        float hueEnd = fminf(sel->rainbowHue + sel->rainbowRange, 360.0f);
        if (!state->colorModeDropdownOpen) {
            GuiHueRangeSlider(UILayoutSlot(l, 1.0f), &sel->rainbowHue, &hueEnd, &state->hueRangeDragging);
            sel->rainbowRange = hueEnd - sel->rainbowHue;
        } else {
            // Just draw, no interaction
            int noDrag = 0;
            GuiHueRangeSlider(UILayoutSlot(l, 1.0f), &sel->rainbowHue, &hueEnd, &noDrag);
        }

        UILayoutRow(l, rowH);
        DrawText("Sat", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rainbowSat, 0.0f, 1.0f);

        UILayoutRow(l, rowH);
        DrawText("Bright", l->x + l->padding, l->y + 4, 10, GRAY);
        (void)UILayoutSlot(l, labelRatio);
        GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &sel->rainbowVal, 0.0f, 1.0f);
    }

    if (state->colorModeDropdownOpen) {
        GuiSetState(STATE_NORMAL);
    }

    UILayoutGroupEnd(l);
    return dropdownRect;
}

static void DrawEffectsGroup(UILayout* l, float* halfLife, BeatDetector* beat)
{
    const int rowH = 20;
    const float labelRatio = 0.38f;

    UILayoutGroupBegin(l, "Effects");

    UILayoutRow(l, rowH);
    DrawText("Half-life", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, halfLife, 0.1f, 2.0f);

    UILayoutRow(l, rowH);
    DrawText("Beat", l->x + l->padding, l->y + 4, 10, GRAY);
    (void)UILayoutSlot(l, labelRatio);
    GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &beat->sensitivity, 1.0f, 3.0f);

    UILayoutRow(l, 40);
    GuiBeatGraph(UILayoutSlot(l, 1.0f), beat->graphHistory, BEAT_GRAPH_SIZE, beat->graphIndex);

    UILayoutGroupEnd(l);
}

void UIDrawWaveformPanel(UIState* state, WaveformConfig* waveforms,
                         int* waveformCount, int* selectedWaveform,
                         float* halfLife, BeatDetector* beat)
{
    UILayout l = UILayoutBegin(10, state->panelY, 180, 8, 4);

    DrawWaveformListGroup(&l, state, waveforms, waveformCount, selectedWaveform);

    WaveformConfig* sel = &waveforms[*selectedWaveform];
    Rectangle dropdownRect = DrawWaveformSettingsGroup(&l, state, sel, *selectedWaveform);

    DrawEffectsGroup(&l, halfLife, beat);

    // Draw dropdown last so it appears on top when open
    int mode = (int)sel->colorMode;
    if (GuiDropdownBox(dropdownRect, "Solid;Rainbow", &mode, state->colorModeDropdownOpen)) {
        state->colorModeDropdownOpen = !state->colorModeDropdownOpen;
    }
    sel->colorMode = (ColorMode)mode;

    state->panelY = l.y;
}

void UIDrawPresetPanel(UIState* state, WaveformConfig* waveforms,
                       int* waveformCount, float* halfLife)
{
    const int rowH = 20;
    const int listHeight = 48;
    const float labelRatio = 0.25f;

    UILayout l = UILayoutBegin(10, state->panelY, 180, 8, 4);

    UILayoutGroupBegin(&l, "Presets");

    // Name input
    UILayoutRow(&l, rowH);
    DrawText("Name", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    if (GuiTextBox(UILayoutSlot(&l, 1.0f), state->presetName, PRESET_NAME_MAX, state->presetNameEditMode)) {
        state->presetNameEditMode = !state->presetNameEditMode;
    }

    // Save button
    UILayoutRow(&l, rowH);
    if (GuiButton(UILayoutSlot(&l, 1.0f), "Save")) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s.json", state->presetName);
        Preset p;
        strncpy(p.name, state->presetName, PRESET_NAME_MAX);
        p.halfLife = *halfLife;
        p.waveformCount = *waveformCount;
        for (int i = 0; i < *waveformCount; i++) {
            p.waveforms[i] = waveforms[i];
        }
        PresetSave(&p, filepath);
        state->presetFileCount = PresetListFiles("presets", state->presetFiles, MAX_PRESET_FILES);
    }

    // Preset list
    UILayoutRow(&l, listHeight);
    const char* listItems[MAX_PRESET_FILES];
    for (int i = 0; i < state->presetFileCount; i++) {
        listItems[i] = state->presetFiles[i];
    }
    int focus = -1;
    GuiListViewEx(UILayoutSlot(&l, 1.0f), listItems, state->presetFileCount,
                  &state->presetScrollIndex, &state->selectedPreset, &focus);

    UILayoutGroupEnd(&l);

    // Auto-load on selection change
    if (state->selectedPreset != state->prevSelectedPreset &&
        state->selectedPreset >= 0 &&
        state->selectedPreset < state->presetFileCount) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s", state->presetFiles[state->selectedPreset]);
        Preset p;
        if (PresetLoad(&p, filepath)) {
            strncpy(state->presetName, p.name, PRESET_NAME_MAX);
            *halfLife = p.halfLife;
            *waveformCount = p.waveformCount;
            for (int i = 0; i < p.waveformCount; i++) {
                waveforms[i] = p.waveforms[i];
            }
        }
        state->prevSelectedPreset = state->selectedPreset;
    }

    state->panelY = l.y;
}
