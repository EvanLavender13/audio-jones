#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/bokeh_config.h"
#include "config/bloom_config.h"
#include "automation/mod_sources.h"

static bool sectionPixelation = false;
static bool sectionGlitch = false;
static bool sectionToon = false;
static bool sectionOilPaint = false;
static bool sectionWatercolor = false;
static bool sectionNeonGlow = false;
static bool sectionHeightfieldRelief = false;
static bool sectionAsciiArt = false;
static bool sectionCrossHatching = false;
static bool sectionBokeh = false;
static bool sectionBloom = false;
static bool sectionPencilSketch = false;
static bool sectionMatrixRain = false;
static bool sectionImpressionist = false;

static void DrawStylePixelation(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Pixelation", categoryGlow, &sectionPixelation)) {
        const bool wasEnabled = e->pixelation.enabled;
        ImGui::Checkbox("Enabled##pixel", &e->pixelation.enabled);
        if (!wasEnabled && e->pixelation.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_PIXELATION); }
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
}

static void DrawStyleGlitch(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Glitch", categoryGlow, &sectionGlitch)) {
        const bool wasEnabled = e->glitch.enabled;
        ImGui::Checkbox("Enabled##glitch", &e->glitch.enabled);
        if (!wasEnabled && e->glitch.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_GLITCH); }
        if (e->glitch.enabled) {
            GlitchConfig* g = &e->glitch;

            if (TreeNodeAccented("CRT##glitch", categoryGlow)) {
                ImGui::Checkbox("Enabled##crt", &g->crtEnabled);
                if (g->crtEnabled) {
                    ImGui::SliderFloat("Curvature##crt", &g->curvature, 0.0f, 0.2f, "%.3f");
                    ImGui::Checkbox("Vignette##crt", &g->vignetteEnabled);
                }
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Analog##glitch", categoryGlow)) {
                ModulatableSlider("Intensity##analog", &g->analogIntensity,
                                  "glitch.analogIntensity", "%.3f", modSources);
                ModulatableSlider("Aberration##analog", &g->aberration,
                                  "glitch.aberration", "%.1f px", modSources);
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Digital##glitch", categoryGlow)) {
                ModulatableSlider("Block Threshold##digital", &g->blockThreshold,
                                  "glitch.blockThreshold", "%.2f", modSources);
                ModulatableSlider("Block Offset##digital", &g->blockOffset,
                                  "glitch.blockOffset", "%.2f", modSources);
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("VHS##glitch", categoryGlow)) {
                ImGui::Checkbox("Enabled##vhs", &g->vhsEnabled);
                if (g->vhsEnabled) {
                    ImGui::SliderFloat("Tracking Bars##vhs", &g->trackingBarIntensity, 0.0f, 0.05f, "%.3f");
                    ImGui::SliderFloat("Scanline Noise##vhs", &g->scanlineNoiseIntensity, 0.0f, 0.02f, "%.4f");
                    ImGui::SliderFloat("Color Drift##vhs", &g->colorDriftIntensity, 0.0f, 2.0f, "%.2f");
                }
                TreeNodeAccentedPop();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Overlay");
            ImGui::SliderFloat("Scanlines##glitch", &g->scanlineAmount, 0.0f, 0.5f, "%.2f");
            ImGui::SliderFloat("Noise##glitch", &g->noiseAmount, 0.0f, 0.3f, "%.2f");
        }
        DrawSectionEnd();
    }
}

static void DrawStyleToon(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    (void)modSources;
    if (DrawSectionBegin("Toon", categoryGlow, &sectionToon)) {
        const bool wasEnabled = e->toon.enabled;
        ImGui::Checkbox("Enabled##toon", &e->toon.enabled);
        if (!wasEnabled && e->toon.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_TOON); }
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
}

