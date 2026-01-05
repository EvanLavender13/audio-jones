#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/kaleidoscope_config.h"
#include "automation/mod_sources.h"

// Persistent section open states
static bool sectionEffectOrder = false;
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

// Selection tracking for effect order list
static int selectedTransformEffect = 0;

// Shared blend mode options for simulation effects
static const char* BLEND_MODES[] = {"Boost", "Tinted Boost", "Screen", "Mix", "Soft Light"};
static const int BLEND_MODE_COUNT = 5;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawEffectsPanel(EffectConfig* e, const ModSources* modSources)
{
    if (!ImGui::Begin("Effects")) {
        ImGui::End();
        return;
    }

    if (DrawSectionBegin("Effect Order", Theme::GLOW_CYAN, &sectionEffectOrder)) {
        // Up/Down reorder buttons
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

        // Effect order list
        if (ImGui::BeginListBox("##EffectOrderList", ImVec2(-FLT_MIN, 80))) {
            for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
                const TransformEffectType type = e->transformOrder[i];
                const char* name = TransformEffectName(type);

                // Check enabled state for dimming
                bool isEnabled = false;
                switch (type) {
                    case TRANSFORM_MOBIUS:            isEnabled = e->mobius.enabled; break;
                    case TRANSFORM_TURBULENCE:        isEnabled = e->turbulence.enabled; break;
                    case TRANSFORM_KALEIDOSCOPE:      isEnabled = e->kaleidoscope.enabled; break;
                    case TRANSFORM_INFINITE_ZOOM:     isEnabled = e->infiniteZoom.enabled; break;
                    case TRANSFORM_RADIAL_STREAK:     isEnabled = e->radialStreak.enabled; break;
                    case TRANSFORM_MULTI_INVERSION:   isEnabled = e->multiInversion.enabled; break;
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
        DrawSectionEnd();
    }

    ImGui::Spacing();

    ImGui::TextColored(Theme::ACCENT_CYAN, "Core Effects");
    ImGui::Spacing();
    ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px", modSources);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", "%.0f px", modSources);
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);
    ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");
    ImGui::SliderFloat("Clarity", &e->clarity, 0.0f, 2.0f, "%.2f");

    ImGui::Spacing();

    if (DrawSectionBegin("Möbius", Theme::GLOW_MAGENTA, &sectionMobius)) {
        ImGui::Checkbox("Enabled##mobius", &e->mobius.enabled);
        if (e->mobius.enabled) {
            ImGui::SliderInt("Iterations##mobius", &e->mobius.iterations, 1, 12);
            ImGui::SliderFloat("Anim Speed##mobius", &e->mobius.animSpeed, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Pole Mag##mobius", &e->mobius.poleMagnitude, 0.0f, 0.5f, "%.3f");
            ImGui::SliderFloat("Rotation##mobius", &e->mobius.rotationSpeed, 0.0f, 2.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Turbulence", Theme::GLOW_ORANGE, &sectionTurbulence)) {
        ImGui::Checkbox("Enabled##turb", &e->turbulence.enabled);
        if (e->turbulence.enabled) {
            ImGui::SliderInt("Octaves##turb", &e->turbulence.octaves, 1, 8);
            ModulatableSlider("Strength##turb", &e->turbulence.strength,
                              "turbulence.strength", "%.2f", modSources);
            ImGui::SliderFloat("Anim Speed##turb", &e->turbulence.animSpeed, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Rotation##turb", &e->turbulence.rotationPerOctave,
                                      "turbulence.rotationPerOctave", modSources);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Radial Streak", Theme::GLOW_CYAN, &sectionRadialStreak)) {
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

    if (DrawSectionBegin("Multi-Inversion", Theme::GLOW_MAGENTA, &sectionMultiInversion)) {
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

    if (DrawSectionBegin("Kaleidoscope", Theme::GLOW_CYAN, &sectionKaleidoscope)) {
        ImGui::Checkbox("Enabled##kaleido", &e->kaleidoscope.enabled);
        if (e->kaleidoscope.enabled) {
            const char* modeLabels[] = {"Polar", "KIFS"};
            int mode = (int)e->kaleidoscope.mode;
            if (ImGui::Combo("Mode", &mode, modeLabels, 2)) {
                e->kaleidoscope.mode = (KaleidoscopeMode)mode;
            }

            // Universal params
            ImGui::SliderInt("Segments", &e->kaleidoscope.segments, 1, 12);
            ModulatableSliderAngleDeg("Spin", &e->kaleidoscope.rotationSpeed,
                                      "kaleidoscope.rotationSpeed", modSources, "%.2f °/f");
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
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Infinite Zoom", Theme::GLOW_MAGENTA, &sectionInfiniteZoom)) {
        ImGui::Checkbox("Enabled##infzoom", &e->infiniteZoom.enabled);
        if (e->infiniteZoom.enabled) {
            ImGui::SliderFloat("Speed##infzoom", &e->infiniteZoom.speed, 0.1f, 2.0f, "%.2f");
            ImGui::SliderFloat("Zoom Depth##infzoom", &e->infiniteZoom.zoomDepth, 1.0f, 5.0f, "%.1f");
            ImGui::SliderFloat("Focal Amp##infzoom", &e->infiniteZoom.focalAmplitude, 0.0f, 0.2f, "%.3f");
            ImGui::SliderFloat("Focal Freq X##infzoom", &e->infiniteZoom.focalFreqX, 0.1f, 5.0f, "%.2f");
            ImGui::SliderFloat("Focal Freq Y##infzoom", &e->infiniteZoom.focalFreqY, 0.1f, 5.0f, "%.2f");
            ImGui::SliderInt("Layers##infzoom", &e->infiniteZoom.layers, 2, 8);
            ModulatableSliderAngleDeg("Spiral##infzoom", &e->infiniteZoom.spiralTurns,
                                      "infiniteZoom.spiralTurns", modSources);
            ModulatableSliderAngleDeg("Twist##infzoom", &e->infiniteZoom.spiralTwist,
                                      "infiniteZoom.spiralTwist", modSources);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Voronoi", Theme::GLOW_ORANGE, &sectionVoronoi)) {
        ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
        if (e->voronoi.enabled) {
            ModulatableSlider("Strength##vor", &e->voronoi.strength,
                              "voronoi.strength", "%.2f", modSources);
            ModulatableSlider("Scale##vor", &e->voronoi.scale,
                              "voronoi.scale", "%.1f", modSources);
            ModulatableSlider("Speed##vor", &e->voronoi.speed,
                              "voronoi.speed", "%.2f", modSources);
            ModulatableSlider("Edge Falloff##vor", &e->voronoi.edgeFalloff,
                              "voronoi.edgeFalloff", "%.2f", modSources);
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

    if (DrawSectionBegin("Curl Flow", Theme::GLOW_ORANGE, &sectionCurlFlow)) {
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

    if (DrawSectionBegin("Attractor Flow", Theme::GLOW_CYAN, &sectionAttractorFlow)) {
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
            ModulatableSliderAngleDeg("Rot X##attr", &e->attractorFlow.rotationX,
                                      "attractorFlow.rotationX", modSources);
            ModulatableSliderAngleDeg("Rot Y##attr", &e->attractorFlow.rotationY,
                                      "attractorFlow.rotationY", modSources);
            ModulatableSliderAngleDeg("Rot Z##attr", &e->attractorFlow.rotationZ,
                                      "attractorFlow.rotationZ", modSources);
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
