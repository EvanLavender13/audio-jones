#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "config/waveform_config.h"
#include "render/waveform.h"
#include <stdio.h>
#include <math.h>

// Preset colors for new waveforms - from ThemeColor constants
static const Color presetColors[] = {
    ThemeColor::NEON_CYAN,
    ThemeColor::NEON_MAGENTA,
    ThemeColor::NEON_ORANGE,
    ThemeColor::NEON_WHITE,
    ThemeColor::NEON_CYAN_BRIGHT,
    ThemeColor::NEON_MAGENTA_BRIGHT,
    ThemeColor::NEON_ORANGE_BRIGHT,
    ThemeColor::NEON_CYAN_DIM
};

// Persistent section open states
static bool sectionGeometry = true;
static bool sectionAnimation = true;
static bool sectionColor = true;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawWaveformsPanel(WaveformConfig* waveforms, int* count, int* selected)
{
    if (!ImGui::Begin("Waveforms")) {
        ImGui::End();
        return;
    }

    // Management section - Cyan header
    ImGui::TextColored(Theme::ACCENT_CYAN, "Waveform List");
    ImGui::Spacing();

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
        for (int i = *selected; i < *count - 1; i++) {
            waveforms[i] = waveforms[i + 1];
        }
        (*count)--;
        if (*selected >= *count) {
            *selected = *count - 1;
        }
    }
    ImGui::EndDisabled();

    ImGui::Spacing();

    // Waveform list
    if (ImGui::BeginListBox("##WaveformList", ImVec2(-FLT_MIN, 80))) {
        for (int i = 0; i < *count; i++) {
            char label[32];
            (void)snprintf(label, sizeof(label), "Waveform %d", i + 1);
            if (ImGui::Selectable(label, *selected == i)) {
                *selected = i;
            }
        }
        ImGui::EndListBox();
    }

    // Selected waveform settings
    if (*selected >= 0 && *selected < *count) {
        WaveformConfig* sel = &waveforms[*selected];

        ImGui::Spacing();

        // Geometry section - Cyan accent
        if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
            ImGui::SliderFloat("X", &sel->x, 0.0f, 1.0f);
            ImGui::SliderFloat("Y", &sel->y, 0.0f, 1.0f);
            ImGui::SliderFloat("Radius", &sel->radius, 0.05f, 0.45f);
            ImGui::SliderFloat("Height", &sel->amplitudeScale, 0.05f, 0.5f);
            ImGui::SliderInt("Thickness", &sel->thickness, 1, 25, "%d px");
            ImGui::SliderFloat("Smooth", &sel->smoothness, 0.0f, 100.0f, "%.1f px");
            DrawSectionEnd();
        }

        ImGui::Spacing();

        // Animation section - Magenta accent
        if (DrawSectionBegin("Animation", Theme::GLOW_MAGENTA, &sectionAnimation)) {
            SliderAngleDeg("Rotation", &sel->rotationSpeed, -2.87f, 2.87f, "%.2f °/f");
            SliderAngleDeg("Offset", &sel->rotationOffset, 0.0f, 360.0f);
            DrawSectionEnd();
        }

        ImGui::Spacing();

        // Color section - Orange accent
        if (DrawSectionBegin("Color", Theme::GLOW_ORANGE, &sectionColor)) {
            ImGuiDrawColorMode(&sel->color);
            DrawSectionEnd();
        }
    }

    ImGui::End();
}
