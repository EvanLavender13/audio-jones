#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "automation/mod_sources.h"

// Persistent section open states (simulations only - transforms in imgui_effects_transforms.cpp)
static bool sectionPhysarum = false;
static bool sectionCurlFlow = false;
static bool sectionAttractorFlow = false;
static bool sectionFlowField = false;

// Selection tracking for effect order list
static int selectedTransformEffect = 0;

// Shared blend mode options for simulation effects
static const char* BLEND_MODES[] = {
    "Boost", "Tinted Boost", "Screen", "Mix", "Soft Light",
    "Overlay", "Color Burn", "Linear Burn", "Vivid Light", "Linear Light",
    "Pin Light", "Difference", "Negation", "Subtract", "Reflect", "Phoenix"
};
static const int BLEND_MODE_COUNT = 16;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawEffectsPanel(EffectConfig* e, const ModSources* modSources)
{
    if (!ImGui::Begin("Effects")) {
        ImGui::End();
        return;
    }

    // -------------------------------------------------------------------------
    // FEEDBACK GROUP
    // -------------------------------------------------------------------------
    DrawGroupHeader("FEEDBACK", Theme::ACCENT_CYAN_U32);

    ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px", modSources);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);

    ImGui::Spacing();

    if (DrawSectionBegin("Flow Field", Theme::GetSectionGlow(0), &sectionFlowField)) {
        ModulatableSlider("Zoom Base", &e->flowField.zoomBase,
                          "flowField.zoomBase", "%.4f", modSources);
        ModulatableSlider("Zoom Radial", &e->flowField.zoomRadial,
                          "flowField.zoomRadial", "%.4f", modSources);
        ModulatableSliderAngleDeg("Spin", &e->flowField.rotationSpeed,
                                  "flowField.rotationSpeed", modSources, "%.2f °/f");
        ModulatableSliderAngleDeg("Spin Radial", &e->flowField.rotationSpeedRadial,
                                  "flowField.rotationSpeedRadial", modSources, "%.2f °/f");
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

    // -------------------------------------------------------------------------
    // OUTPUT GROUP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Spacing();
    DrawGroupHeader("OUTPUT", Theme::ACCENT_MAGENTA_U32);

    ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", "%.0f px", modSources);
    ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");
    ImGui::SliderFloat("Clarity", &e->clarity, 0.0f, 2.0f, "%.2f");

    // -------------------------------------------------------------------------
    // SIMULATIONS GROUP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Spacing();
    DrawGroupHeader("SIMULATIONS", Theme::ACCENT_ORANGE_U32);

    int simIdx = 0;
    if (DrawSectionBegin("Physarum", Theme::GetSectionGlow(simIdx++), &sectionPhysarum)) {
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
            int blendModeInt = (int)e->physarum.blendMode;
            if (ImGui::Combo("Blend Mode", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->physarum.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);
            ImGuiDrawColorMode(&e->physarum.color);
            ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Curl Flow", Theme::GetSectionGlow(simIdx++), &sectionCurlFlow)) {
        ImGui::Checkbox("Enabled##curl", &e->curlFlow.enabled);
        if (e->curlFlow.enabled) {
            ImGui::SliderInt("Agents##curl", &e->curlFlow.agentCount, 1000, 1000000);
            ImGui::SliderFloat("Frequency", &e->curlFlow.noiseFrequency, 0.001f, 0.1f, "%.4f");
            ImGui::SliderFloat("Evolution", &e->curlFlow.noiseEvolution, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Density Influence", &e->curlFlow.trailInfluence, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Sense Blend##curl", &e->curlFlow.accumSenseBlend, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Gradient Radius", &e->curlFlow.gradientRadius, 1.0f, 32.0f, "%.0f px");
            ImGui::SliderFloat("Step Size##curl", &e->curlFlow.stepSize, 0.5f, 5.0f, "%.1f px");
            ImGui::SliderFloat("Deposit##curl", &e->curlFlow.depositAmount, 0.01f, 0.2f, "%.3f");
            ImGui::SliderFloat("Decay##curl", &e->curlFlow.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion##curl", &e->curlFlow.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost##curl", &e->curlFlow.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->curlFlow.blendMode;
            if (ImGui::Combo("Blend Mode##curl", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->curlFlow.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->curlFlow.color);
            ImGui::Checkbox("Debug##curl", &e->curlFlow.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Attractor Flow", Theme::GetSectionGlow(simIdx++), &sectionAttractorFlow)) {
        ImGui::Checkbox("Enabled##attr", &e->attractorFlow.enabled);
        if (e->attractorFlow.enabled) {
            const char* attractorTypes[] = {"Lorenz", "Rossler", "Aizawa", "Thomas"};
            int attractorType = (int)e->attractorFlow.attractorType;
            if (ImGui::Combo("Attractor##attr", &attractorType, attractorTypes, 4)) {
                e->attractorFlow.attractorType = (AttractorType)attractorType;
            }
            ImGui::SliderInt("Agents##attr", &e->attractorFlow.agentCount, 10000, 500000);
            ImGui::SliderFloat("Time Scale", &e->attractorFlow.timeScale, 0.001f, 0.1f, "%.3f");
            ImGui::SliderFloat("Scale##attr", &e->attractorFlow.attractorScale, 0.005f, 0.1f, "%.3f");
            ImGui::SliderFloat("X##attr", &e->attractorFlow.x, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Y##attr", &e->attractorFlow.y, 0.0f, 1.0f, "%.2f");
            ModulatableSliderAngleDeg("Angle X##attr", &e->attractorFlow.rotationAngleX,
                                      "attractorFlow.rotationAngleX", modSources);
            ModulatableSliderAngleDeg("Angle Y##attr", &e->attractorFlow.rotationAngleY,
                                      "attractorFlow.rotationAngleY", modSources);
            ModulatableSliderAngleDeg("Angle Z##attr", &e->attractorFlow.rotationAngleZ,
                                      "attractorFlow.rotationAngleZ", modSources);
            ModulatableSliderAngleDeg("Spin X##attr", &e->attractorFlow.rotationSpeedX,
                                      "attractorFlow.rotationSpeedX", modSources, "%.3f °/f");
            ModulatableSliderAngleDeg("Spin Y##attr", &e->attractorFlow.rotationSpeedY,
                                      "attractorFlow.rotationSpeedY", modSources, "%.3f °/f");
            ModulatableSliderAngleDeg("Spin Z##attr", &e->attractorFlow.rotationSpeedZ,
                                      "attractorFlow.rotationSpeedZ", modSources, "%.3f °/f");
            if (e->attractorFlow.attractorType == ATTRACTOR_LORENZ) {
                ImGui::SliderFloat("Sigma", &e->attractorFlow.sigma, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Rho", &e->attractorFlow.rho, 10.0f, 50.0f, "%.1f");
                ImGui::SliderFloat("Beta", &e->attractorFlow.beta, 0.5f, 5.0f, "%.2f");
            } else if (e->attractorFlow.attractorType == ATTRACTOR_ROSSLER) {
                ImGui::SliderFloat("C", &e->attractorFlow.rosslerC, 4.0f, 7.0f, "%.2f");
            } else if (e->attractorFlow.attractorType == ATTRACTOR_THOMAS) {
                ImGui::SliderFloat("B", &e->attractorFlow.thomasB, 0.17f, 0.22f, "%.4f");
            }
            ImGui::SliderFloat("Deposit##attr", &e->attractorFlow.depositAmount, 0.01f, 0.2f, "%.3f");
            ImGui::SliderFloat("Decay##attr", &e->attractorFlow.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion##attr", &e->attractorFlow.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost##attr", &e->attractorFlow.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->attractorFlow.blendMode;
            if (ImGui::Combo("Blend Mode##attr", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->attractorFlow.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->attractorFlow.color);
            ImGui::Checkbox("Debug##attr", &e->attractorFlow.debugOverlay);
        }
        DrawSectionEnd();
    }

    // -------------------------------------------------------------------------
    // TRANSFORMS GROUP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Spacing();
    DrawGroupHeader("TRANSFORMS", Theme::ACCENT_CYAN_U32);

    // Effect Order reorder UI (inline, not collapsible)
    {
        const bool canMoveUp = selectedTransformEffect > 0;
        ImGui::BeginDisabled(!canMoveUp);
        if (ImGui::Button("Up") && canMoveUp) {
            const TransformEffectType temp = e->transformOrder[selectedTransformEffect];
            e->transformOrder[selectedTransformEffect] = e->transformOrder[selectedTransformEffect - 1];
            e->transformOrder[selectedTransformEffect - 1] = temp;
            selectedTransformEffect--;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        const bool canMoveDown = selectedTransformEffect < TRANSFORM_EFFECT_COUNT - 1;
        ImGui::BeginDisabled(!canMoveDown);
        if (ImGui::Button("Down") && canMoveDown) {
            const TransformEffectType temp = e->transformOrder[selectedTransformEffect];
            e->transformOrder[selectedTransformEffect] = e->transformOrder[selectedTransformEffect + 1];
            e->transformOrder[selectedTransformEffect + 1] = temp;
            selectedTransformEffect++;
        }
        ImGui::EndDisabled();

        ImGui::Spacing();

        if (ImGui::BeginListBox("##EffectOrderList", ImVec2(-FLT_MIN, 80))) {
            ImDrawList* draw = ImGui::GetWindowDrawList();

            for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
                const TransformEffectType type = e->transformOrder[i];
                const char* name = TransformEffectName(type);

                bool isEnabled = false;
                switch (type) {
                    case TRANSFORM_SINE_WARP:         isEnabled = e->sineWarp.enabled; break;
                    case TRANSFORM_KALEIDOSCOPE:      isEnabled = e->kaleidoscope.enabled; break;
                    case TRANSFORM_INFINITE_ZOOM:     isEnabled = e->infiniteZoom.enabled; break;
                    case TRANSFORM_RADIAL_STREAK:     isEnabled = e->radialStreak.enabled; break;
                    case TRANSFORM_TEXTURE_WARP:      isEnabled = e->textureWarp.enabled; break;
                    case TRANSFORM_VORONOI:           isEnabled = e->voronoi.enabled; break;
                    case TRANSFORM_WAVE_RIPPLE:       isEnabled = e->waveRipple.enabled; break;
                    case TRANSFORM_MOBIUS:            isEnabled = e->mobius.enabled; break;
                    case TRANSFORM_PIXELATION:        isEnabled = e->pixelation.enabled; break;
                    case TRANSFORM_GLITCH:            isEnabled = e->glitch.enabled; break;
                    case TRANSFORM_POINCARE_DISK:     isEnabled = e->poincareDisk.enabled; break;
                    case TRANSFORM_TOON:              isEnabled = e->toon.enabled; break;
                    case TRANSFORM_HEIGHTFIELD_RELIEF: isEnabled = e->heightfieldRelief.enabled; break;
                    case TRANSFORM_GRADIENT_FLOW:     isEnabled = e->gradientFlow.enabled; break;
                    default: break;
                }

                // Alternating row background
                const ImVec2 rowMin = ImGui::GetCursorScreenPos();
                const ImVec2 rowMax = ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x,
                                             rowMin.y + ImGui::GetTextLineHeightWithSpacing());
                if (i % 2 == 1) {
                    draw->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 8));
                }

                // Left-edge enabled indicator (2px bar)
                if (isEnabled) {
                    draw->AddRectFilled(
                        rowMin,
                        ImVec2(rowMin.x + 2.0f, rowMax.y),
                        Theme::ACCENT_CYAN_U32
                    );
                }

                if (!isEnabled) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                }

                // Indent past the enabled indicator bar
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);

                if (ImGui::Selectable(name, selectedTransformEffect == i)) {
                    selectedTransformEffect = i;
                }

                if (!isEnabled) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndListBox();
        }
    }

    // Transform subcategories (extracted to imgui_effects_transforms.cpp)
    ImGui::Spacing();
    DrawSymmetryCategory(e, modSources);
    ImGui::Spacing();
    DrawWarpCategory(e, modSources);
    ImGui::Spacing();
    DrawCellularCategory(e, modSources);
    ImGui::Spacing();
    DrawMotionCategory(e, modSources);
    ImGui::Spacing();
    DrawStyleCategory(e, modSources);

    ImGui::End();
}
