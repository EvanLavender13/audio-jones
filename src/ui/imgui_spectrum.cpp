#include "imgui.h"
#include "ui/imgui_panels.h"
#include "config/spectrum_bars_config.h"
#include <math.h>

void ImGuiDrawSpectrumPanel(SpectrumConfig* cfg)
{
    if (!ImGui::Begin("Spectrum")) {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Enabled", &cfg->enabled);

    ImGui::Separator();

    // Geometry
    ImGui::SliderFloat("Radius", &cfg->innerRadius, 0.05f, 0.4f);
    ImGui::SliderFloat("Height", &cfg->barHeight, 0.1f, 0.5f);
    ImGui::SliderFloat("Width", &cfg->barWidth, 0.3f, 1.0f);

    // Dynamics
    ImGui::SliderFloat("Smooth", &cfg->smoothing, 0.0f, 0.95f);
    ImGui::SliderFloat("Min dB", &cfg->minDb, 0.0f, 40.0f);
    ImGui::SliderFloat("Max dB", &cfg->maxDb, 20.0f, 60.0f);

    // Rotation
    ImGui::SliderFloat("Rotation", &cfg->rotationSpeed, -0.05f, 0.05f, "%.4f rad");
    ImGui::SliderFloat("Offset", &cfg->rotationOffset, 0.0f, 2.0f * PI, "%.2f rad");

    // Color
    ImGuiDrawColorMode(&cfg->color);

    ImGui::End();
}