static void DrawStyleOilPaint(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Oil Paint", categoryGlow, &sectionOilPaint)) {
        const bool wasEnabled = e->oilPaint.enabled;
        ImGui::Checkbox("Enabled##oilpaint", &e->oilPaint.enabled);
        if (!wasEnabled && e->oilPaint.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_OIL_PAINT); }
        if (e->oilPaint.enabled) {
            OilPaintConfig* op = &e->oilPaint;
            ModulatableSlider("Radius##oilpaint", &op->radius,
                              "oilPaint.radius", "%.0f", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawStyleWatercolor(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Watercolor", categoryGlow, &sectionWatercolor)) {
        const bool wasEnabled = e->watercolor.enabled;
        ImGui::Checkbox("Enabled##watercolor", &e->watercolor.enabled);
        if (!wasEnabled && e->watercolor.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_WATERCOLOR); }
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
}

static void DrawStyleNeonGlow(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Neon Glow", categoryGlow, &sectionNeonGlow)) {
        const bool wasEnabled = e->neonGlow.enabled;
        ImGui::Checkbox("Enabled##neonglow", &e->neonGlow.enabled);
        if (!wasEnabled && e->neonGlow.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_NEON_GLOW); }
        if (e->neonGlow.enabled) {
            NeonGlowConfig* ng = &e->neonGlow;

            float glowCol[3] = { ng->glowR, ng->glowG, ng->glowB };
            if (ImGui::ColorEdit3("Glow Color##neonglow", glowCol)) {
                ng->glowR = glowCol[0];
                ng->glowG = glowCol[1];
                ng->glowB = glowCol[2];
            }

            ModulatableSlider("Glow Intensity##neonglow", &ng->glowIntensity,
                              "neonGlow.glowIntensity", "%.2f", modSources);
            ModulatableSlider("Edge Threshold##neonglow", &ng->edgeThreshold,
                              "neonGlow.edgeThreshold", "%.3f", modSources);
            ModulatableSlider("Original Visibility##neonglow", &ng->originalVisibility,
                              "neonGlow.originalVisibility", "%.2f", modSources);

            if (TreeNodeAccented("Advanced##neonglow", categoryGlow)) {
                ImGui::SliderFloat("Edge Power##neonglow", &ng->edgePower, 0.5f, 3.0f, "%.2f");
                ImGui::SliderFloat("Glow Radius##neonglow", &ng->glowRadius, 0.0f, 10.0f, "%.1f");
                ImGui::SliderInt("Glow Samples##neonglow", &ng->glowSamples, 3, 9);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawStyleHeightfieldRelief(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Heightfield Relief", categoryGlow, &sectionHeightfieldRelief)) {
        const bool wasEnabled = e->heightfieldRelief.enabled;
        ImGui::Checkbox("Enabled##relief", &e->heightfieldRelief.enabled);
        if (!wasEnabled && e->heightfieldRelief.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_HEIGHTFIELD_RELIEF); }
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
}

static void DrawStyleAsciiArt(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("ASCII Art", categoryGlow, &sectionAsciiArt)) {
        const bool wasEnabled = e->asciiArt.enabled;
        ImGui::Checkbox("Enabled##ascii", &e->asciiArt.enabled);
        if (!wasEnabled && e->asciiArt.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_ASCII_ART); }
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

static void DrawStyleCrossHatching(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Cross-Hatching", categoryGlow, &sectionCrossHatching)) {
        const bool wasEnabled = e->crossHatching.enabled;
        ImGui::Checkbox("Enabled##crosshatch", &e->crossHatching.enabled);
        if (!wasEnabled && e->crossHatching.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_CROSS_HATCHING); }
        if (e->crossHatching.enabled) {
            CrossHatchingConfig* ch = &e->crossHatching;

            ModulatableSlider("Width##crosshatch", &ch->width,
                              "crossHatching.width", "%.2f px", modSources);
            ModulatableSlider("Threshold##crosshatch", &ch->threshold,
                              "crossHatching.threshold", "%.2f", modSources);
            ModulatableSlider("Noise##crosshatch", &ch->noise,
                              "crossHatching.noise", "%.2f", modSources);
            ModulatableSlider("Outline##crosshatch", &ch->outline,
                              "crossHatching.outline", "%.2f", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawStyleBokeh(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Bokeh", categoryGlow, &sectionBokeh)) {
        const bool wasEnabled = e->bokeh.enabled;
        ImGui::Checkbox("Enabled##bokeh", &e->bokeh.enabled);
        if (!wasEnabled && e->bokeh.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_BOKEH); }
        if (e->bokeh.enabled) {
            BokehConfig* b = &e->bokeh;

            ModulatableSlider("Radius##bokeh", &b->radius,
                              "bokeh.radius", "%.3f", modSources);
            ImGui::SliderInt("Iterations##bokeh", &b->iterations, 16, 150);
            ModulatableSlider("Brightness##bokeh", &b->brightnessPower,
                              "bokeh.brightnessPower", "%.1f", modSources);
        }
        DrawSectionEnd();
    }
}

static void DrawStyleBloom(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Bloom", categoryGlow, &sectionBloom)) {
        const bool wasEnabled = e->bloom.enabled;
        ImGui::Checkbox("Enabled##bloom", &e->bloom.enabled);
        if (!wasEnabled && e->bloom.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_BLOOM); }
        if (e->bloom.enabled) {
            BloomConfig* b = &e->bloom;

            ModulatableSlider("Threshold##bloom", &b->threshold,
                              "bloom.threshold", "%.2f", modSources);
            ImGui::SliderFloat("Knee##bloom", &b->knee, 0.0f, 1.0f, "%.2f");
            ModulatableSlider("Intensity##bloom", &b->intensity,
                              "bloom.intensity", "%.2f", modSources);
            ImGui::SliderInt("Iterations##bloom", &b->iterations, 3, 5);
        }
        DrawSectionEnd();
    }
}

