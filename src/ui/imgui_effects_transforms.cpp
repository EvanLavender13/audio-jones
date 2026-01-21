#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "config/false_color_config.h"
#include "config/halftone_config.h"
#include "config/cross_hatching_config.h"
#include "config/palette_quantization_config.h"
#include "automation/mod_sources.h"

// Persistent section open states for transform categories
static bool sectionInfiniteZoom = false;
static bool sectionRadialStreak = false;
static bool sectionDrosteZoom = false;
static bool sectionColorGrade = false;
static bool sectionFalseColor = false;
static bool sectionHalftone = false;
static bool sectionPaletteQuantization = false;
static bool sectionDensityWaveSpiral = false;

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
