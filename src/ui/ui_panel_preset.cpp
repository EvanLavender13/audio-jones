#include "raygui.h"
#include "ui_panel_preset.h"
#include "ui_layout.h"
#include "../preset.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct PresetPanelState {
    char presetFiles[MAX_PRESET_FILES][PRESET_PATH_MAX];
    int presetFileCount;
    int selectedPreset;
    int presetScrollIndex;
    int prevSelectedPreset;
    char presetName[PRESET_NAME_MAX];
    bool presetNameEditMode;
};

PresetPanelState* PresetPanelInit(void)
{
    PresetPanelState* state = (PresetPanelState*)calloc(1, sizeof(PresetPanelState));
    if (state == NULL) {
        return NULL;
    }

    state->selectedPreset = -1;
    state->prevSelectedPreset = -1;
    strncpy(state->presetName, "Default", PRESET_NAME_MAX);
    state->presetFileCount = PresetListFiles("presets", state->presetFiles, MAX_PRESET_FILES);

    return state;
}

void PresetPanelUninit(PresetPanelState* state)
{
    free(state);
}

int UIDrawPresetPanel(PresetPanelState* state, int startY,
                      WaveformConfig* waveforms, int* waveformCount,
                      EffectsConfig* effects, AudioConfig* audio,
                      SpectrumConfig* spectrum)
{
    const int rowH = 20;
    const int listHeight = 48;
    const float labelRatio = 0.25f;

    UILayout l = UILayoutBegin(10, startY, 180, 8, 4);

    UILayoutGroupBegin(&l, "Presets");

    // Name input
    UILayoutRow(&l, rowH);
    DrawText("Name", l.x + l.padding, l.y + 4, 10, GRAY);
    (void)UILayoutSlot(&l, labelRatio);
    if (GuiTextBox(UILayoutSlot(&l, 1.0f), state->presetName, PRESET_NAME_MAX, state->presetNameEditMode) != 0) {
        state->presetNameEditMode = !state->presetNameEditMode;
    }

    // Save button
    UILayoutRow(&l, rowH);
    if (GuiButton(UILayoutSlot(&l, 1.0f), "Save") != 0) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s.json", state->presetName);
        Preset p;
        strncpy(p.name, state->presetName, PRESET_NAME_MAX);
        p.effects = *effects;
        p.audio = *audio;
        p.waveformCount = *waveformCount;
        for (int i = 0; i < *waveformCount; i++) {
            p.waveforms[i] = waveforms[i];
        }
        p.spectrum = *spectrum;
        if (!PresetSave(&p, filepath)) {
            TraceLog(LOG_WARNING, "PRESET: Failed to save %s", filepath);
        }
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
            *effects = p.effects;
            *audio = p.audio;
            *waveformCount = p.waveformCount;
            for (int i = 0; i < p.waveformCount; i++) {
                waveforms[i] = p.waveforms[i];
            }
            *spectrum = p.spectrum;
        }
        state->prevSelectedPreset = state->selectedPreset;
    }

    return l.y;
}
