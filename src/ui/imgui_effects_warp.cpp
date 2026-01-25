#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/domain_warp_config.h"
#include "config/phyllotaxis_warp_config.h"
#include "automation/mod_sources.h"

static bool sectionSineWarp = false;
static bool sectionTextureWarp = false;
static bool sectionGradientFlow = false;
static bool sectionWaveRipple = false;
static bool sectionMobius = false;
static bool sectionChladniWarp = false;
static bool sectionDomainWarp = false;
static bool sectionPhyllotaxisWarp = false;

static void DrawWarpSine(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Sine Warp", categoryGlow, &sectionSineWarp)) {
        const bool wasEnabled = e->sineWarp.enabled;
        ImGui::Checkbox("Enabled##sineWarp", &e->sineWarp.enabled);
        if (!wasEnabled && e->sineWarp.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_SINE_WARP); }
        if (e->sineWarp.enabled) {
            ImGui::SliderInt("Octaves##sineWarp", &e->sineWarp.octaves, 1, 8);
            ModulatableSlider("Strength##sineWarp", &e->sineWarp.strength,
                              "sineWarp.strength", "%.2f", modSources);
            SliderAngleDeg("Anim Rate##sineWarp", &e->sineWarp.animRate, -180.0f, 180.0f, "%.1f °/s");
            ModulatableSliderAngleDeg("Octave Rotation##sineWarp", &e->sineWarp.octaveRotation,
                                      "sineWarp.octaveRotation", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawWarpTexture(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Texture Warp", categoryGlow, &sectionTextureWarp)) {
        const bool wasEnabled = e->textureWarp.enabled;
        ImGui::Checkbox("Enabled##texwarp", &e->textureWarp.enabled);
        if (!wasEnabled && e->textureWarp.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_TEXTURE_WARP); }
        if (e->textureWarp.enabled) {
            const char* channelModeNames[] = { "RG", "RB", "GB", "Luminance", "LuminanceSplit", "Chrominance", "Polar" };
            int channelMode = (int)e->textureWarp.channelMode;
            if (ImGui::Combo("Channel Mode##texwarp", &channelMode, channelModeNames, 7)) {
                e->textureWarp.channelMode = (TextureWarpChannelMode)channelMode;
            }
            ModulatableSlider("Strength##texwarp", &e->textureWarp.strength,
                              "textureWarp.strength", "%.3f", modSources);
            ImGui::SliderInt("Iterations##texwarp", &e->textureWarp.iterations, 1, 8);

            if (TreeNodeAccented("Directional##texwarp", categoryGlow)) {
                ModulatableSliderAngleDeg("Ridge Angle##texwarp", &e->textureWarp.ridgeAngle,
                                          "textureWarp.ridgeAngle", modSources);
                ModulatableSlider("Anisotropy##texwarp", &e->textureWarp.anisotropy,
                                  "textureWarp.anisotropy", "%.2f", modSources);
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Noise##texwarp", categoryGlow)) {
                ModulatableSlider("Noise Amount##texwarp", &e->textureWarp.noiseAmount,
                                  "textureWarp.noiseAmount", "%.2f", modSources);
                ImGui::SliderFloat("Noise Scale##texwarp", &e->textureWarp.noiseScale, 1.0f, 20.0f, "%.1f");
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawWarpGradientFlow(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Gradient Flow", categoryGlow, &sectionGradientFlow)) {
        const bool wasEnabled = e->gradientFlow.enabled;
        ImGui::Checkbox("Enabled##gradflow", &e->gradientFlow.enabled);
        if (!wasEnabled && e->gradientFlow.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_GRADIENT_FLOW); }
        if (e->gradientFlow.enabled) {
            ModulatableSlider("Strength##gradflow", &e->gradientFlow.strength,
                              "gradientFlow.strength", "%.3f", modSources);
            ImGui::SliderInt("Iterations##gradflow", &e->gradientFlow.iterations, 1, 8);
            ModulatableSlider("Edge Weight##gradflow", &e->gradientFlow.edgeWeight,
                              "gradientFlow.edgeWeight", "%.2f", modSources);
            ImGui::Checkbox("Random Direction##gradflow", &e->gradientFlow.randomDirection);
        }
        DrawSectionEnd();
    }
}

static void DrawWarpWaveRipple(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Wave Ripple", categoryGlow, &sectionWaveRipple)) {
        const bool wasEnabled = e->waveRipple.enabled;
        ImGui::Checkbox("Enabled##waveripple", &e->waveRipple.enabled);
        if (!wasEnabled && e->waveRipple.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_WAVE_RIPPLE); }
        if (e->waveRipple.enabled) {
            ImGui::SliderInt("Octaves##waveripple", &e->waveRipple.octaves, 1, 4);
            ModulatableSlider("Strength##waveripple", &e->waveRipple.strength,
                              "waveRipple.strength", "%.3f", modSources);
            ImGui::SliderFloat("Anim Rate##waveripple", &e->waveRipple.animRate, 0.0f, 5.0f, "%.2f rad/s");
            ModulatableSlider("Frequency##waveripple", &e->waveRipple.frequency,
                              "waveRipple.frequency", "%.1f", modSources);
            ModulatableSlider("Steepness##waveripple", &e->waveRipple.steepness,
                              "waveRipple.steepness", "%.2f", modSources);
            ModulatableSlider("Decay##waveripple", &e->waveRipple.decay,
                              "waveRipple.decay", "%.1f", modSources);
            ModulatableSlider("Center Hole##waveripple", &e->waveRipple.centerHole,
                              "waveRipple.centerHole", "%.2f", modSources);
            if (TreeNodeAccented("Origin##waveripple", categoryGlow)) {
                ModulatableSlider("X##waveripple", &e->waveRipple.originX,
                                  "waveRipple.originX", "%.2f", modSources);
                ModulatableSlider("Y##waveripple", &e->waveRipple.originY,
                                  "waveRipple.originY", "%.2f", modSources);
                ImGui::SliderFloat("Amplitude##waveripple", &e->waveRipple.originAmplitude, 0.0f, 0.3f, "%.3f");
                if (e->waveRipple.originAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq X##waveripple", &e->waveRipple.originFreqX, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq Y##waveripple", &e->waveRipple.originFreqY, 0.1f, 5.0f, "%.2f");
                }
                TreeNodeAccentedPop();
            }
            ImGui::Checkbox("Shading##waveripple", &e->waveRipple.shadeEnabled);
            if (e->waveRipple.shadeEnabled) {
                ModulatableSlider("Shade Intensity##waveripple", &e->waveRipple.shadeIntensity,
                                  "waveRipple.shadeIntensity", "%.2f", modSources);
            }
        }
        DrawSectionEnd();
    }
}

