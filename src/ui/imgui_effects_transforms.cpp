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
#include "automation/mod_sources.h"

// Persistent section open states for transform categories
static bool sectionKaleidoscope = false;
static bool sectionKifs = false;
static bool sectionLatticeFold = false;
static bool sectionPoincareDisk = false;
static bool sectionSineWarp = false;
static bool sectionTextureWarp = false;
static bool sectionGradientFlow = false;
static bool sectionWaveRipple = false;
static bool sectionMobius = false;
static bool sectionVoronoi = false;
static bool sectionInfiniteZoom = false;
static bool sectionRadialStreak = false;
static bool sectionPixelation = false;
static bool sectionGlitch = false;
static bool sectionToon = false;
static bool sectionOilPaint = false;
static bool sectionWatercolor = false;
static bool sectionHeightfieldRelief = false;
static bool sectionDrosteZoom = false;
static bool sectionColorGrade = false;
static bool sectionAsciiArt = false;

void DrawSymmetryCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(0);
    DrawCategoryHeader("Symmetry", categoryGlow);
    if (DrawSectionBegin("Kaleidoscope", categoryGlow, &sectionKaleidoscope)) {
        ImGui::Checkbox("Enabled##kaleido", &e->kaleidoscope.enabled);
        if (e->kaleidoscope.enabled) {
            KaleidoscopeConfig* k = &e->kaleidoscope;

            ImGui::SliderInt("Segments", &k->segments, 1, 12);
            ModulatableSliderAngleDeg("Spin", &k->rotationSpeed,
                                      "kaleidoscope.rotationSpeed", modSources, "%.2f °/f");
            ModulatableSliderAngleDeg("Twist##kaleido", &k->twistAngle,
                                      "kaleidoscope.twistAngle", modSources, "%.1f °");
            ModulatableSlider("Smoothing##kaleido", &k->smoothing,
                              "kaleidoscope.smoothing", "%.2f", modSources);

            if (TreeNodeAccented("Focal Offset##kaleido", categoryGlow)) {
                ImGui::SliderFloat("Amplitude", &k->focalAmplitude, 0.0f, 0.2f, "%.3f");
                if (k->focalAmplitude > 0.0f) {
                    ImGui::SliderFloat("Freq X", &k->focalFreqX, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Freq Y", &k->focalFreqY, 0.1f, 5.0f, "%.2f");
                }
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Warp##kaleido", categoryGlow)) {
                ImGui::SliderFloat("Strength", &k->warpStrength, 0.0f, 0.5f, "%.3f");
                if (k->warpStrength > 0.0f) {
                    ImGui::SliderFloat("Speed", &k->warpSpeed, 0.0f, 1.0f, "%.2f");
                    ImGui::SliderFloat("Scale", &k->noiseScale, 0.5f, 10.0f, "%.1f");
                }
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("KIFS", categoryGlow, &sectionKifs)) {
        ImGui::Checkbox("Enabled##kifs", &e->kifs.enabled);
        if (e->kifs.enabled) {
            KifsConfig* k = &e->kifs;

            ImGui::SliderInt("Iterations##kifs", &k->iterations, 1, 12);
            ImGui::SliderFloat("Scale##kifs", &k->scale, 1.5f, 4.0f, "%.2f");
            ImGui::SliderFloat("Offset X##kifs", &k->offsetX, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Offset Y##kifs", &k->offsetY, 0.0f, 2.0f, "%.2f");
            ModulatableSliderAngleDeg("Spin##kifs", &k->rotationSpeed,
                                      "kifs.rotationSpeed", modSources, "%.2f °/f");
            ModulatableSliderAngleDeg("Twist##kifs", &k->twistAngle,
                                      "kifs.twistAngle", modSources, "%.1f °");
            ImGui::Checkbox("Octant Fold##kifs", &k->octantFold);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Poincare Disk", categoryGlow, &sectionPoincareDisk)) {
        ImGui::Checkbox("Enabled##poincare", &e->poincareDisk.enabled);
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
                                      "poincareDisk.translationSpeed", modSources, "%.2f °/f");
            ModulatableSliderAngleDeg("Rotation Speed##poincare", &pd->rotationSpeed,
                                      "poincareDisk.rotationSpeed", modSources, "%.2f °/f");
        }
        DrawSectionEnd();
    }
}

// NOLINTNEXTLINE(readability-function-size) - UI panel for warp effects
void DrawWarpCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(1);
    DrawCategoryHeader("Warp", categoryGlow);
    if (DrawSectionBegin("Sine Warp", categoryGlow, &sectionSineWarp)) {
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

    if (DrawSectionBegin("Texture Warp", categoryGlow, &sectionTextureWarp)) {
        ImGui::Checkbox("Enabled##texwarp", &e->textureWarp.enabled);
        if (e->textureWarp.enabled) {
            const char* channelModeNames[] = { "RG", "RB", "GB", "Luminance", "LuminanceSplit", "Chrominance", "Polar" };
            int channelMode = (int)e->textureWarp.channelMode;
            if (ImGui::Combo("Channel Mode##texwarp", &channelMode, channelModeNames, 7)) {
                e->textureWarp.channelMode = (TextureWarpChannelMode)channelMode;
            }
            ModulatableSlider("Strength##texwarp", &e->textureWarp.strength,
                              "textureWarp.strength", "%.3f", modSources);
            ImGui::SliderInt("Iterations##texwarp", &e->textureWarp.iterations, 1, 8);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Gradient Flow", categoryGlow, &sectionGradientFlow)) {
        ImGui::Checkbox("Enabled##gradflow", &e->gradientFlow.enabled);
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

    ImGui::Spacing();

    if (DrawSectionBegin("Wave Ripple", categoryGlow, &sectionWaveRipple)) {
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

    ImGui::Spacing();

    if (DrawSectionBegin("Mobius", categoryGlow, &sectionMobius)) {
        ImGui::Checkbox("Enabled##mobius", &e->mobius.enabled);
        if (e->mobius.enabled) {
            ModulatableSlider("Spiral Tightness##mobius", &e->mobius.spiralTightness,
                              "mobius.spiralTightness", "%.2f", modSources);
            ModulatableSlider("Zoom Factor##mobius", &e->mobius.zoomFactor,
                              "mobius.zoomFactor", "%.2f", modSources);
            ImGui::SliderFloat("Anim Speed##mobius", &e->mobius.animSpeed, 0.0f, 2.0f, "%.2f");
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

void DrawMotionCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(3);
    DrawCategoryHeader("Motion", categoryGlow);
    if (DrawSectionBegin("Infinite Zoom", categoryGlow, &sectionInfiniteZoom)) {
        ImGui::Checkbox("Enabled##infzoom", &e->infiniteZoom.enabled);
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

    ImGui::Spacing();

    if (DrawSectionBegin("Radial Blur", categoryGlow, &sectionRadialStreak)) {
        ImGui::Checkbox("Enabled##streak", &e->radialStreak.enabled);
        if (e->radialStreak.enabled) {
            ImGui::SliderInt("Samples##streak", &e->radialStreak.samples, 8, 32);
            ImGui::SliderFloat("Streak Length##streak", &e->radialStreak.streakLength, 0.1f, 1.0f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Droste Zoom", categoryGlow, &sectionDrosteZoom)) {
        ImGui::Checkbox("Enabled##droste", &e->drosteZoom.enabled);
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

// NOLINTNEXTLINE(readability-function-size) - UI panel for style effects
void DrawStyleCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(4);
    DrawCategoryHeader("Style", categoryGlow);
    if (DrawSectionBegin("Pixelation", categoryGlow, &sectionPixelation)) {
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

    ImGui::Spacing();

    if (DrawSectionBegin("Glitch", categoryGlow, &sectionGlitch)) {
        ImGui::Checkbox("Enabled##glitch", &e->glitch.enabled);
        if (e->glitch.enabled) {
            GlitchConfig* g = &e->glitch;

            // CRT Mode
            if (TreeNodeAccented("CRT##glitch", categoryGlow)) {
                ImGui::Checkbox("Enabled##crt", &g->crtEnabled);
                if (g->crtEnabled) {
                    ImGui::SliderFloat("Curvature##crt", &g->curvature, 0.0f, 0.2f, "%.3f");
                    ImGui::Checkbox("Vignette##crt", &g->vignetteEnabled);
                }
                TreeNodeAccentedPop();
            }

            // Analog Mode (enabled when intensity > 0)
            if (TreeNodeAccented("Analog##glitch", categoryGlow)) {
                ModulatableSlider("Intensity##analog", &g->analogIntensity,
                                  "glitch.analogIntensity", "%.3f", modSources);
                ModulatableSlider("Aberration##analog", &g->aberration,
                                  "glitch.aberration", "%.1f px", modSources);
                TreeNodeAccentedPop();
            }

            // Digital Mode (enabled when blockThreshold > 0)
            if (TreeNodeAccented("Digital##glitch", categoryGlow)) {
                ModulatableSlider("Block Threshold##digital", &g->blockThreshold,
                                  "glitch.blockThreshold", "%.2f", modSources);
                ModulatableSlider("Block Offset##digital", &g->blockOffset,
                                  "glitch.blockOffset", "%.2f", modSources);
                TreeNodeAccentedPop();
            }

            // VHS Mode
            if (TreeNodeAccented("VHS##glitch", categoryGlow)) {
                ImGui::Checkbox("Enabled##vhs", &g->vhsEnabled);
                if (g->vhsEnabled) {
                    ImGui::SliderFloat("Tracking Bars##vhs", &g->trackingBarIntensity, 0.0f, 0.05f, "%.3f");
                    ImGui::SliderFloat("Scanline Noise##vhs", &g->scanlineNoiseIntensity, 0.0f, 0.02f, "%.4f");
                    ImGui::SliderFloat("Color Drift##vhs", &g->colorDriftIntensity, 0.0f, 2.0f, "%.2f");
                }
                TreeNodeAccentedPop();
            }

            // Overlay (always visible when glitch enabled)
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Overlay");
            ImGui::SliderFloat("Scanlines##glitch", &g->scanlineAmount, 0.0f, 0.5f, "%.2f");
            ImGui::SliderFloat("Noise##glitch", &g->noiseAmount, 0.0f, 0.3f, "%.2f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Toon", categoryGlow, &sectionToon)) {
        ImGui::Checkbox("Enabled##toon", &e->toon.enabled);
        if (e->toon.enabled) {
            ToonConfig* t = &e->toon;

            ImGui::SliderInt("Levels##toon", &t->levels, 2, 16);
            ImGui::SliderFloat("Edge Threshold##toon", &t->edgeThreshold, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Edge Softness##toon", &t->edgeSoftness, 0.0f, 0.2f, "%.3f");

            if (TreeNodeAccented("Brush Stroke##toon", categoryGlow)) {
                ImGui::SliderFloat("Thickness Variation##toon", &t->thicknessVariation, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Noise Scale##toon", &t->noiseScale, 1.0f, 20.0f, "%.1f");
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Oil Paint", categoryGlow, &sectionOilPaint)) {
        ImGui::Checkbox("Enabled##oilpaint", &e->oilPaint.enabled);
        if (e->oilPaint.enabled) {
            OilPaintConfig* op = &e->oilPaint;
            ModulatableSlider("Radius##oilpaint", &op->radius,
                              "oilPaint.radius", "%.0f", modSources);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Watercolor", categoryGlow, &sectionWatercolor)) {
        ImGui::Checkbox("Enabled##watercolor", &e->watercolor.enabled);
        if (e->watercolor.enabled) {
            WatercolorConfig* wc = &e->watercolor;
            ModulatableSlider("Edge Darkening##wc", &wc->edgeDarkening,
                              "watercolor.edgeDarkening", "%.2f", modSources);
            ModulatableSlider("Granulation##wc", &wc->granulationStrength,
                              "watercolor.granulationStrength", "%.2f", modSources);
            ImGui::SliderFloat("Paper Scale##wc", &wc->paperScale, 1.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Softness##wc", &wc->softness, 0.0f, 5.0f, "%.1f");
            ModulatableSlider("Bleed Strength##wc", &wc->bleedStrength,
                              "watercolor.bleedStrength", "%.2f", modSources);
            ImGui::SliderFloat("Bleed Radius##wc", &wc->bleedRadius, 1.0f, 10.0f, "%.1f");
            ImGui::SliderInt("Color Levels##wc", &wc->colorLevels, 0, 16);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Heightfield Relief", categoryGlow, &sectionHeightfieldRelief)) {
        ImGui::Checkbox("Enabled##relief", &e->heightfieldRelief.enabled);
        if (e->heightfieldRelief.enabled) {
            HeightfieldReliefConfig* h = &e->heightfieldRelief;

            ModulatableSlider("Intensity##relief", &h->intensity,
                              "heightfieldRelief.intensity", "%.2f", modSources);
            ImGui::SliderFloat("Relief Scale##relief", &h->reliefScale, 0.02f, 1.0f, "%.2f");
            ModulatableSliderAngleDeg("Light Angle##relief", &h->lightAngle,
                                      "heightfieldRelief.lightAngle", modSources);
            ImGui::SliderFloat("Light Height##relief", &h->lightHeight, 0.1f, 2.0f, "%.2f");
            ImGui::SliderFloat("Shininess##relief", &h->shininess, 1.0f, 128.0f, "%.0f");
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Color Grade", categoryGlow, &sectionColorGrade)) {
        ImGui::Checkbox("Enabled##colorgrade", &e->colorGrade.enabled);
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

    ImGui::Spacing();

    if (DrawSectionBegin("ASCII Art", categoryGlow, &sectionAsciiArt)) {
        ImGui::Checkbox("Enabled##ascii", &e->asciiArt.enabled);
        if (e->asciiArt.enabled) {
            AsciiArtConfig* aa = &e->asciiArt;

            ModulatableSlider("Cell Size##ascii", &aa->cellSize,
                              "asciiArt.cellSize", "%.0f px", modSources);

            const char* colorModeNames[] = { "Original", "Mono", "CRT Green" };
            ImGui::Combo("Color Mode##ascii", &aa->colorMode, colorModeNames, 3);

            if (aa->colorMode == 1) {
                float fg[3] = { aa->foregroundR, aa->foregroundG, aa->foregroundB };
                if (ImGui::ColorEdit3("Foreground##ascii", fg)) {
                    aa->foregroundR = fg[0];
                    aa->foregroundG = fg[1];
                    aa->foregroundB = fg[2];
                }
                float bg[3] = { aa->backgroundR, aa->backgroundG, aa->backgroundB };
                if (ImGui::ColorEdit3("Background##ascii", bg)) {
                    aa->backgroundR = bg[0];
                    aa->backgroundG = bg[1];
                    aa->backgroundB = bg[2];
                }
            }

            ImGui::Checkbox("Invert##ascii", &aa->invert);
        }
        DrawSectionEnd();
    }
}

// NOLINTNEXTLINE(readability-function-size) - UI panel for cellular/grid effects
void DrawCellularCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(2);
    DrawCategoryHeader("Cellular", categoryGlow);
    if (DrawSectionBegin("Voronoi", categoryGlow, &sectionVoronoi)) {
        ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);
        if (e->voronoi.enabled) {
            VoronoiConfig* v = &e->voronoi;

            ModulatableSlider("Scale##vor", &v->scale, "voronoi.scale", "%.1f", modSources);
            ModulatableSlider("Speed##vor", &v->speed, "voronoi.speed", "%.2f", modSources);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Effects");
            ImGui::Spacing();

            const bool uvDistortActive = IntensityToggleButton("Distort", &v->uvDistortIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeIsoActive = IntensityToggleButton("Edge Iso", &v->edgeIsoIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool centerIsoActive = IntensityToggleButton("Ctr Iso", &v->centerIsoIntensity, Theme::ACCENT_ORANGE_U32);

            const bool flatFillActive = IntensityToggleButton("Fill", &v->flatFillIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool edgeDarkenActive = IntensityToggleButton("Darken", &v->edgeDarkenIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool angleShadeActive = IntensityToggleButton("Angle", &v->angleShadeIntensity, Theme::ACCENT_ORANGE_U32);

            const bool determinantActive = IntensityToggleButton("Determ", &v->determinantIntensity, Theme::ACCENT_CYAN_U32);
            ImGui::SameLine();
            const bool ratioActive = IntensityToggleButton("Ratio", &v->ratioIntensity, Theme::ACCENT_MAGENTA_U32);
            ImGui::SameLine();
            const bool edgeDetectActive = IntensityToggleButton("Detect", &v->edgeDetectIntensity, Theme::ACCENT_ORANGE_U32);

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

            if (TreeNodeAccented("Iso Settings##vor", categoryGlow)) {
                ModulatableSlider("Frequency", &v->isoFrequency, "voronoi.isoFrequency", "%.1f", modSources);
                ModulatableSlider("Edge Falloff", &v->edgeFalloff, "voronoi.edgeFalloff", "%.2f", modSources);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Lattice Fold", categoryGlow, &sectionLatticeFold)) {
        ImGui::Checkbox("Enabled##lattice", &e->latticeFold.enabled);
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
                                      "latticeFold.rotationSpeed", modSources, "%.2f °/f");
        }
        DrawSectionEnd();
    }
}
