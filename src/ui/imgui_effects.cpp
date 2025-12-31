#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/kaleidoscope_config.h"
#include "automation/mod_sources.h"

// Persistent section open states
static bool sectionKaleidoscope = false;
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

    ImGui::TextColored(Theme::ACCENT_CYAN, "Core Effects");
    ImGui::Spacing();
    ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px", modSources);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", "%.0f px", modSources);
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);
    ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");
    ImGui::SliderFloat("Clarity", &e->clarity, 0.0f, 2.0f, "%.2f");

    ImGui::Spacing();

    if (DrawSectionBegin("Kaleidoscope", Theme::GLOW_CYAN, &sectionKaleidoscope)) {
        const char* modeLabels[] = {"Polar", "KIFS"};
        int mode = (int)e->kaleidoscope.mode;
        if (ImGui::Combo("Mode", &mode, modeLabels, 2)) {
            e->kaleidoscope.mode = (KaleidoscopeMode)mode;
        }

        // Universal params
        ImGui::SliderInt("Segments", &e->kaleidoscope.segments, 1, 12);
        SliderAngleDeg("Spin", &e->kaleidoscope.rotationSpeed, -0.6f, 0.6f, "%.2f °/f");
        SliderAngleDeg("Twist", &e->kaleidoscope.twistAmount, -60.0f, 60.0f, "%.1f °");

        // KIFS-only params
        if (e->kaleidoscope.mode == KALEIDO_KIFS) {
            ImGui::SliderInt("Iterations", &e->kaleidoscope.kifsIterations, 1, 8);
            ImGui::SliderFloat("Scale##kifs", &e->kaleidoscope.kifsScale, 1.1f, 4.0f, "%.2f");
            ImGui::SliderFloat("Offset X", &e->kaleidoscope.kifsOffsetX, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Offset Y", &e->kaleidoscope.kifsOffsetY, 0.0f, 2.0f, "%.2f");
        }

        // Universal params (continued)
        ImGui::SliderFloat("Focal Amp", &e->kaleidoscope.focalAmplitude, 0.0f, 0.2f, "%.3f");
        ImGui::SliderFloat("Focal Freq X", &e->kaleidoscope.focalFreqX, 0.1f, 5.0f, "%.2f");
        ImGui::SliderFloat("Focal Freq Y", &e->kaleidoscope.focalFreqY, 0.1f, 5.0f, "%.2f");
        ImGui::SliderFloat("Warp", &e->kaleidoscope.warpStrength, 0.0f, 0.5f, "%.3f");
        ImGui::SliderFloat("Warp Speed", &e->kaleidoscope.warpSpeed, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Noise Scale", &e->kaleidoscope.noiseScale, 0.5f, 10.0f, "%.1f");
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Voronoi", Theme::GLOW_ORANGE, &sectionVoronoi)) {
        ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
        if (e->voronoi.enabled) {
            ModulatableSlider("Intensity##vor", &e->voronoi.intensity,
                              "voronoi.intensity", "%.2f", modSources);
            ModulatableSlider("Scale##vor", &e->voronoi.scale,
                              "voronoi.scale", "%.1f", modSources);
            ModulatableSlider("Speed##vor", &e->voronoi.speed,
                              "voronoi.speed", "%.2f", modSources);
            ModulatableSlider("Edge##vor", &e->voronoi.edgeWidth,
                              "voronoi.edgeWidth", "%.3f", modSources);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Physarum", Theme::GLOW_CYAN, &sectionPhysarum)) {
        ImGui::Checkbox("Enabled##phys", &e->physarum.enabled);
        if (e->physarum.enabled) {
            ImGui::SliderInt("Agents", &e->physarum.agentCount, 10000, 1000000);
            ModulatableSlider("Sensor Dist", &e->physarum.sensorDistance,
                              "physarum.sensorDistance", "%.1f px", modSources);
            ModulatableSliderAngleDeg("Sensor Angle", &e->physarum.sensorAngle,
                                      "physarum.sensorAngle", modSources);
            ModulatableSliderAngleDeg("Turn Angle", &e->physarum.turningAngle,
                                      "physarum.turningAngle", modSources);
            ModulatableSlider("Step Size", &e->physarum.stepSize,
                              "physarum.stepSize", "%.1f px", modSources);
            ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
            ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion", &e->physarum.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost", &e->physarum.boostIntensity, 0.0f, 5.0f);
            const char* blendModes[] = {"Boost", "Tinted Boost", "Screen", "Mix", "Soft Light"};
            int blendModeInt = (int)e->physarum.blendMode;
            if (ImGui::Combo("Blend Mode", &blendModeInt, blendModes, 5)) {
                e->physarum.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);
            ImGuiDrawColorMode(&e->physarum.color);
            ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Flow Field", Theme::GLOW_MAGENTA, &sectionFlowField)) {
        ModulatableSlider("Zoom Base", &e->flowField.zoomBase,
                          "flowField.zoomBase", "%.4f", modSources);
        ModulatableSlider("Zoom Radial", &e->flowField.zoomRadial,
                          "flowField.zoomRadial", "%.4f", modSources);
        ModulatableSliderAngleDeg("Rot Base", &e->flowField.rotBase,
                                  "flowField.rotBase", modSources, "%.2f °/f");
        ModulatableSliderAngleDeg("Rot Radial", &e->flowField.rotRadial,
                                  "flowField.rotRadial", modSources, "%.2f °/f");
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
