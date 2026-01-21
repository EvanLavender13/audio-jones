#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/kaleidoscope_config.h"
#include "config/kifs_config.h"
#include "config/lattice_fold_config.h"
#include "config/voronoi_config.h"
#include "config/false_color_config.h"
#include "config/halftone_config.h"
#include "config/cross_hatching_config.h"
#include "config/palette_quantization_config.h"
#include "config/bokeh_config.h"
#include "config/bloom_config.h"
#include "config/mandelbox_config.h"
#include "config/triangle_fold_config.h"
#include "config/domain_warp_config.h"
#include "config/phyllotaxis_config.h"
#include "config/phyllotaxis_warp_config.h"
#include "config/moire_interference_config.h"
#include "automation/mod_sources.h"

// Persistent section open states for transform categories
static bool sectionKaleidoscope = false;
static bool sectionKifs = false;
static bool sectionLatticeFold = false;
static bool sectionPoincareDisk = false;
static bool sectionRadialPulse = false;
static bool sectionSineWarp = false;
static bool sectionTextureWarp = false;
static bool sectionGradientFlow = false;
static bool sectionWaveRipple = false;
static bool sectionMobius = false;
static bool sectionVoronoi = false;
static bool sectionInfiniteZoom = false;
static bool sectionRadialStreak = false;
static bool sectionDrosteZoom = false;
static bool sectionColorGrade = false;
static bool sectionFalseColor = false;
static bool sectionHalftone = false;
static bool sectionChladniWarp = false;
static bool sectionPaletteQuantization = false;
static bool sectionMandelbox = false;
static bool sectionTriangleFold = false;
static bool sectionDomainWarp = false;
static bool sectionPhyllotaxis = false;
static bool sectionPhyllotaxisWarp = false;
static bool sectionDensityWaveSpiral = false;
static bool sectionMoireInterference = false;

static void DrawSymmetryKaleidoscope(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Kaleidoscope", categoryGlow, &sectionKaleidoscope)) {
        const bool wasEnabled = e->kaleidoscope.enabled;
        ImGui::Checkbox("Enabled##kaleido", &e->kaleidoscope.enabled);
        if (!wasEnabled && e->kaleidoscope.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_KALEIDOSCOPE); }
        if (e->kaleidoscope.enabled) {
            KaleidoscopeConfig* k = &e->kaleidoscope;

            ImGui::SliderInt("Segments", &k->segments, 1, 12);
            ModulatableSliderAngleDeg("Spin", &k->rotationSpeed,
                                      "kaleidoscope.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Twist##kaleido", &k->twistAngle,
                                      "kaleidoscope.twistAngle", modSources, "%.1f °");
            ModulatableSlider("Smoothing##kaleido", &k->smoothing,
                              "kaleidoscope.smoothing", "%.2f", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawSymmetryKifs(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("KIFS", categoryGlow, &sectionKifs)) {
        const bool wasEnabled = e->kifs.enabled;
        ImGui::Checkbox("Enabled##kifs", &e->kifs.enabled);
        if (!wasEnabled && e->kifs.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_KIFS); }
        if (e->kifs.enabled) {
            KifsConfig* k = &e->kifs;

            ImGui::SliderInt("Iterations##kifs", &k->iterations, 1, 6);
            ImGui::SliderFloat("Scale##kifs", &k->scale, 1.5f, 2.5f, "%.2f");
            ImGui::SliderFloat("Offset X##kifs", &k->offsetX, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Offset Y##kifs", &k->offsetY, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Spin##kifs", &k->rotationSpeed,
                                      "kifs.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Twist##kifs", &k->twistSpeed,
                                      "kifs.twistSpeed", modSources, "%.1f °/s");
            ImGui::Checkbox("Octant Fold##kifs", &k->octantFold);
            ImGui::Checkbox("Polar Fold##kifs", &k->polarFold);
            if (k->polarFold) {
                ImGui::SliderInt("Segments##kifsPolar", &k->polarFoldSegments, 2, 12);
                ModulatableSlider("Smoothing##kifsPolar", &k->polarFoldSmoothing,
                                  "kifs.polarFoldSmoothing", "%.2f", modSources);
            }
        }
        DrawSectionEnd();
    }
}