static void DrawWarpMobius(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Mobius", categoryGlow, &sectionMobius)) {
        const bool wasEnabled = e->mobius.enabled;
        ImGui::Checkbox("Enabled##mobius", &e->mobius.enabled);
        if (!wasEnabled && e->mobius.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOBIUS); }
        if (e->mobius.enabled) {
            ModulatableSlider("Spiral Tightness##mobius", &e->mobius.spiralTightness,
                              "mobius.spiralTightness", "%.2f", modSources);
            ModulatableSlider("Zoom Factor##mobius", &e->mobius.zoomFactor,
                              "mobius.zoomFactor", "%.2f", modSources);
            ModulatableSliderAngleDeg("Anim Rate##mobius", &e->mobius.animRate,
                                      "mobius.animRate", modSources, "%.1f °/s");
            if (TreeNodeAccented("Fixed Points##mobius", categoryGlow)) {
                ModulatableSlider("Point 1 X##mobius", &e->mobius.point1X,
                                  "mobius.point1X", "%.2f", modSources);
                ModulatableSlider("Point 1 Y##mobius", &e->mobius.point1Y,
                                  "mobius.point1Y", "%.2f", modSources);
                ModulatableSlider("Point 2 X##mobius", &e->mobius.point2X,
                                  "mobius.point2X", "%.2f", modSources);
                ModulatableSlider("Point 2 Y##mobius", &e->mobius.point2Y,
                                  "mobius.point2Y", "%.2f", modSources);
                TreeNodeAccentedPop();
            }
            if (TreeNodeAccented("Point Motion##mobius", categoryGlow)) {
                ImGui::SliderFloat("Amplitude##mobius", &e->mobius.pointAmplitude, 0.0f, 0.3f, "%.3f");
                if (e->mobius.pointAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq 1##mobius", &e->mobius.pointFreq1, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq 2##mobius", &e->mobius.pointFreq2, 0.1f, 5.0f, "%.2f");
                }
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawWarpChladniWarp(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Chladni Warp", categoryGlow, &sectionChladniWarp)) {
        const bool wasEnabled = e->chladniWarp.enabled;
        ImGui::Checkbox("Enabled##chladni", &e->chladniWarp.enabled);
        if (!wasEnabled && e->chladniWarp.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_CHLADNI_WARP); }
        if (e->chladniWarp.enabled) {
            ChladniWarpConfig* cw = &e->chladniWarp;

            ModulatableSlider("N (X Mode)##chladni", &cw->n,
                              "chladniWarp.n", "%.1f", modSources);
            ModulatableSlider("M (Y Mode)##chladni", &cw->m,
                              "chladniWarp.m", "%.1f", modSources);
            ImGui::SliderFloat("Plate Size##chladni", &cw->plateSize, 0.5f, 2.0f, "%.2f");
            ModulatableSlider("Strength##chladni", &cw->strength,
                              "chladniWarp.strength", "%.3f", modSources);

            const char* warpModeNames[] = { "Toward Nodes", "Along Nodes", "Intensity" };
            ImGui::Combo("Mode##chladni", &cw->warpMode, warpModeNames, 3);

            if (TreeNodeAccented("Animation##chladni", categoryGlow)) {
                ImGui::SliderFloat("Anim Rate##chladni", &cw->animRate, 0.0f, 2.0f, "%.2f rad/s");
                ModulatableSlider("Range##chladni", &cw->animRange,
                                  "chladniWarp.animRange", "%.1f", modSources);
                TreeNodeAccentedPop();
            }

            ImGui::Checkbox("Pre-Fold (Symmetry)##chladni", &cw->preFold);
        }
        DrawSectionEnd();
    }
}

