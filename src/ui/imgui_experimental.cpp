#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "config/experimental_config.h"

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

        ImGui::TextColored(Theme::ACCENT_MAGENTA, "Feedback");
        ImGui::Spacing();

        ImGui::SliderFloat("Half-life", &cfg->halfLife, 0.1f, 2.0f, "%.2f s");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Trail persistence in seconds");
        }

        ImGui::SliderFloat("Zoom", &cfg->zoomFactor, 0.98f, 1.0f, "%.4f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Zoom toward center per frame (lower = faster inward)");
        }

        ImGui::Spacing();
        ImGui::TextColored(Theme::ACCENT_CYAN, "Injection");
        ImGui::Spacing();

        ImGui::SliderFloat("Opacity", &cfg->injectionOpacity, 0.05f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Waveform blend strength (lower = more subtle seed)");
        }
    }

    ImGui::End();
}