static void DrawSymmetryPoincare(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Poincare Disk", categoryGlow, &sectionPoincareDisk)) {
        const bool wasEnabled = e->poincareDisk.enabled;
        ImGui::Checkbox("Enabled##poincare", &e->poincareDisk.enabled);
        if (!wasEnabled && e->poincareDisk.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_POINCARE_DISK); }
        if (e->poincareDisk.enabled) {
            PoincareDiskConfig* pd = &e->poincareDisk;

            ImGui::SliderInt("Tile P##poincare", &pd->tileP, 2, 12);
            ImGui::SliderInt("Tile Q##poincare", &pd->tileQ, 2, 12);
            ImGui::SliderInt("Tile R##poincare", &pd->tileR, 2, 12);

            ModulatableSlider("Translation X##poincare", &pd->translationX,
                              "poincareDisk.translationX", "%.2f", modSources);
            ModulatableSlider("Translation Y##poincare", &pd->translationY,
                              "poincareDisk.translationY", "%.2f", modSources);
            ModulatableSlider("Disk Scale##poincare", &pd->diskScale,
                              "poincareDisk.diskScale", "%.2f", modSources);

            ModulatableSlider("Motion Radius##poincare", &pd->translationAmplitude,
                              "poincareDisk.translationAmplitude", "%.2f", modSources);
            ModulatableSliderAngleDeg("Motion Speed##poincare", &pd->translationSpeed,
                                      "poincareDisk.translationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Rotation Speed##poincare", &pd->rotationSpeed,
                                      "poincareDisk.rotationSpeed", modSources, "%.1f °/s");
        }
        DrawSectionEnd();
    }
}

static void DrawSymmetryRadialPulse(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Radial Pulse", categoryGlow, &sectionRadialPulse)) {
        const bool wasEnabled = e->radialPulse.enabled;
        ImGui::Checkbox("Enabled##radpulse", &e->radialPulse.enabled);
        if (!wasEnabled && e->radialPulse.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_RADIAL_PULSE); }
        if (e->radialPulse.enabled) {
            RadialPulseConfig* rp = &e->radialPulse;

            ModulatableSlider("Radial Freq##radpulse", &rp->radialFreq,
                              "radialPulse.radialFreq", "%.1f", modSources);
            ModulatableSlider("Radial Amp##radpulse", &rp->radialAmp,
                              "radialPulse.radialAmp", "%.3f", modSources);
            ImGui::SliderInt("Segments##radpulse", &rp->segments, 2, 16);
            ModulatableSlider("Swirl##radpulse", &rp->angularAmp,
                              "radialPulse.angularAmp", "%.3f", modSources);
            ModulatableSlider("Petal##radpulse", &rp->petalAmp,
                              "radialPulse.petalAmp", "%.2f", modSources);
            ImGui::SliderFloat("Phase Speed##radpulse", &rp->phaseSpeed, -5.0f, 5.0f, "%.2f");
            ModulatableSliderAngleDeg("Spiral Twist##radpulse", &rp->spiralTwist,
                                      "radialPulse.spiralTwist", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawSymmetryMandelbox(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Mandelbox", categoryGlow, &sectionMandelbox)) {
        const bool wasEnabled = e->mandelbox.enabled;
        ImGui::Checkbox("Enabled##mandelbox", &e->mandelbox.enabled);
        if (!wasEnabled && e->mandelbox.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_MANDELBOX); }
        if (e->mandelbox.enabled) {
            MandelboxConfig* m = &e->mandelbox;

            ImGui::SliderInt("Iterations##mandelbox", &m->iterations, 1, 6);
            ImGui::SliderFloat("Scale##mandelbox", &m->scale, 1.5f, 2.5f, "%.2f");
            ImGui::SliderFloat("Offset X##mandelbox", &m->offsetX, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Offset Y##mandelbox", &m->offsetY, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Spin##mandelbox", &m->rotationSpeed,
                                      "mandelbox.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Twist##mandelbox", &m->twistSpeed,
                                      "mandelbox.twistSpeed", modSources, "%.1f °/s");

            if (TreeNodeAccented("Box Fold##mandelbox", categoryGlow)) {
                ImGui::SliderFloat("Limit##boxfold", &m->boxLimit, 0.5f, 2.0f, "%.2f");
                ModulatableSlider("Intensity##boxfold", &m->boxIntensity,
                                  "mandelbox.boxIntensity", "%.2f", modSources);
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Sphere Fold##mandelbox", categoryGlow)) {
                ImGui::SliderFloat("Min Radius##spherefold", &m->sphereMin, 0.1f, 0.5f, "%.2f");
                ImGui::SliderFloat("Max Radius##spherefold", &m->sphereMax, 0.5f, 2.0f, "%.2f");
                ModulatableSlider("Intensity##spherefold", &m->sphereIntensity,
                                  "mandelbox.sphereIntensity", "%.2f", modSources);
                TreeNodeAccentedPop();
            }

            ImGui::Checkbox("Polar Fold##mandelbox", &m->polarFold);
            if (m->polarFold) {
                ImGui::SliderInt("Segments##mandelboxPolar", &m->polarFoldSegments, 2, 12);
            }
        }
        DrawSectionEnd();
    }
}