static void DrawStylePencilSketch(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Pencil Sketch", categoryGlow, &sectionPencilSketch)) {
        const bool wasEnabled = e->pencilSketch.enabled;
        ImGui::Checkbox("Enabled##pencilsketch", &e->pencilSketch.enabled);
        if (!wasEnabled && e->pencilSketch.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_PENCIL_SKETCH); }
        if (e->pencilSketch.enabled) {
            PencilSketchConfig* ps = &e->pencilSketch;

            ImGui::SliderInt("Angle Count##pencilsketch", &ps->angleCount, 2, 6);
            ImGui::SliderInt("Sample Count##pencilsketch", &ps->sampleCount, 8, 24);
            ModulatableSlider("Stroke Falloff##pencilsketch", &ps->strokeFalloff,
                              "pencilSketch.strokeFalloff", "%.2f", modSources);
            ImGui::SliderFloat("Gradient Eps##pencilsketch", &ps->gradientEps, 0.2f, 1.0f, "%.2f");
            ModulatableSlider("Paper Strength##pencilsketch", &ps->paperStrength,
                              "pencilSketch.paperStrength", "%.2f", modSources);
            ModulatableSlider("Vignette##pencilsketch", &ps->vignetteStrength,
                              "pencilSketch.vignetteStrength", "%.2f", modSources);

            if (TreeNodeAccented("Animation##pencilsketch", categoryGlow)) {
                ImGui::SliderFloat("Wobble Speed##pencilsketch", &ps->wobbleSpeed, 0.0f, 2.0f, "%.2f");
                ModulatableSlider("Wobble Amount##pencilsketch", &ps->wobbleAmount,
                                  "pencilSketch.wobbleAmount", "%.1f px", modSources);
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}

static void DrawStyleMatrixRain(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Matrix Rain", categoryGlow, &sectionMatrixRain)) {
        const bool wasEnabled = e->matrixRain.enabled;
        ImGui::Checkbox("Enabled##matrixrain", &e->matrixRain.enabled);
        if (!wasEnabled && e->matrixRain.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_MATRIX_RAIN); }
        if (e->matrixRain.enabled) {
            MatrixRainConfig* mr = &e->matrixRain;

            ImGui::SliderFloat("Cell Size##matrixrain", &mr->cellSize, 4.0f, 32.0f, "%.0f px");
            ModulatableSlider("Rain Speed##matrixrain", &mr->rainSpeed,
                              "matrixRain.rainSpeed", "%.2f", modSources);
            ModulatableSlider("Trail Length##matrixrain", &mr->trailLength,
                              "matrixRain.trailLength", "%.0f", modSources);
            ImGui::SliderInt("Faller Count##matrixrain", &mr->fallerCount, 1, 20);
            ModulatableSlider("Overlay Intensity##matrixrain", &mr->overlayIntensity,
                              "matrixRain.overlayIntensity", "%.2f", modSources);
            ImGui::SliderFloat("Refresh Rate##matrixrain", &mr->refreshRate, 0.1f, 5.0f, "%.2f");
            ModulatableSlider("Lead Brightness##matrixrain", &mr->leadBrightness,
                              "matrixRain.leadBrightness", "%.2f", modSources);
            ImGui::Checkbox("Sample##matrixrain", &mr->sampleMode);
        }
        DrawSectionEnd();
    }
}

