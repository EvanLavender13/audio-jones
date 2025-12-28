#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "automation/mod_sources.h"

// Persistent section open states
static bool sectionVoronoi = false;
static bool sectionPhysarum = false;
static bool sectionFlowField = false;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawEffectsPanel(EffectConfig* e, const ModSources* modSources)
{
    if (!ImGui::Begin("Effects")) {
        ImGui::End();
        return;
    }

    // Core effects - Cyan header
    ImGui::TextColored(Theme::ACCENT_CYAN, "Core Effects");
    ImGui::Spacing();
    ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px", modSources);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", "%.0f px", modSources);
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);
    ImGui::SliderInt("Kaleido", &e->kaleidoSegments, 1, 12);
    ImGui::SliderFloat("Kaleido Spin", &e->kaleidoRotationSpeed, -0.01f, 0.01f, "%.4f");
    ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");

    ImGui::Spacing();

    // Voronoi - Orange accent
    if (DrawSectionBegin("Voronoi", Theme::GLOW_ORANGE, &sectionVoronoi)) {
        ImGui::SliderFloat("Intensity", &e->voronoiIntensity, 0.0f, 1.0f);
        if (e->voronoiIntensity > 0.0f) {
            ImGui::SliderFloat("Scale##vor", &e->voronoiScale, 5.0f, 50.0f);
            ImGui::SliderFloat("Speed##vor", &e->voronoiSpeed, 0.1f, 2.0f);
            ImGui::SliderFloat("Edge", &e->voronoiEdgeWidth, 0.01f, 0.1f);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Physarum - Cyan accent
    if (DrawSectionBegin("Physarum", Theme::GLOW_CYAN, &sectionPhysarum)) {
        ImGui::Checkbox("Enabled##phys", &e->physarum.enabled);
        if (e->physarum.enabled) {
            ImGui::SliderInt("Agents", &e->physarum.agentCount, 10000, 1000000);
            ModulatableSlider("Sensor Dist", &e->physarum.sensorDistance,
                              "physarum.sensorDistance", "%.1f px", modSources);
            ModulatableSlider("Sensor Angle", &e->physarum.sensorAngle,
                              "physarum.sensorAngle", "%.2f rad", modSources);
            ModulatableSlider("Turn Angle", &e->physarum.turningAngle,
                              "physarum.turningAngle", "%.2f rad", modSources);
            ModulatableSlider("Step Size", &e->physarum.stepSize,
                              "physarum.stepSize", "%.1f px", modSources);
            ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
            ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion", &e->physarum.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost", &e->physarum.boostIntensity, 0.0f, 5.0f);
            const char* blendModes[] = {"Boost", "Tinted Boost", "Screen", "Mix", "Soft Light"};
            int blendMode = (int)e->physarum.trailBlendMode;
            if (ImGui::Combo("Blend Mode", &blendMode, blendModes, 5)) {
                e->physarum.trailBlendMode = (TrailBlendMode)blendMode;
            }
            ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);
            ImGuiDrawColorMode(&e->physarum.color);
            ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    // Flow Field - Magenta accent
    if (DrawSectionBegin("Flow Field", Theme::GLOW_MAGENTA, &sectionFlowField)) {
        ModulatableSlider("Zoom Base", &e->flowField.zoomBase,
                          "flowField.zoomBase", "%.4f", modSources);
        ModulatableSlider("Zoom Radial", &e->flowField.zoomRadial,
                          "flowField.zoomRadial", "%.4f", modSources);
        ModulatableSlider("Rot Base", &e->flowField.rotBase,
                          "flowField.rotBase", "%.4f", modSources);
        ModulatableSlider("Rot Radial", &e->flowField.rotRadial,
                          "flowField.rotRadial", "%.4f", modSources);
        ModulatableSlider("DX Base", &e->flowField.dxBase,
                          "flowField.dxBase", "%.4f", modSources);
        ModulatableSlider("DX Radial", &e->flowField.dxRadial,
                          "flowField.dxRadial", "%.4f", modSources);
        ModulatableSlider("DY Base", &e->flowField.dyBase,
                          "flowField.dyBase", "%.4f", modSources);
        ModulatableSlider("DY Radial", &e->flowField.dyRadial,
                          "flowField.dyRadial", "%.4f", modSources);
        DrawSectionEnd();
    }

    ImGui::End();
}