static void DrawSymmetryTriangleFold(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Triangle Fold", categoryGlow, &sectionTriangleFold)) {
        const bool wasEnabled = e->triangleFold.enabled;
        ImGui::Checkbox("Enabled##trianglefold", &e->triangleFold.enabled);
        if (!wasEnabled && e->triangleFold.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_TRIANGLE_FOLD); }
        if (e->triangleFold.enabled) {
            TriangleFoldConfig* t = &e->triangleFold;

            ImGui::SliderInt("Iterations##trianglefold", &t->iterations, 1, 6);
            ImGui::SliderFloat("Scale##trianglefold", &t->scale, 1.5f, 2.5f, "%.2f");
            ImGui::SliderFloat("Offset X##trianglefold", &t->offsetX, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Offset Y##trianglefold", &t->offsetY, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Spin##trianglefold", &t->rotationSpeed,
                                      "triangleFold.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Twist##trianglefold", &t->twistSpeed,
                                      "triangleFold.twistSpeed", modSources, "%.1f °/s");
        }
        DrawSectionEnd();
    }
}

static void DrawSymmetryMoireInterference(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Moire Interference", categoryGlow, &sectionMoireInterference)) {
        const bool wasEnabled = e->moireInterference.enabled;
        ImGui::Checkbox("Enabled##moire", &e->moireInterference.enabled);
        if (!wasEnabled && e->moireInterference.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOIRE_INTERFERENCE); }
        if (e->moireInterference.enabled) {
            MoireInterferenceConfig* mi = &e->moireInterference;

            ModulatableSliderAngleDeg("Rotation##moire", &mi->rotationAngle,
                                      "moireInterference.rotationAngle", modSources, "%.1f °");
            ModulatableSlider("Scale Diff##moire", &mi->scaleDiff,
                              "moireInterference.scaleDiff", "%.3f", modSources);
            ImGui::SliderInt("Layers##moire", &mi->layers, 2, 4);
            ImGui::Combo("Blend Mode##moire", &mi->blendMode, "Multiply\0Min\0Average\0Difference\0");
            ModulatableSliderAngleDeg("Spin##moire", &mi->animationSpeed,
                                      "moireInterference.animationSpeed", modSources, "%.1f °/s");
            if (TreeNodeAccented("Center##moire", categoryGlow)) {
                ImGui::SliderFloat("X##moirecenter", &mi->centerX, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Y##moirecenter", &mi->centerY, 0.0f, 1.0f, "%.2f");
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

void DrawSymmetryCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(0);
    DrawCategoryHeader("Symmetry", categoryGlow);
    DrawSymmetryKaleidoscope(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawSymmetryKifs(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawSymmetryMoireInterference(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawSymmetryPoincare(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawSymmetryRadialPulse(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawSymmetryMandelbox(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawSymmetryTriangleFold(e, modSources, categoryGlow);
}

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
            ImGui::SliderFloat("Anim Rate##sineWarp", &e->sineWarp.animRate, 0.0f, 2.0f, "%.2f rad/s");
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
            ImGui::SliderInt("Iterations##gradflow", &e->gradientFlow.iterations, 1, 32);
            ModulatableSliderAngleDeg("Flow Angle##gradflow", &e->gradientFlow.flowAngle,
                                      "gradientFlow.flowAngle", modSources);
            ModulatableSlider("Edge Weight##gradflow", &e->gradientFlow.edgeWeight,
                              "gradientFlow.edgeWeight", "%.2f", modSources);
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

static void DrawMotionInfiniteZoom(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Infinite Zoom", categoryGlow, &sectionInfiniteZoom)) {
        const bool wasEnabled = e->infiniteZoom.enabled;
        ImGui::Checkbox("Enabled##infzoom", &e->infiniteZoom.enabled);
        if (!wasEnabled && e->infiniteZoom.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_INFINITE_ZOOM); }
        if (e->infiniteZoom.enabled) {
            ImGui::SliderFloat("Speed##infzoom", &e->infiniteZoom.speed, -2.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Zoom Depth##infzoom", &e->infiniteZoom.zoomDepth, 1.0f, 5.0f, "%.1f");
            ImGui::SliderInt("Layers##infzoom", &e->infiniteZoom.layers, 2, 8);
            ModulatableSliderAngleDeg("Spiral Angle##infzoom", &e->infiniteZoom.spiralAngle,
                                      "infiniteZoom.spiralAngle", modSources);
            ModulatableSliderAngleDeg("Twist##infzoom", &e->infiniteZoom.spiralTwist,
                                      "infiniteZoom.spiralTwist", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawMotionRadialBlur(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    (void)modSources;
    if (DrawSectionBegin("Radial Blur", categoryGlow, &sectionRadialStreak)) {
        const bool wasEnabled = e->radialStreak.enabled;
        ImGui::Checkbox("Enabled##streak", &e->radialStreak.enabled);
        if (!wasEnabled && e->radialStreak.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_RADIAL_STREAK); }
        if (e->radialStreak.enabled) {
            ImGui::SliderInt("Samples##streak", &e->radialStreak.samples, 8, 32);
            ImGui::SliderFloat("Streak Length##streak", &e->radialStreak.streakLength, 0.1f, 1.0f, "%.2f");
        }
        DrawSectionEnd();
    }
}

static void DrawMotionDroste(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Droste Zoom", categoryGlow, &sectionDrosteZoom)) {
        const bool wasEnabled = e->drosteZoom.enabled;
        ImGui::Checkbox("Enabled##droste", &e->drosteZoom.enabled);
        if (!wasEnabled && e->drosteZoom.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_DROSTE_ZOOM); }
        if (e->drosteZoom.enabled) {
            ImGui::SliderFloat("Speed##droste", &e->drosteZoom.speed, -2.0f, 2.0f, "%.2f");
            ModulatableSlider("Scale##droste", &e->drosteZoom.scale,
                              "drosteZoom.scale", "%.1f", modSources);
            ModulatableSliderAngleDeg("Spiral Angle##droste", &e->drosteZoom.spiralAngle,
                                      "drosteZoom.spiralAngle", modSources);
            ModulatableSlider("Shear##droste", &e->drosteZoom.shearCoeff,
                              "drosteZoom.shearCoeff", "%.2f", modSources);
            if (TreeNodeAccented("Masking##droste", categoryGlow)) {
                ModulatableSlider("Inner Radius##droste", &e->drosteZoom.innerRadius,
                                  "drosteZoom.innerRadius", "%.2f", modSources);
                TreeNodeAccentedPop();
            }
            if (TreeNodeAccented("Spiral##droste", categoryGlow)) {
                ImGui::SliderInt("Branches##droste", &e->drosteZoom.branches, 1, 8);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawMotionDensityWaveSpiral(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Density Wave Spiral", categoryGlow, &sectionDensityWaveSpiral)) {
        const bool wasEnabled = e->densityWaveSpiral.enabled;
        ImGui::Checkbox("Enabled##dws", &e->densityWaveSpiral.enabled);
        if (!wasEnabled && e->densityWaveSpiral.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_DENSITY_WAVE_SPIRAL); }
        if (e->densityWaveSpiral.enabled) {
            DensityWaveSpiralConfig* dws = &e->densityWaveSpiral;
            if (TreeNodeAccented("Center##dws", categoryGlow)) {
                ImGui::SliderFloat("X##dwscenter", &dws->centerX, -0.5f, 0.5f, "%.2f");
                ImGui::SliderFloat("Y##dwscenter", &dws->centerY, -0.5f, 0.5f, "%.2f");
                TreeNodeAccentedPop();
            }
            if (TreeNodeAccented("Aspect##dws", categoryGlow)) {
                ImGui::SliderFloat("X##dwsaspect", &dws->aspectX, 0.1f, 1.0f, "%.2f");
                ImGui::SliderFloat("Y##dwsaspect", &dws->aspectY, 0.1f, 1.0f, "%.2f");
                TreeNodeAccentedPop();
            }
            ModulatableSliderAngleDeg("Tightness##dws", &dws->tightness,
                                      "densityWaveSpiral.tightness", modSources);
            ModulatableSliderAngleDeg("Rotation Speed##dws", &dws->rotationSpeed,
                                      "densityWaveSpiral.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Global Rotation##dws", &dws->globalRotationSpeed,
                                      "densityWaveSpiral.globalRotationSpeed", modSources, "%.1f °/s");
            ModulatableSlider("Thickness##dws", &dws->thickness,
                              "densityWaveSpiral.thickness", "%.2f", modSources);
            ImGui::SliderInt("Ring Count##dws", &dws->ringCount, 10, 50);
            ImGui::SliderFloat("Falloff##dws", &dws->falloff, 0.5f, 2.0f, "%.2f");
        }
        DrawSectionEnd();
    }
}

void DrawMotionCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(3);
    DrawCategoryHeader("Motion", categoryGlow);
    DrawMotionInfiniteZoom(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawMotionRadialBlur(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawMotionDroste(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawMotionDensityWaveSpiral(e, modSources, categoryGlow);
}

static void DrawColorColorGrade(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Color Grade", categoryGlow, &sectionColorGrade)) {
        const bool wasEnabled = e->colorGrade.enabled;
        ImGui::Checkbox("Enabled##colorgrade", &e->colorGrade.enabled);
        if (!wasEnabled && e->colorGrade.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_COLOR_GRADE); }
        if (e->colorGrade.enabled) {
            ColorGradeConfig* cg = &e->colorGrade;

            ModulatableSlider("Hue Shift##colorgrade", &cg->hueShift,
                              "colorGrade.hueShift", "%.0f °", modSources, 360.0f);
            ModulatableSlider("Saturation##colorgrade", &cg->saturation,
                              "colorGrade.saturation", "%.2f", modSources);
            ModulatableSlider("Brightness##colorgrade", &cg->brightness,
                              "colorGrade.brightness", "%.2f F", modSources);
            ModulatableSlider("Contrast##colorgrade", &cg->contrast,
                              "colorGrade.contrast", "%.2f", modSources);
            ModulatableSlider("Temperature##colorgrade", &cg->temperature,
                              "colorGrade.temperature", "%.2f", modSources);

            if (TreeNodeAccented("Lift/Gamma/Gain##colorgrade", categoryGlow)) {
                ModulatableSlider("Shadows##colorgrade", &cg->shadowsOffset,
                                  "colorGrade.shadowsOffset", "%.2f", modSources);
                ModulatableSlider("Midtones##colorgrade", &cg->midtonesOffset,
                                  "colorGrade.midtonesOffset", "%.2f", modSources);
                ModulatableSlider("Highlights##colorgrade", &cg->highlightsOffset,
                                  "colorGrade.highlightsOffset", "%.2f", modSources);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawColorFalseColor(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("False Color", categoryGlow, &sectionFalseColor)) {
        const bool wasEnabled = e->falseColor.enabled;
        ImGui::Checkbox("Enabled##falsecolor", &e->falseColor.enabled);
        if (!wasEnabled && e->falseColor.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_FALSE_COLOR); }
        if (e->falseColor.enabled) {
            FalseColorConfig* fc = &e->falseColor;

            ImGuiDrawColorMode(&fc->gradient);

            ModulatableSlider("Intensity##falsecolor", &fc->intensity,
                              "falseColor.intensity", "%.2f", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawColorHalftone(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Halftone", categoryGlow, &sectionHalftone)) {
        const bool wasEnabled = e->halftone.enabled;
        ImGui::Checkbox("Enabled##halftone", &e->halftone.enabled);
        if (!wasEnabled && e->halftone.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_HALFTONE); }
        if (e->halftone.enabled) {
            HalftoneConfig* ht = &e->halftone;

            ModulatableSlider("Dot Scale##halftone", &ht->dotScale,
                              "halftone.dotScale", "%.1f px", modSources);
            ImGui::SliderFloat("Dot Size##halftone", &ht->dotSize, 0.5f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Spin##halftone", &ht->rotationSpeed,
                                      "halftone.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Angle##halftone", &ht->rotationAngle,
                                      "halftone.rotationAngle", modSources);
            ModulatableSlider("Threshold##halftone", &ht->threshold,
                              "halftone.threshold", "%.3f", modSources);
            ImGui::SliderFloat("Softness##halftone", &ht->softness, 0.1f, 0.5f, "%.3f");
        }
        DrawSectionEnd();
    }
}

static void DrawColorPaletteQuantization(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Palette Quantization", categoryGlow, &sectionPaletteQuantization)) {
        const bool wasEnabled = e->paletteQuantization.enabled;
        ImGui::Checkbox("Enabled##palettequant", &e->paletteQuantization.enabled);
        if (!wasEnabled && e->paletteQuantization.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_PALETTE_QUANTIZATION); }
        if (e->paletteQuantization.enabled) {
            PaletteQuantizationConfig* pq = &e->paletteQuantization;

            ModulatableSlider("Color Levels##palettequant", &pq->colorLevels,
                              "paletteQuantization.colorLevels", "%.0f", modSources);
            ModulatableSlider("Dither##palettequant", &pq->ditherStrength,
                              "paletteQuantization.ditherStrength", "%.2f", modSources);

            const char* bayerSizeNames[] = { "4x4 (Coarse)", "8x8 (Fine)" };
            int bayerIndex = (pq->bayerSize == 4) ? 0 : 1;
            if (ImGui::Combo("Pattern##palettequant", &bayerIndex, bayerSizeNames, 2)) {
                pq->bayerSize = (bayerIndex == 0) ? 4 : 8;
            }
        }
        DrawSectionEnd();
    }
}

void DrawColorCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(5);
    DrawCategoryHeader("Color", categoryGlow);
    DrawColorColorGrade(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawColorFalseColor(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawColorHalftone(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawColorPaletteQuantization(e, modSources, categoryGlow);
}

static void DrawCellularVoronoi(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Voronoi", categoryGlow, &sectionVoronoi)) {
        const bool wasEnabled = e->voronoi.enabled;
        ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
        if (!wasEnabled && e->voronoi.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_VORONOI); }
        if (e->voronoi.enabled) {
            VoronoiConfig* v = &e->voronoi;

            ModulatableSlider("Scale##vor", &v->scale, "voronoi.scale", "%.1f", modSources);
            ModulatableSlider("Speed##vor", &v->speed, "voronoi.speed", "%.2f", modSources);
            ImGui::Checkbox("Smooth##vor", &v->smoothMode);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Effects");
            ImGui::Spacing();

            const bool uvDistortActive = IntensityToggleButton("Distort", &v->uvDistortIntensity, "voronoi.uvDistortIntensity", Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeIsoActive = IntensityToggleButton("Edge Iso", &v->edgeIsoIntensity, "voronoi.edgeIsoIntensity", Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool centerIsoActive = IntensityToggleButton("Ctr Iso", &v->centerIsoIntensity, "voronoi.centerIsoIntensity", Theme::ACCENT_ORANGE_U32);

            const bool flatFillActive = IntensityToggleButton("Fill", &v->flatFillIntensity, "voronoi.flatFillIntensity", Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool organicFlowActive = IntensityToggleButton("Organic", &v->organicFlowIntensity, "voronoi.organicFlowIntensity", Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeGlowActive = IntensityToggleButton("Glow", &v->edgeGlowIntensity, "voronoi.edgeGlowIntensity", Theme::ACCENT_ORANGE_U32);

            const bool determinantActive = IntensityToggleButton("Determ", &v->determinantIntensity, "voronoi.determinantIntensity", Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool ratioActive = IntensityToggleButton("Ratio", &v->ratioIntensity, "voronoi.ratioIntensity", Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeDetectActive = IntensityToggleButton("Detect", &v->edgeDetectIntensity, "voronoi.edgeDetectIntensity", Theme::ACCENT_ORANGE_U32);

            const int activeCount = (uvDistortActive ? 1 : 0) + (edgeIsoActive ? 1 : 0) + (centerIsoActive ? 1 : 0) +
                                    (flatFillActive ? 1 : 0) + (organicFlowActive ? 1 : 0) + (edgeGlowActive ? 1 : 0) +
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
                if (organicFlowActive) {
                    ImGui::SliderFloat("Organic##mix", &v->organicFlowIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeGlowActive) {
                    ImGui::SliderFloat("Glow##mix", &v->edgeGlowIntensity, 0.01f, 1.0f, "%.2f");
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

            if (TreeNodeAccented("Iso Settings##vor", categoryGlow)) {
                ModulatableSlider("Frequency", &v->isoFrequency, "voronoi.isoFrequency", "%.1f", modSources);
                ModulatableSlider("Edge Falloff", &v->edgeFalloff, "voronoi.edgeFalloff", "%.2f", modSources);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawCellularLatticeFold(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Lattice Fold", categoryGlow, &sectionLatticeFold)) {
        const bool wasEnabled = e->latticeFold.enabled;
        ImGui::Checkbox("Enabled##lattice", &e->latticeFold.enabled);
        if (!wasEnabled && e->latticeFold.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_LATTICE_FOLD); }
        if (e->latticeFold.enabled) {
            LatticeFoldConfig* l = &e->latticeFold;

            const char* cellTypeNames[] = { "Square", "Hexagon" };
            int cellTypeIndex = (l->cellType == 4) ? 0 : 1;
            if (ImGui::Combo("Cell Type##lattice", &cellTypeIndex, cellTypeNames, 2)) {
                l->cellType = (cellTypeIndex == 0) ? 4 : 6;
            }
            ModulatableSlider("Cell Scale##lattice", &l->cellScale,
                              "latticeFold.cellScale", "%.1f", modSources);
            ModulatableSliderAngleDeg("Spin##lattice", &l->rotationSpeed,
                                      "latticeFold.rotationSpeed", modSources, "%.1f °/s");
            ModulatableSlider("Smoothing##lattice", &l->smoothing,
                              "latticeFold.smoothing", "%.2f", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawCellularPhyllotaxis(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Phyllotaxis", categoryGlow, &sectionPhyllotaxis)) {
        const bool wasEnabled = e->phyllotaxis.enabled;
        ImGui::Checkbox("Enabled##phyllo", &e->phyllotaxis.enabled);
        if (!wasEnabled && e->phyllotaxis.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_PHYLLOTAXIS); }
        if (e->phyllotaxis.enabled) {
            PhyllotaxisConfig* p = &e->phyllotaxis;

            ModulatableSlider("Scale##phyllo", &p->scale, "phyllotaxis.scale", "%.3f", modSources);
            ModulatableSliderAngleDeg("Angle Drift##phyllo", &p->angleSpeed,
                                      "phyllotaxis.angleSpeed", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Phase Pulse##phyllo", &p->phaseSpeed,
                                      "phyllotaxis.phaseSpeed", modSources, "%.1f °/s");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Effects");
            ImGui::Spacing();

            const bool uvDistortActive = IntensityToggleButton("Distort##phyllo", &p->uvDistortIntensity, "phyllotaxis.uvDistortIntensity", Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool organicFlowActive = IntensityToggleButton("Organic##phyllo", &p->organicFlowIntensity, "phyllotaxis.organicFlowIntensity", Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeIsoActive = IntensityToggleButton("Edge Iso##phyllo", &p->edgeIsoIntensity, "phyllotaxis.edgeIsoIntensity", Theme::ACCENT_ORANGE_U32);

            const bool centerIsoActive = IntensityToggleButton("Ctr Iso##phyllo", &p->centerIsoIntensity, "phyllotaxis.centerIsoIntensity", Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool flatFillActive = IntensityToggleButton("Fill##phyllo", &p->flatFillIntensity, "phyllotaxis.flatFillIntensity", Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeGlowActive = IntensityToggleButton("Glow##phyllo", &p->edgeGlowIntensity, "phyllotaxis.edgeGlowIntensity", Theme::ACCENT_ORANGE_U32);

            const bool ratioActive = IntensityToggleButton("Ratio##phyllo", &p->ratioIntensity, "phyllotaxis.ratioIntensity", Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool determinantActive = IntensityToggleButton("Determ##phyllo", &p->determinantIntensity, "phyllotaxis.determinantIntensity", Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeDetectActive = IntensityToggleButton("Detect##phyllo", &p->edgeDetectIntensity, "phyllotaxis.edgeDetectIntensity", Theme::ACCENT_ORANGE_U32);

            const int activeCount = (uvDistortActive ? 1 : 0) + (organicFlowActive ? 1 : 0) +
                                    (edgeIsoActive ? 1 : 0) + (centerIsoActive ? 1 : 0) +
                                    (flatFillActive ? 1 : 0) + (edgeGlowActive ? 1 : 0) +
                                    (ratioActive ? 1 : 0) + (determinantActive ? 1 : 0) + (edgeDetectActive ? 1 : 0);

            if (activeCount > 1) {
                ImGui::Spacing();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Blend Mix");
                if (uvDistortActive) {
                    ImGui::SliderFloat("Distort##phyllomix", &p->uvDistortIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (organicFlowActive) {
                    ImGui::SliderFloat("Organic##phyllomix", &p->organicFlowIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeIsoActive) {
                    ImGui::SliderFloat("Edge Iso##phyllomix", &p->edgeIsoIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (centerIsoActive) {
                    ImGui::SliderFloat("Ctr Iso##phyllomix", &p->centerIsoIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (flatFillActive) {
                    ImGui::SliderFloat("Fill##phyllomix", &p->flatFillIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeGlowActive) {
                    ImGui::SliderFloat("Glow##phyllomix", &p->edgeGlowIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (ratioActive) {
                    ImGui::SliderFloat("Ratio##phyllomix", &p->ratioIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (determinantActive) {
                    ImGui::SliderFloat("Determ##phyllomix", &p->determinantIntensity, 0.01f, 1.0f, "%.2f");
                }
                if (edgeDetectActive) {
                    ImGui::SliderFloat("Detect##phyllomix", &p->edgeDetectIntensity, 0.01f, 1.0f, "%.2f");
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (TreeNodeAccented("Iso Settings##phyllo", categoryGlow)) {
                ModulatableSlider("Frequency##phyllo", &p->isoFrequency, "phyllotaxis.isoFrequency", "%.1f", modSources);
                ModulatableSlider("Cell Radius##phyllo", &p->cellRadius, "phyllotaxis.cellRadius", "%.2f", modSources);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

void DrawCellularCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(2);
    DrawCategoryHeader("Cellular", categoryGlow);
    DrawCellularVoronoi(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawCellularLatticeFold(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawCellularPhyllotaxis(e, modSources, categoryGlow);
}
