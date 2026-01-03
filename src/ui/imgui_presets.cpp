#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "config/preset.h"
#include "config/app_configs.h"
#include "render/post_effect.h"
#include <stdio.h>
#include <string.h>

// UI state for preset panel (file list cache and selection).
// Unlike other panels which only display config, this panel maintains persistent
// state for file browser functionality that isn't part of AppConfigs.
static char presetFiles[MAX_PRESET_FILES][PRESET_PATH_MAX];
static int presetFileCount = 0;
static int selectedPreset = -1;
static int prevSelectedPreset = -1;
static char presetName[PRESET_NAME_MAX] = "Default";
static bool initialized = false;

static void RefreshPresetList(void)
{
    presetFileCount = PresetListFiles("presets", presetFiles, MAX_PRESET_FILES);
}

void ImGuiDrawPresetPanel(AppConfigs* configs)
{
    if (!initialized) {
        RefreshPresetList();
        initialized = true;
    }

    if (!ImGui::Begin("Presets")) {
        ImGui::End();
        return;
    }

    // Save section - Cyan header
    ImGui::TextColored(Theme::ACCENT_CYAN, "Save Preset");
    ImGui::Spacing();

    // Name input
    ImGui::InputText("Name", presetName, PRESET_NAME_MAX);

    // Save button
    if (ImGui::Button("Save", ImVec2(-1, 0))) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s.json", presetName);
        Preset p;
        strncpy(p.name, presetName, PRESET_NAME_MAX);
        p.name[PRESET_NAME_MAX - 1] = '\0';
        PresetFromAppConfigs(&p, configs);
        if (PresetSave(&p, filepath)) {
            RefreshPresetList();
        }
    }

    ImGui::Spacing();

    // Load section - Magenta header
    ImGui::TextColored(Theme::ACCENT_MAGENTA, "Load Preset");
    ImGui::Spacing();

    // Preset list
    if (ImGui::BeginListBox("##presets", ImVec2(-1, 120))) {
        for (int i = 0; i < presetFileCount; i++) {
            const bool isSelected = (selectedPreset == i);
            if (ImGui::Selectable(presetFiles[i], isSelected)) {
                selectedPreset = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    // Auto-load on selection change
    if (selectedPreset != prevSelectedPreset &&
        selectedPreset >= 0 &&
        selectedPreset < presetFileCount) {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, sizeof(filepath), "presets/%s", presetFiles[selectedPreset]);
        Preset p;
        if (PresetLoad(&p, filepath)) {
            strncpy(presetName, p.name, PRESET_NAME_MAX);
            presetName[PRESET_NAME_MAX - 1] = '\0';
            PresetToAppConfigs(&p, configs);
            PostEffectClearFeedback(configs->postEffect);
        }
        prevSelectedPreset = selectedPreset;
    }

    ImGui::Spacing();

    // Refresh button
    if (ImGui::Button("Refresh", ImVec2(-1, 0))) {
        RefreshPresetList();
    }

    ImGui::End();
}
