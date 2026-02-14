#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionColorGrade = false;
static bool sectionFalseColor = false;
static bool sectionPaletteQuantization = false;

static void DrawColorColorGrade(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Color Grade", categoryGlow, &sectionColorGrade,
                       e->colorGrade.enabled)) {
    const bool wasEnabled = e->colorGrade.enabled;
    ImGui::Checkbox("Enabled##colorgrade", &e->colorGrade.enabled);
    if (!wasEnabled && e->colorGrade.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_COLOR_GRADE);
    }
    if (e->colorGrade.enabled) {
      ColorGradeConfig *cg = &e->colorGrade;

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

static void DrawColorFalseColor(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("False Color", categoryGlow, &sectionFalseColor,
                       e->falseColor.enabled)) {
    const bool wasEnabled = e->falseColor.enabled;
    ImGui::Checkbox("Enabled##falsecolor", &e->falseColor.enabled);
    if (!wasEnabled && e->falseColor.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_FALSE_COLOR);
    }
    if (e->falseColor.enabled) {
      FalseColorConfig *fc = &e->falseColor;

      ImGuiDrawColorMode(&fc->gradient);

      ModulatableSlider("Intensity##falsecolor", &fc->intensity,
                        "falseColor.intensity", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawColorPaletteQuantization(EffectConfig *e,
                                         const ModSources *modSources,
                                         const ImU32 categoryGlow) {
  if (DrawSectionBegin("Palette Quantization", categoryGlow,
                       &sectionPaletteQuantization,
                       e->paletteQuantization.enabled)) {
    const bool wasEnabled = e->paletteQuantization.enabled;
    ImGui::Checkbox("Enabled##palettequant", &e->paletteQuantization.enabled);
    if (!wasEnabled && e->paletteQuantization.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PALETTE_QUANTIZATION);
    }
    if (e->paletteQuantization.enabled) {
      PaletteQuantizationConfig *pq = &e->paletteQuantization;

      ModulatableSlider("Color Levels##palettequant", &pq->colorLevels,
                        "paletteQuantization.colorLevels", "%.0f", modSources);
      ModulatableSlider("Dither##palettequant", &pq->ditherStrength,
                        "paletteQuantization.ditherStrength", "%.2f",
                        modSources);

      const char *bayerSizeNames[] = {"4x4 (Coarse)", "8x8 (Fine)"};
      int bayerIndex = (pq->bayerSize == 4) ? 0 : 1;
      if (ImGui::Combo("Pattern##palettequant", &bayerIndex, bayerSizeNames,
                       2)) {
        pq->bayerSize = (bayerIndex == 0) ? 4 : 8;
      }
    }
    DrawSectionEnd();
  }
}

void DrawColorCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(8);
  DrawCategoryHeader("Color", categoryGlow);
  DrawColorColorGrade(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawColorFalseColor(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawColorPaletteQuantization(e, modSources, categoryGlow);
}
