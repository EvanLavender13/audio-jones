#include "imgui.h"
#include "ui/imgui_panels.h"
#include "config/waveform_config.h"
#include "render/waveform.h"
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

void ImGuiDrawWaveformsPanel(WaveformConfig* waveforms, int* count, int* selected)
{
    if (!ImGui::Begin("Waveforms")) {
        ImGui::End();
        return;
    }

    // New button
    ImGui::BeginDisabled(*count >= MAX_WAVEFORMS);
    if (ImGui::Button("New")) {
        waveforms[*count] = WaveformConfig{};
        waveforms[*count].color.solid = presetColors[*count % 8];
        *selected = *count;
        (*count)++;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Delete button
    ImGui::BeginDisabled(*count <= 1);
    if (ImGui::Button("Delete") && *count > 1) {
        // Shift remaining waveforms down
        for (int i = *selected; i < *count - 1; i++) {
            waveforms[i] = waveforms[i + 1];
        }
        (*count)--;
        if (*selected >= *count) {
            *selected = *count - 1;
        }
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    // Waveform list
    if (ImGui::BeginListBox("##WaveformList", ImVec2(-FLT_MIN, 80))) {
        for (int i = 0; i < *count; i++) {
            char label[32];
            snprintf(label, sizeof(label), "Waveform %d", i + 1);
            if (ImGui::Selectable(label, *selected == i)) {
                *selected = i;
            }
        }
        ImGui::EndListBox();
    }

    // Selected waveform settings
    if (*selected >= 0 && *selected < *count) {
        ImGui::Separator();

        WaveformConfig* sel = &waveforms[*selected];

        ImGui::Text("Waveform %d", *selected + 1);

        // Geometry
        ImGui::SliderFloat("Radius", &sel->radius, 0.05f, 0.45f);
        ImGui::SliderFloat("Height", &sel->amplitudeScale, 0.05f, 0.5f);
        ImGui::SliderInt("Thickness", &sel->thickness, 1, 25);
        ImGui::SliderFloat("Smooth", &sel->smoothness, 0.0f, 100.0f);

        // Rotation
        ImGui::SliderFloat("Rotation", &sel->rotationSpeed, -0.05f, 0.05f, "%.4f rad");
        ImGui::SliderFloat("Offset", &sel->rotationOffset, 0.0f, 2.0f * PI, "%.2f rad");

        // Color
        ImGuiDrawColorMode(&sel->color);
    }

    ImGui::End();
}