static void DrawWarpDomainWarp(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Domain Warp", categoryGlow, &sectionDomainWarp)) {
        const bool wasEnabled = e->domainWarp.enabled;
        ImGui::Checkbox("Enabled##domainwarp", &e->domainWarp.enabled);
        if (!wasEnabled && e->domainWarp.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_DOMAIN_WARP); }
        if (e->domainWarp.enabled) {
            DomainWarpConfig* dw = &e->domainWarp;

            ModulatableSlider("Strength##domainwarp", &dw->warpStrength,
                              "domainWarp.warpStrength", "%.3f", modSources);
            ImGui::SliderFloat("Scale##domainwarp", &dw->warpScale, 1.0f, 10.0f, "%.1f");
            ImGui::SliderInt("Iterations##domainwarp", &dw->warpIterations, 1, 3);
            ModulatableSlider("Falloff##domainwarp", &dw->falloff,
                              "domainWarp.falloff", "%.2f", modSources);
            ModulatableSliderAngleDeg("Drift Speed##domainwarp", &dw->driftSpeed,
                                      "domainWarp.driftSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Drift Angle##domainwarp", &dw->driftAngle,
                                      "domainWarp.driftAngle", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawWarpPhyllotaxisWarp(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Phyllotaxis Warp", categoryGlow, &sectionPhyllotaxisWarp)) {
        const bool wasEnabled = e->phyllotaxisWarp.enabled;
        ImGui::Checkbox("Enabled##phyllowarp", &e->phyllotaxisWarp.enabled);
        if (!wasEnabled && e->phyllotaxisWarp.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_PHYLLOTAXIS_WARP); }
        if (e->phyllotaxisWarp.enabled) {
            PhyllotaxisWarpConfig* pw = &e->phyllotaxisWarp;

            ImGui::SliderFloat("Scale##phyllowarp", &pw->scale, 0.02f, 0.15f, "%.3f");
            SliderAngleDeg("Divergence Angle##phyllowarp", &pw->divergenceAngle, 57.0f, 200.0f, "%.1f °");
            ModulatableSlider("Warp Strength##phyllowarp", &pw->warpStrength,
                              "phyllotaxisWarp.warpStrength", "%.2f", modSources);
            ModulatableSlider("Warp Falloff##phyllowarp", &pw->warpFalloff,
                              "phyllotaxisWarp.warpFalloff", "%.1f", modSources);
            ModulatableSlider("Tangent Intensity##phyllowarp", &pw->tangentIntensity,
                              "phyllotaxisWarp.tangentIntensity", "%.2f", modSources);
            ModulatableSlider("Radial Intensity##phyllowarp", &pw->radialIntensity,
                              "phyllotaxisWarp.radialIntensity", "%.2f", modSources);
            ModulatableSliderAngleDeg("Spin Speed##phyllowarp", &pw->spinSpeed,
                                      "phyllotaxisWarp.spinSpeed", modSources, "%.1f °/s");
            ImGui::SliderFloat("Crawl Speed##phyllowarp", &pw->crawlSpeed, -10.0f, 10.0f, "%.2f idx/s");
        }
        DrawSectionEnd();
    }
}

void DrawWarpCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(1);
    DrawCategoryHeader("Warp", categoryGlow);
    DrawWarpSine(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpTexture(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpGradientFlow(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpWaveRipple(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpMobius(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpChladniWarp(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpDomainWarp(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawWarpPhyllotaxisWarp(e, modSources, categoryGlow);
}
