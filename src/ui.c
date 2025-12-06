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
    int contentRight = l->x + l->padding + innerWidth;
    int remainingWidth = contentRight - l->slotX;

    int slotWidth;
    if (widthRatio >= 1.0f) {
        slotWidth = remainingWidth;
    } else {
        slotWidth = (int)(innerWidth * widthRatio);
    }

    Rectangle r = { (float)l->slotX, (float)l->y, (float)slotWidth, (float)l->rowHeight };
    l->slotX += slotWidth;
    return r;
}

int UILayoutEnd(UILayout* l)
{
    return l->y + l->rowHeight + l->spacing;
}

void UILayoutGroupBegin(UILayout* l, const char* title)
{
    l->groupStartY = l->y;
    l->groupTitle = title;
    l->y += 14;  // groupTitleH - space for title
}

void UILayoutGroupEnd(UILayout* l)
{
    int groupH = (l->y + l->rowHeight + l->padding) - l->groupStartY;
    GuiGroupBox((Rectangle){l->x, l->groupStartY, l->width, groupH}, l->groupTitle);
    l->y = l->groupStartY + groupH + l->spacing * 2;
    l->rowHeight = 0;
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
    const int rowH = 20;
    const int listHeight = 80;
    const int colorPickerSize = 62;
    const float labelRatio = 0.38f;

    UILayout l = UILayoutBegin(10, state->panelY, 180, 8, 4);

    // Waveforms group
    UILayoutGroupBegin(&l, "Waveforms");

    UILayoutRow(&l, rowH);
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton(UILayoutSlot(&l, 1.0f), "New")) {
        waveforms[*waveformCount] = WaveformConfigDefault();
        waveforms[*waveformCount].color = presetColors[*waveformCount % 8];
        *selectedWaveform = *waveformCount;
        (*waveformCount)++;
    }
    GuiSetState(STATE_NORMAL);

    UILayoutRow(&l, listHeight);
    static char itemNames[MAX_WAVEFORMS][16];
    const char* listItems[MAX_WAVEFORMS];
    for (int i = 0; i < *waveformCount; i++) {
        (void)snprintf(itemNames[i], sizeof(itemNames[i]), "Waveform %d", i + 1);
        listItems[i] = itemNames[i];
    }
    int focus = -1;
    GuiListViewEx(UILayoutSlot(&l, 1.0f), listItems, *waveformCount,
                  &state->waveformScrollIndex, selectedWaveform, &focus);

    UILayoutGroupEnd(&l);

    // Settings group
    WaveformConfig* sel = &waveforms[*selectedWaveform];
    UILayoutGroupBegin(&l, TextFormat("Waveform %d", *selectedWaveform + 1));

    UILayoutRow(&l, rowH);
    DrawText("Radius", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->radius, 0.05f, 0.45f);

    UILayoutRow(&l, rowH);
    DrawText("Height", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->amplitudeScale, 0.05f, 0.5f);

    UILayoutRow(&l, rowH);
    DrawText("Thickness", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->thickness, 1.0f, 10.0f);

    UILayoutRow(&l, rowH);
    DrawText("Smooth", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->smoothness, 0.0f, 50.0f);

    UILayoutRow(&l, rowH);
    DrawText(TextFormat("Rot %.3f", sel->rotationSpeed), l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, &sel->rotationSpeed, -0.05f, 0.05f);

    UILayoutRow(&l, colorPickerSize);
    DrawText("Color", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    Rectangle colorSlot = UILayoutSlot(&l, 1.0f);
    GuiColorPicker((Rectangle){colorSlot.x, colorSlot.y, colorSlot.width - 24, colorSlot.height}, NULL, &sel->color);

    UILayoutRow(&l, rowH);
    DrawText("Alpha", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    float alpha = sel->color.a / 255.0f;
    GuiColorBarAlpha(UILayoutSlot(&l, 1.0f), NULL, &alpha);
    sel->color.a = (unsigned char)(alpha * 255.0f);

    UILayoutGroupEnd(&l);

    // Trails group
    UILayoutGroupBegin(&l, "Trails");

    UILayoutRow(&l, rowH);
    DrawText("Half-life", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    GuiSliderBar(UILayoutSlot(&l, 1.0f), NULL, NULL, halfLife, 0.1f, 2.0f);

    UILayoutGroupEnd(&l);

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