static void DrawStyleImpressionist(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("Impressionist", categoryGlow, &sectionImpressionist)) {
        const bool wasEnabled = e->impressionist.enabled;
        ImGui::Checkbox("Enabled##impressionist", &e->impressionist.enabled);
        if (!wasEnabled && e->impressionist.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_IMPRESSIONIST); }
        if (e->impressionist.enabled) {
            ImpressionistConfig* imp = &e->impressionist;

            ModulatableSlider("Splat Size Max##impressionist", &imp->splatSizeMax,
                              "impressionist.splatSizeMax", "%.3f", modSources);
            ModulatableSlider("Stroke Freq##impressionist", &imp->strokeFreq,
                              "impressionist.strokeFreq", "%.0f", modSources);
            ModulatableSlider("Edge Strength##impressionist", &imp->edgeStrength,
                              "impressionist.edgeStrength", "%.2f", modSources);
            ModulatableSlider("Stroke Opacity##impressionist", &imp->strokeOpacity,
                              "impressionist.strokeOpacity", "%.2f", modSources);
            ImGui::SliderInt("Splat Count##impressionist", &imp->splatCount, 4, 16);
            ImGui::SliderFloat("Splat Size Min##impressionist", &imp->splatSizeMin, 0.01f, 0.1f, "%.3f");
            ImGui::SliderFloat("Outline Strength##impressionist", &imp->outlineStrength, 0.0f, 0.5f, "%.3f");
            ImGui::SliderFloat("Edge Max Darken##impressionist", &imp->edgeMaxDarken, 0.0f, 0.3f, "%.3f");
            ImGui::SliderFloat("Grain Scale##impressionist", &imp->grainScale, 100.0f, 800.0f, "%.0f");
            ImGui::SliderFloat("Grain Amount##impressionist", &imp->grainAmount, 0.0f, 0.2f, "%.3f");
            ImGui::SliderFloat("Exposure##impressionist", &imp->exposure, 0.5f, 2.0f, "%.2f");
        }
        DrawSectionEnd();
    }
}

void DrawStyleCategory(EffectConfig* e, const ModSources* modSources)
{
    const ImU32 categoryGlow = Theme::GetSectionGlow(4);
    DrawCategoryHeader("Style", categoryGlow);
    DrawStylePixelation(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleGlitch(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleToon(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleOilPaint(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleWatercolor(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleNeonGlow(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleHeightfieldRelief(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleAsciiArt(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleCrossHatching(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleBokeh(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleBloom(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStylePencilSketch(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleMatrixRain(e, modSources, categoryGlow);
    ImGui::Spacing();
    DrawStyleImpressionist(e, modSources, categoryGlow);
}
