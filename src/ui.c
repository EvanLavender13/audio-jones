#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "ui.h"
#include "preset.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Layout helper implementation

UILayout UILayoutBegin(int x, int y, int width, int padding, int spacing)
{
    return (UILayout){
        .x = x,
        .y = y,
        .width = width,
        .padding = padding,
        .spacing = spacing,
        .rowHeight = 0,
        .slotX = x + padding
    };
}

void UILayoutRow(UILayout* l, int height)
{
    l->y += l->rowHeight + l->spacing;
    l->rowHeight = height;
    l->slotX = l->x + l->padding;
}

Rectangle UILayoutSlot(UILayout* l, float widthRatio)
{
    int innerWidth = l->width - 2 * l->padding;
    int slotWidth = (int)(innerWidth * widthRatio);
    Rectangle r = { (float)l->slotX, (float)l->y, (float)slotWidth, (float)l->rowHeight };
    l->slotX += slotWidth;
    return r;
}

int UILayoutEnd(UILayout* l)
{
    return l->y + l->rowHeight + l->spacing;
}

// UI State

struct UIState {
    // Panel layout
    int panelY;

    // Waveform panel state
    int waveformScrollIndex;

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
    UIState* state = calloc(1, sizeof(UIState));
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

// NOLINTNEXTLINE(readability-function-size) - cohesive UI panel, splitting fragments layout logic
void UIDrawWaveformPanel(UIState* state, WaveformConfig* waveforms,
                         int* waveformCount, int* selectedWaveform,
                         float* halfLife)
{
    const int panelX = 10;
    const int panelW = 180;
    const int padding = 8;
    const int spacing = 4;
    const int groupSpacing = 8;  // Space between group boxes
    const int groupTitleH = 14;  // Space for group box title
    const int listHeight = 80;
    const int colorPickerSize = 62;
    const int rowH = 20;

    int y = state->panelY;

    // Waveforms group box
    int waveformGroupH = groupTitleH + rowH + spacing + listHeight + padding;
    GuiGroupBox((Rectangle){panelX, y, panelW, waveformGroupH}, "Waveforms");
    y += groupTitleH;

    // New button
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton((Rectangle){panelX + padding, y, panelW - 2*padding, rowH}, "New")) {
        waveforms[*waveformCount] = WaveformConfigDefault();
        waveforms[*waveformCount].color = presetColors[*waveformCount % 8];
        *selectedWaveform = *waveformCount;
        (*waveformCount)++;
    }
    GuiSetState(STATE_NORMAL);
    y += rowH + spacing;

    // Waveform list
    static char itemNames[MAX_WAVEFORMS][16];
    const char* listItems[MAX_WAVEFORMS];
    for (int i = 0; i < *waveformCount; i++) {
        (void)snprintf(itemNames[i], sizeof(itemNames[i]), "Waveform %d", i + 1);
        listItems[i] = itemNames[i];
    }
    int focus = -1;
    GuiListViewEx((Rectangle){panelX + padding, y, panelW - 2*padding, listHeight},
                  listItems, *waveformCount, &state->waveformScrollIndex, selectedWaveform, &focus);

    y = state->panelY + waveformGroupH + groupSpacing;

    // Selected waveform settings group
    WaveformConfig* sel = &waveforms[*selectedWaveform];
    int settingsGroupH = groupTitleH + 5*(rowH + spacing) + colorPickerSize + padding;
    GuiGroupBox((Rectangle){panelX, y, panelW, settingsGroupH},
                TextFormat("Waveform %d", *selectedWaveform + 1));
    int settingsY = y;
    y += groupTitleH;

    const int labelW = 60;
    const int sliderX = panelX + padding + labelW;
    const int sliderW = panelW - 2*padding - labelW;

    // Radius
    DrawText("Radius", panelX + padding, y + 4, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, rowH - 2}, NULL, NULL, &sel->radius, 0.05f, 0.45f);
    y += rowH + spacing;

    // Height
    DrawText("Height", panelX + padding, y + 4, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, rowH - 2}, NULL, NULL, &sel->amplitudeScale, 0.05f, 0.5f);
    y += rowH + spacing;

    // Thickness
    DrawText("Thickness", panelX + padding, y + 4, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, rowH - 2}, NULL, NULL, &sel->thickness, 1.0f, 10.0f);
    y += rowH + spacing;

    // Smoothness
    DrawText("Smooth", panelX + padding, y + 4, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, rowH - 2}, NULL, NULL, &sel->smoothness, 0.0f, 50.0f);
    y += rowH + spacing;

    // Rotation
    DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), panelX + padding, y + 4, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, rowH - 2}, NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);
    y += rowH + spacing;

    // Color picker (subtract 24 for hue bar: HUEBAR_WIDTH=16 + HUEBAR_PADDING=8)
    DrawText("Color", panelX + padding, y + 4, 10, GRAY);
    GuiColorPicker((Rectangle){sliderX, y, sliderW - 24, colorPickerSize}, NULL, &sel->color);

    y = settingsY + settingsGroupH + groupSpacing;

    // Trails group
    int trailsGroupH = groupTitleH + rowH + padding;
    GuiGroupBox((Rectangle){panelX, y, panelW, trailsGroupH}, "Trails");
    y += groupTitleH;

    DrawText("Half-life", panelX + padding, y + 4, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, rowH - 2}, NULL, NULL, halfLife, 0.1f, 2.0f);

    state->panelY = y - groupTitleH + trailsGroupH + groupSpacing;
}

void UIDrawPresetPanel(UIState* state, WaveformConfig* waveforms,
                       int* waveformCount, float* halfLife)
{
    const int panelX = 10;
    const int panelW = 180;
    const int padding = 8;
    const int spacing = 4;
    const int groupTitleH = 14;
    const int rowH = 20;
    const int listHeight = 48;

    int y = state->panelY;

    int presetGroupH = groupTitleH + 2*(rowH + spacing) + listHeight + padding;
    GuiGroupBox((Rectangle){panelX, y, panelW, presetGroupH}, "Presets");
    y += groupTitleH;

    const int labelW = 40;
    const int inputX = panelX + padding + labelW;
    const int inputW = panelW - 2*padding - labelW;

    // Name input
    DrawText("Name", panelX + padding, y + 4, 10, GRAY);
    if (GuiTextBox((Rectangle){inputX, y, inputW, rowH}, state->presetName, PRESET_NAME_MAX, state->presetNameEditMode)) {
        state->presetNameEditMode = !state->presetNameEditMode;
    }
    y += rowH + spacing;

    // Save button
    if (GuiButton((Rectangle){panelX + padding, y, panelW - 2*padding, rowH}, "Save")) {
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
    y += rowH + spacing;

    // Preset list
    const char* listItems[MAX_PRESET_FILES];
    for (int i = 0; i < state->presetFileCount; i++) {
        listItems[i] = state->presetFiles[i];
    }
    int focus = -1;
    GuiListViewEx((Rectangle){panelX + padding, y, panelW - 2*padding, listHeight},
                  listItems, state->presetFileCount,
                  &state->presetScrollIndex, &state->selectedPreset, &focus);

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
}
