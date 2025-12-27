#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "config/spectrum_bars_config.h"
#include <math.h>

// Persistent section open states
static bool sectionGeometry = true;
static bool sectionDynamics = true;
static bool sectionAnimation = true;
static bool sectionColor = true;

void ImGuiDrawSpectrumPanel(SpectrumConfig* cfg)
{
    if (!ImGui::Begin("Spectrum")) {
        ImGui::End();
        return;
    }

    // Enable toggle with magenta accent
    ImGui::TextColored(Theme::ACCENT_MAGENTA, "Spectrum Bars");
    ImGui::Spacing();
    ImGui::Checkbox("Enabled", &cfg->enabled);

    ImGui::Spacing();

    // Geometry section - Cyan accent
    if (DrawSectionBegin("Geometry", Theme::GLOW_CYAN, &sectionGeometry)) {
        ImGui::SliderFloat("Radius", &cfg->innerRadius, 0.05f, 0.4f);
        ImGui::SliderFloat("Height", &cfg->barHeight, 0.1f, 0.5f);
        ImGui::SliderFloat("Width", &cfg->barWidth, 0.3f, 1.0f);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Dynamics section - Magenta accent
    if (DrawSectionBegin("Dynamics", Theme::GLOW_MAGENTA, &sectionDynamics)) {
        ImGui::SliderFloat("Smooth", &cfg->smoothing, 0.0f, 0.95f);
        ImGui::SliderFloat("Min dB", &cfg->minDb, 0.0f, 40.0f, "%.1f dB");
        ImGui::SliderFloat("Max dB", &cfg->maxDb, 20.0f, 60.0f, "%.1f dB");
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Animation section - Orange accent
    if (DrawSectionBegin("Animation", Theme::GLOW_ORANGE, &sectionAnimation)) {
        SliderAngleDeg("Rotation", &cfg->rotationSpeed, -2.87f, 2.87f, "%.2f °/f");
        SliderAngleDeg("Offset", &cfg->rotationOffset, 0.0f, 360.0f);
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Color section - Cyan accent (cycle)
    if (DrawSectionBegin("Color", Theme::GLOW_CYAN, &sectionColor)) {
        ImGuiDrawColorMode(&cfg->color);
        DrawSectionEnd();
    }

    ImGui::End();
}
