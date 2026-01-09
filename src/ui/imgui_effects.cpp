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
static bool sectionSineWarp = false;
static bool sectionKaleidoscope = false;
static bool sectionVoronoi = false;
static bool sectionPhysarum = false;
static bool sectionCurlFlow = false;
static bool sectionAttractorFlow = false;
static bool sectionFlowField = false;
static bool sectionInfiniteZoom = false;
static bool sectionRadialStreak = false;
static bool sectionTextureWarp = false;
static bool sectionWaveRipple = false;
static bool sectionMobius = false;
static bool sectionPixelation = false;

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
                    case TRANSFORM_SINE_WARP:         isEnabled = e->sineWarp.enabled; break;
                    case TRANSFORM_KALEIDOSCOPE:      isEnabled = e->kaleidoscope.enabled; break;
                    case TRANSFORM_INFINITE_ZOOM:     isEnabled = e->infiniteZoom.enabled; break;
                    case TRANSFORM_RADIAL_STREAK:     isEnabled = e->radialStreak.enabled; break;
                    case TRANSFORM_TEXTURE_WARP:      isEnabled = e->textureWarp.enabled; break;
                    case TRANSFORM_VORONOI:           isEnabled = e->voronoi.enabled; break;
                    case TRANSFORM_WAVE_RIPPLE:       isEnabled = e->waveRipple.enabled; break;
                    case TRANSFORM_MOBIUS:            isEnabled = e->mobius.enabled; break;
                    case TRANSFORM_PIXELATION:        isEnabled = e->pixelation.enabled; break;
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

    // -------------------------------------------------------------------------
    // SYMMETRY
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    DrawCategoryHeader("Symmetry", Theme::GLOW_CYAN);
    if (DrawSectionBegin("Kaleidoscope", Theme::GLOW_CYAN, &sectionKaleidoscope)) {
        ImGui::Checkbox("Enabled##kaleido", &e->kaleidoscope.enabled);
        if (e->kaleidoscope.enabled) {
            KaleidoscopeConfig* k = &e->kaleidoscope;

            ImGui::SliderInt("Segments", &k->segments, 1, 12);
            ModulatableSliderAngleDeg("Spin", &k->rotationSpeed,
                                      "kaleidoscope.rotationSpeed", modSources, "%.2f °/f");
            ModulatableSliderAngleDeg("Twist##kaleido", &k->twistAngle,
                                      "kaleidoscope.twistAngle", modSources, "%.1f °");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Techniques");
            ImGui::Spacing();

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

            const bool polarActive = TechniqueToggle("Polar", &k->polarIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool kifsActive = TechniqueToggle("KIFS", &k->kifsIntensity, Theme::ACCENT_MAGENTA_U32);

            const bool iterActive = TechniqueToggle("Mirror", &k->iterMirrorIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool hexActive = TechniqueToggle("Hex", &k->hexFoldIntensity, Theme::ACCENT_MAGENTA_U32);

            const int activeCount = (polarActive ? 1 : 0) + (kifsActive ? 1 : 0) +
                                    (iterActive ? 1 : 0) + (hexActive ? 1 : 0);

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

    // -------------------------------------------------------------------------
    // WARP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    DrawCategoryHeader("Warp", Theme::GLOW_MAGENTA);
    if (DrawSectionBegin("Sine Warp", Theme::GLOW_MAGENTA, &sectionSineWarp)) {
        ImGui::Checkbox("Enabled##sineWarp", &e->sineWarp.enabled);
        if (e->sineWarp.enabled) {
            ImGui::SliderInt("Octaves##sineWarp", &e->sineWarp.octaves, 1, 8);
            ModulatableSlider("Strength##sineWarp", &e->sineWarp.strength,
                              "sineWarp.strength", "%.2f", modSources);
            ImGui::SliderFloat("Anim Speed##sineWarp", &e->sineWarp.animSpeed, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Octave Rotation##sineWarp", &e->sineWarp.octaveRotation,
                                      "sineWarp.octaveRotation", modSources);
            ImGui::SliderFloat("UV Scale##sineWarp", &e->sineWarp.uvScale, 0.2f, 1.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Texture Warp", Theme::GLOW_MAGENTA, &sectionTextureWarp)) {
        ImGui::Checkbox("Enabled##texwarp", &e->textureWarp.enabled);
        if (e->textureWarp.enabled) {
            ModulatableSlider("Strength##texwarp", &e->textureWarp.strength,
                              "textureWarp.strength", "%.3f", modSources);
            ImGui::SliderInt("Iterations##texwarp", &e->textureWarp.iterations, 1, 8);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Wave Ripple", Theme::GLOW_MAGENTA, &sectionWaveRipple)) {
        ImGui::Checkbox("Enabled##waveripple", &e->waveRipple.enabled);
        if (e->waveRipple.enabled) {
            ImGui::SliderInt("Octaves##waveripple", &e->waveRipple.octaves, 1, 4);
            ModulatableSlider("Strength##waveripple", &e->waveRipple.strength,
                              "waveRipple.strength", "%.3f", modSources);
            ImGui::SliderFloat("Anim Speed##waveripple", &e->waveRipple.animSpeed, 0.0f, 5.0f, "%.2f");
            ModulatableSlider("Frequency##waveripple", &e->waveRipple.frequency,
                              "waveRipple.frequency", "%.1f", modSources);
            ModulatableSlider("Steepness##waveripple", &e->waveRipple.steepness,
                              "waveRipple.steepness", "%.2f", modSources);
            if (ImGui::TreeNode("Origin##waveripple")) {
                ModulatableSlider("X##waveripple", &e->waveRipple.originX,
                                  "waveRipple.originX", "%.2f", modSources);
                ModulatableSlider("Y##waveripple", &e->waveRipple.originY,
                                  "waveRipple.originY", "%.2f", modSources);
                ImGui::SliderFloat("Amplitude##waveripple", &e->waveRipple.originAmplitude, 0.0f, 0.3f, "%.3f");
                if (e->waveRipple.originAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq X##waveripple", &e->waveRipple.originFreqX, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq Y##waveripple", &e->waveRipple.originFreqY, 0.1f, 5.0f, "%.2f");
                }
                ImGui::TreePop();
            }
            ImGui::Checkbox("Shading##waveripple", &e->waveRipple.shadeEnabled);
            if (e->waveRipple.shadeEnabled) {
                ModulatableSlider("Shade Intensity##waveripple", &e->waveRipple.shadeIntensity,
                                  "waveRipple.shadeIntensity", "%.2f", modSources);
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Mobius", Theme::GLOW_MAGENTA, &sectionMobius)) {
        ImGui::Checkbox("Enabled##mobius", &e->mobius.enabled);
        if (e->mobius.enabled) {
            ModulatableSlider("Spiral Tightness##mobius", &e->mobius.spiralTightness,
                              "mobius.spiralTightness", "%.2f", modSources);
            ModulatableSlider("Zoom Factor##mobius", &e->mobius.zoomFactor,
                              "mobius.zoomFactor", "%.2f", modSources);
            ImGui::SliderFloat("Anim Speed##mobius", &e->mobius.animSpeed, 0.0f, 2.0f, "%.2f");
            if (ImGui::TreeNode("Fixed Points##mobius")) {
                ModulatableSlider("Point 1 X##mobius", &e->mobius.point1X,
                                  "mobius.point1X", "%.2f", modSources);
                ModulatableSlider("Point 1 Y##mobius", &e->mobius.point1Y,
                                  "mobius.point1Y", "%.2f", modSources);
                ModulatableSlider("Point 2 X##mobius", &e->mobius.point2X,
                                  "mobius.point2X", "%.2f", modSources);
                ModulatableSlider("Point 2 Y##mobius", &e->mobius.point2Y,
                                  "mobius.point2Y", "%.2f", modSources);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Point Motion##mobius")) {
                ImGui::SliderFloat("Amplitude##mobius", &e->mobius.pointAmplitude, 0.0f, 0.3f, "%.3f");
                if (e->mobius.pointAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq 1##mobius", &e->mobius.pointFreq1, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq 2##mobius", &e->mobius.pointFreq2, 0.1f, 5.0f, "%.2f");
                }
                ImGui::TreePop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Voronoi", Theme::GLOW_MAGENTA, &sectionVoronoi)) {
        ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
        if (e->voronoi.enabled) {
            VoronoiConfig* v = &e->voronoi;

            ModulatableSlider("Scale##vor", &v->scale, "voronoi.scale", "%.1f", modSources);
            ModulatableSlider("Speed##vor", &v->speed, "voronoi.speed", "%.2f", modSources);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Effects");
            ImGui::Spacing();

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

            const bool uvDistortActive = EffectToggle("Distort", &v->uvDistortIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeIsoActive = EffectToggle("Edge Iso", &v->edgeIsoIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool centerIsoActive = EffectToggle("Ctr Iso", &v->centerIsoIntensity, Theme::ACCENT_ORANGE_U32);

            const bool flatFillActive = EffectToggle("Fill", &v->flatFillIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeDarkenActive = EffectToggle("Darken", &v->edgeDarkenIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool angleShadeActive = EffectToggle("Angle", &v->angleShadeIntensity, Theme::ACCENT_ORANGE_U32);

            const bool determinantActive = EffectToggle("Determ", &v->determinantIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool ratioActive = EffectToggle("Ratio", &v->ratioIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeDetectActive = EffectToggle("Detect", &v->edgeDetectIntensity, Theme::ACCENT_ORANGE_U32);

            const int activeCount = (uvDistortActive ? 1 : 0) + (edgeIsoActive ? 1 : 0) + (centerIsoActive ? 1 : 0) +
                                    (flatFillActive ? 1 : 0) + (edgeDarkenActive ? 1 : 0) + (angleShadeActive ? 1 : 0) +
                                    (determinantActive ? 1 : 0) + (ratioActive ? 1 : 0) + (edgeDetectActive ? 1 : 0);

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

    // -------------------------------------------------------------------------
    // MOTION
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    DrawCategoryHeader("Motion", Theme::GLOW_ORANGE);
    if (DrawSectionBegin("Infinite Zoom", Theme::GLOW_ORANGE, &sectionInfiniteZoom)) {
        ImGui::Checkbox("Enabled##infzoom", &e->infiniteZoom.enabled);
        if (e->infiniteZoom.enabled) {
            ImGui::SliderFloat("Speed##infzoom", &e->infiniteZoom.speed, -2.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Zoom Depth##infzoom", &e->infiniteZoom.zoomDepth, 1.0f, 5.0f, "%.1f");
            ImGui::SliderInt("Layers##infzoom", &e->infiniteZoom.layers, 2, 8);
            ModulatableSliderAngleDeg("Spiral Angle##infzoom", &e->infiniteZoom.spiralAngle,
                                      "infiniteZoom.spiralAngle", modSources);
            ModulatableSliderAngleDeg("Twist##infzoom", &e->infiniteZoom.spiralTwist,
                                      "infiniteZoom.spiralTwist", modSources);
            ModulatableSliderInt("Droste Shear##infzoom", &e->infiniteZoom.drosteShear,
                                 "infiniteZoom.drosteShear", modSources);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Radial Blur", Theme::GLOW_ORANGE, &sectionRadialStreak)) {
        ImGui::Checkbox("Enabled##streak", &e->radialStreak.enabled);
        if (e->radialStreak.enabled) {
            ImGui::SliderInt("Samples##streak", &e->radialStreak.samples, 8, 32);
            ImGui::SliderFloat("Streak Length##streak", &e->radialStreak.streakLength, 0.1f, 1.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    // -------------------------------------------------------------------------
    // STYLE
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    DrawCategoryHeader("Style", Theme::GLOW_CYAN);
    if (DrawSectionBegin("Pixelation", Theme::GLOW_CYAN, &sectionPixelation)) {
        ImGui::Checkbox("Enabled##pixel", &e->pixelation.enabled);
        if (e->pixelation.enabled) {
            ModulatableSlider("Cell Count##pixel", &e->pixelation.cellCount,
                              "pixelation.cellCount", "%.0f", modSources);
            ImGui::SliderInt("Posterize##pixel", &e->pixelation.posterizeLevels, 0, 16);
            if (e->pixelation.posterizeLevels > 0) {
                ModulatableSliderInt("Dither Scale##pixel", &e->pixelation.ditherScale,
                                     "pixelation.ditherScale", modSources);
            }
        }
        DrawSectionEnd();
    }

    ImGui::End();
}
