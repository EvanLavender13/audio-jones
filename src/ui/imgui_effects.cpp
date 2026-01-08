#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/kaleidoscope_config.h"
#include "config/voronoi_config.h"
#include "automation/mod_sources.h"

// Persistent section open states
static bool sectionMobius = false;
static bool sectionTurbulence = false;
static bool sectionKaleidoscope = false;
static bool sectionVoronoi = false;
static bool sectionPhysarum = false;
static bool sectionCurlFlow = false;
static bool sectionAttractorFlow = false;
static bool sectionFlowField = false;
static bool sectionInfiniteZoom = false;
static bool sectionRadialStreak = false;
static bool sectionMultiInversion = false;
static bool sectionConformalWarp = false;

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
            for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
                const TransformEffectType type = e->transformOrder[i];
                const char* name = TransformEffectName(type);

                bool isEnabled = false;
                switch (type) {
                    case TRANSFORM_MOBIUS:            isEnabled = e->mobius.enabled; break;
                    case TRANSFORM_TURBULENCE:        isEnabled = e->turbulence.enabled; break;
                    case TRANSFORM_KALEIDOSCOPE:      isEnabled = e->kaleidoscope.enabled; break;
                    case TRANSFORM_INFINITE_ZOOM:     isEnabled = e->infiniteZoom.enabled; break;
                    case TRANSFORM_RADIAL_STREAK:     isEnabled = e->radialStreak.enabled; break;
                    case TRANSFORM_MULTI_INVERSION:   isEnabled = e->multiInversion.enabled; break;
                    case TRANSFORM_CONFORMAL_WARP:    isEnabled = e->conformalWarp.enabled; break;
                    case TRANSFORM_VORONOI:           isEnabled = e->voronoi.enabled; break;
                    default: break;
                }

                if (!isEnabled) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                }

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

    ImGui::Spacing();

    int transformIdx = 0;
    if (DrawSectionBegin("Infinite Zoom", Theme::GetSectionGlow(transformIdx++), &sectionInfiniteZoom)) {
        ImGui::Checkbox("Enabled##infzoom", &e->infiniteZoom.enabled);
        if (e->infiniteZoom.enabled) {
            ImGui::SliderFloat("Speed##infzoom", &e->infiniteZoom.speed, 0.1f, 2.0f, "%.2f");
            ImGui::SliderFloat("Zoom Depth##infzoom", &e->infiniteZoom.zoomDepth, 1.0f, 5.0f, "%.1f");
            ImGui::SliderFloat("Focal Amp##infzoom", &e->infiniteZoom.focalAmplitude, 0.0f, 0.2f, "%.3f");
            ImGui::SliderFloat("Focal Freq X##infzoom", &e->infiniteZoom.focalFreqX, 0.1f, 5.0f, "%.2f");
            ImGui::SliderFloat("Focal Freq Y##infzoom", &e->infiniteZoom.focalFreqY, 0.1f, 5.0f, "%.2f");
            ImGui::SliderInt("Layers##infzoom", &e->infiniteZoom.layers, 2, 8);
            ModulatableSliderAngleDeg("Spiral Angle##infzoom", &e->infiniteZoom.spiralAngle,
                                      "infiniteZoom.spiralAngle", modSources);
            ModulatableSliderAngleDeg("Twist##infzoom", &e->infiniteZoom.spiralTwist,
                                      "infiniteZoom.spiralTwist", modSources);
            ModulatableSlider("Droste##infzoom", &e->infiniteZoom.drosteIntensity,
                              "infiniteZoom.drosteIntensity", "%.2f", modSources);
            if (e->infiniteZoom.drosteIntensity > 0.0f) {
                ImGui::SliderFloat("Droste Scale##infzoom", &e->infiniteZoom.drosteScale, 2.0f, 256.0f, "%.1f",
                                   ImGuiSliderFlags_Logarithmic);
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Kaleidoscope", Theme::GetSectionGlow(transformIdx++), &sectionKaleidoscope)) {
        ImGui::Checkbox("Enabled##kaleido", &e->kaleidoscope.enabled);
        if (e->kaleidoscope.enabled) {
            KaleidoscopeConfig* k = &e->kaleidoscope;

            // Shared params first - these affect all techniques
            ImGui::SliderInt("Segments", &k->segments, 1, 12);
            ModulatableSliderAngleDeg("Spin", &k->rotationSpeed,
                                      "kaleidoscope.rotationSpeed", modSources, "%.2f °/f");
            ModulatableSliderAngleDeg("Twist##kaleido", &k->twistAngle,
                                      "kaleidoscope.twistAngle", modSources, "%.1f °");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Technique toggle buttons - horizontal row with intensity on same line
            ImGui::Text("Techniques");
            ImGui::Spacing();

            // Helper: draws a toggle button that sets intensity to 1.0 or 0.0
            auto TechniqueToggle = [](const char* label, float* intensity, ImU32 activeColor) {
                const bool active = *intensity > 0.0f;
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(activeColor));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(activeColor));
                }
                if (ImGui::Button(label, ImVec2(70, 0))) {
                    *intensity = active ? 0.0f : 1.0f;
                }
                if (active) {
                    ImGui::PopStyleColor(2);
                }
                return active;
            };

            // Row 1: Polar, KIFS
            const bool polarActive = TechniqueToggle("Polar", &k->polarIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool kifsActive = TechniqueToggle("KIFS", &k->kifsIntensity, Theme::ACCENT_MAGENTA_U32);

            // Row 2: Iter Mirror, Hex
            const bool iterActive = TechniqueToggle("Mirror", &k->iterMirrorIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool hexActive = TechniqueToggle("Hex", &k->hexFoldIntensity, Theme::ACCENT_MAGENTA_U32);

            // Count active techniques for blend info
            const int activeCount = (polarActive ? 1 : 0) + (kifsActive ? 1 : 0) +
                                    (iterActive ? 1 : 0) + (hexActive ? 1 : 0);

            // Show blend sliders only when multiple techniques active
            if (activeCount > 1) {
                ImGui::Spacing();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Blend Mix");
                if (polarActive) {
                    ImGui::SliderFloat("Polar##mix", &k->polarIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (kifsActive) {
                    ImGui::SliderFloat("KIFS##mix", &k->kifsIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (iterActive) {
                    ImGui::SliderFloat("Mirror##mix", &k->iterMirrorIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (hexActive) {
                    ImGui::SliderFloat("Hex##mix", &k->hexFoldIntensity, 0.01f, 1.0f, "%.2f");
                }
            }

            // Technique-specific parameters - show inline when active
            if (kifsActive) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::ACCENT_MAGENTA_U32), "KIFS");
                ImGui::SliderInt("Iterations##kifs", &k->kifsIterations, 1, 8);
                ImGui::SliderFloat("Scale##kifs", &k->kifsScale, 1.1f, 4.0f, "%.2f");
                ImGui::SliderFloat("Offset X##kifs", &k->kifsOffsetX, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Offset Y##kifs", &k->kifsOffsetY, 0.0f, 2.0f, "%.2f");
            }

            if (hexActive) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::ACCENT_MAGENTA_U32), "Hex");
                ImGui::SliderFloat("Density##hex", &k->hexScale, 1.0f, 20.0f, "%.1f");
            }

            // Focal and Warp in collapsible sections at the bottom
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::TreeNode("Focal Offset##kaleido")) {
                ImGui::SliderFloat("Amplitude", &k->focalAmplitude, 0.0f, 0.2f, "%.3f");
                if (k->focalAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq X", &k->focalFreqX, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq Y", &k->focalFreqY, 0.1f, 5.0f, "%.2f");
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Warp##kaleido")) {
                ImGui::SliderFloat("Strength", &k->warpStrength, 0.0f, 0.5f, "%.3f");
                if (k->warpStrength > 0.0f) {
                    ImGui::SliderFloat("Speed", &k->warpSpeed, 0.0f, 1.0f, "%.2f");
                    ImGui::SliderFloat("Scale", &k->noiseScale, 0.5f, 10.0f, "%.1f");
                }
                ImGui::TreePop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Möbius", Theme::GetSectionGlow(transformIdx++), &sectionMobius)) {
        ImGui::Checkbox("Enabled##mobius", &e->mobius.enabled);
        if (e->mobius.enabled) {
            ImGui::SliderInt("Iterations##mobius", &e->mobius.iterations, 1, 12);
            ImGui::SliderFloat("Anim Speed##mobius", &e->mobius.animSpeed, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Pole Mag##mobius", &e->mobius.poleMagnitude, 0.0f, 0.5f, "%.3f");
            ModulatableSliderAngleDeg("Spin##mobius", &e->mobius.animRotation,
                              "mobius.animRotation", modSources, "%.2f °/f");
            ImGui::SliderFloat("UV Scale##mobius", &e->mobius.uvScale, 0.2f, 1.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Multi-Inversion", Theme::GetSectionGlow(transformIdx++), &sectionMultiInversion)) {
        ImGui::Checkbox("Enabled##multiinv", &e->multiInversion.enabled);
        if (e->multiInversion.enabled) {
            ImGui::SliderInt("Iterations##multiinv", &e->multiInversion.iterations, 1, 12);
            ImGui::SliderFloat("Radius##multiinv", &e->multiInversion.radius, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("Radius Scale##multiinv", &e->multiInversion.radiusScale, 0.5f, 1.5f, "%.2f");
            ImGui::SliderFloat("Focal Amp##multiinv", &e->multiInversion.focalAmplitude, 0.0f, 0.3f, "%.3f");
            ImGui::SliderFloat("Focal Freq X##multiinv", &e->multiInversion.focalFreqX, 0.1f, 5.0f, "%.2f");
            ImGui::SliderFloat("Focal Freq Y##multiinv", &e->multiInversion.focalFreqY, 0.1f, 5.0f, "%.2f");
            ImGui::SliderFloat("Phase Offset##multiinv", &e->multiInversion.phaseOffset, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Anim Speed##multiinv", &e->multiInversion.animSpeed, 0.0f, 2.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Conformal Warp", Theme::GetSectionGlow(transformIdx++), &sectionConformalWarp)) {
        ImGui::Checkbox("Enabled##confwarp", &e->conformalWarp.enabled);
        if (e->conformalWarp.enabled) {
            if (ImGui::SliderFloat("Power##confwarp", &e->conformalWarp.powerMapN, 0.5f, 8.0f, "%.1f")) {
                e->conformalWarp.powerMapN = roundf(e->conformalWarp.powerMapN * 2.0f) / 2.0f;
            }
            ModulatableSliderAngleDeg("Spin##confwarp", &e->conformalWarp.rotationSpeed,
                                      "conformalWarp.rotationSpeed", modSources, "%.2f °/f");
            if (ImGui::TreeNode("Focal Offset##confwarp")) {
                ImGui::SliderFloat("Amplitude##confwarp", &e->conformalWarp.focalAmplitude, 0.0f, 0.2f, "%.3f");
                if (e->conformalWarp.focalAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq X##confwarp", &e->conformalWarp.focalFreqX, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq Y##confwarp", &e->conformalWarp.focalFreqY, 0.1f, 5.0f, "%.2f");
                }
                ImGui::TreePop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Radial Streak", Theme::GetSectionGlow(transformIdx++), &sectionRadialStreak)) {
        ImGui::Checkbox("Enabled##streak", &e->radialStreak.enabled);
        if (e->radialStreak.enabled) {
            ImGui::SliderInt("Samples##streak", &e->radialStreak.samples, 8, 32);
            ImGui::SliderFloat("Streak Length##streak", &e->radialStreak.streakLength, 0.1f, 1.0f, "%.2f");
            ModulatableSliderAngleDeg("Twist##streak", &e->radialStreak.spiralTwist,
                                      "radialStreak.spiralTwist", modSources);
            ImGui::SliderFloat("Focal Amp##streak", &e->radialStreak.focalAmplitude, 0.0f, 0.2f, "%.3f");
            ImGui::SliderFloat("Focal Freq X##streak", &e->radialStreak.focalFreqX, 0.1f, 5.0f, "%.2f");
            ImGui::SliderFloat("Focal Freq Y##streak", &e->radialStreak.focalFreqY, 0.1f, 5.0f, "%.2f");
            ImGui::SliderFloat("Anim Speed##streak", &e->radialStreak.animSpeed, 0.0f, 2.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Turbulence", Theme::GetSectionGlow(transformIdx++), &sectionTurbulence)) {
        ImGui::Checkbox("Enabled##turb", &e->turbulence.enabled);
        if (e->turbulence.enabled) {
            ImGui::SliderInt("Octaves##turb", &e->turbulence.octaves, 1, 8);
            ModulatableSlider("Strength##turb", &e->turbulence.strength,
                              "turbulence.strength", "%.2f", modSources);
            ImGui::SliderFloat("Anim Speed##turb", &e->turbulence.animSpeed, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Octave Twist##turb", &e->turbulence.octaveTwist,
                                      "turbulence.octaveTwist", modSources);
            ImGui::SliderFloat("UV Scale##turb", &e->turbulence.uvScale, 0.2f, 1.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Voronoi", Theme::GetSectionGlow(transformIdx++), &sectionVoronoi)) {
        ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
        if (e->voronoi.enabled) {
            VoronoiConfig* v = &e->voronoi;

            // Shared params first
            ModulatableSlider("Scale##vor", &v->scale, "voronoi.scale", "%.1f", modSources);
            ModulatableSlider("Speed##vor", &v->speed, "voronoi.speed", "%.2f", modSources);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Effect toggle buttons
            ImGui::Text("Effects");
            ImGui::Spacing();

            // Helper: draws a toggle button that sets intensity to 1.0 or 0.0
            auto EffectToggle = [](const char* label, float* intensity, ImU32 activeColor) {
                const bool active = *intensity > 0.0f;
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(activeColor));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(activeColor));
                }
                if (ImGui::Button(label, ImVec2(70, 0))) {
                    *intensity = active ? 0.0f : 1.0f;
                }
                if (active) {
                    ImGui::PopStyleColor(2);
                }
                return active;
            };

            // Row 1: UV Distort, Edge Iso, Center Iso
            const bool uvDistortActive = EffectToggle("Distort", &v->uvDistortIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeIsoActive = EffectToggle("Edge Iso", &v->edgeIsoIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool centerIsoActive = EffectToggle("Ctr Iso", &v->centerIsoIntensity, Theme::ACCENT_ORANGE_U32);

            // Row 2: Flat Fill, Edge Darken, Angle Shade
            const bool flatFillActive = EffectToggle("Fill", &v->flatFillIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeDarkenActive = EffectToggle("Darken", &v->edgeDarkenIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool angleShadeActive = EffectToggle("Angle", &v->angleShadeIntensity, Theme::ACCENT_ORANGE_U32);

            // Row 3: Determinant, Ratio, Edge Detect
            const bool determinantActive = EffectToggle("Determ", &v->determinantIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool ratioActive = EffectToggle("Ratio", &v->ratioIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeDetectActive = EffectToggle("Detect", &v->edgeDetectIntensity, Theme::ACCENT_ORANGE_U32);

            // Count active effects for blend info
            const int activeCount = (uvDistortActive ? 1 : 0) + (edgeIsoActive ? 1 : 0) + (centerIsoActive ? 1 : 0) +
                                    (flatFillActive ? 1 : 0) + (edgeDarkenActive ? 1 : 0) + (angleShadeActive ? 1 : 0) +
                                    (determinantActive ? 1 : 0) + (ratioActive ? 1 : 0) + (edgeDetectActive ? 1 : 0);

            // Show blend sliders only when multiple effects active
            if (activeCount > 1) {
                ImGui::Spacing();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Blend Mix");
                if (uvDistortActive) {
                    ImGui::SliderFloat("Distort##mix", &v->uvDistortIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeIsoActive) {
                    ImGui::SliderFloat("Edge Iso##mix", &v->edgeIsoIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (centerIsoActive) {
                    ImGui::SliderFloat("Ctr Iso##mix", &v->centerIsoIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (flatFillActive) {
                    ImGui::SliderFloat("Fill##mix", &v->flatFillIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeDarkenActive) {
                    ImGui::SliderFloat("Darken##mix", &v->edgeDarkenIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (angleShadeActive) {
                    ImGui::SliderFloat("Angle##mix", &v->angleShadeIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (determinantActive) {
                    ImGui::SliderFloat("Determ##mix", &v->determinantIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (ratioActive) {
                    ImGui::SliderFloat("Ratio##mix", &v->ratioIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeDetectActive) {
                    ImGui::SliderFloat("Detect##mix", &v->edgeDetectIntensity, 0.01f, 1.0f, "%.2f");
                }
            }

            // Shared effect modifiers in collapsible sections
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::TreeNode("Iso Settings##vor")) {
                ModulatableSlider("Frequency", &v->isoFrequency, "voronoi.isoFrequency", "%.1f", modSources);
                ModulatableSlider("Edge Falloff", &v->edgeFalloff, "voronoi.edgeFalloff", "%.2f", modSources);
                ImGui::TreePop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::End();
}
