#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "config/experimental_config.h"

// Persistent section open states
static bool sectionFeedback = true;
static bool sectionFlowField = false;
static bool sectionInjection = true;

void ImGuiDrawExperimentalPanel(ExperimentalConfig* cfg, bool* useExperimental)
{
    if (!ImGui::Begin("Experimental")) {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Enable Experimental Pipeline", useExperimental);

    if (*useExperimental) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Feedback section - Magenta accent
        if (DrawSectionBegin("Feedback", Theme::GLOW_MAGENTA, &sectionFeedback)) {
            SliderFloatWithTooltip("Half-life", &cfg->halfLife, 0.1f, 2.0f, "%.2f s",
                                   "Trail persistence in seconds");
            DrawSectionEnd();
        }

        ImGui::Spacing();

        // Flow Field section - Cyan accent
        if (DrawSectionBegin("Flow Field", Theme::GLOW_CYAN, &sectionFlowField)) {
            SliderFloatWithTooltip("Zoom Base", &cfg->flowField.zoomBase, 0.98f, 1.02f, "%.4f",
                                   "Base zoom factor (<1 pulls inward, >1 pushes outward)");
            SliderFloatWithTooltip("Zoom Radial", &cfg->flowField.zoomRadial, -0.02f, 0.02f, "%.4f",
                                   "Radial zoom variation (+ = center zooms faster)");

            ImGui::Spacing();

            SliderFloatWithTooltip("Rot Base", &cfg->flowField.rotBase, -0.01f, 0.01f, "%.4f rad",
                                   "Base rotation per frame in radians");
            SliderFloatWithTooltip("Rot Radial", &cfg->flowField.rotRadial, -0.01f, 0.01f, "%.4f rad",
                                   "Radial rotation variation (+ = center rotates faster)");

            ImGui::Spacing();

            SliderFloatWithTooltip("DX Base", &cfg->flowField.dxBase, -0.02f, 0.02f, "%.4f",
                                   "Base horizontal translation per frame");
            SliderFloatWithTooltip("DX Radial", &cfg->flowField.dxRadial, -0.02f, 0.02f, "%.4f",
                                   "Radial horizontal variation (+ = edges push outward)");
            SliderFloatWithTooltip("DY Base", &cfg->flowField.dyBase, -0.02f, 0.02f, "%.4f",
                                   "Base vertical translation per frame");
            SliderFloatWithTooltip("DY Radial", &cfg->flowField.dyRadial, -0.02f, 0.02f, "%.4f",
                                   "Radial vertical variation (+ = edges push outward)");

            DrawSectionEnd();
        }

        ImGui::Spacing();

        // Injection section - Orange accent
        if (DrawSectionBegin("Injection", Theme::GLOW_ORANGE, &sectionInjection)) {
            SliderFloatWithTooltip("Opacity", &cfg->injectionOpacity, 0.05f, 1.0f, "%.2f",
                                   "Waveform blend strength (lower = more subtle seed)");
            DrawSectionEnd();
        }
    }

    ImGui::End();
}
