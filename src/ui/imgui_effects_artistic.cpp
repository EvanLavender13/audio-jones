#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"

static bool sectionOilPaint = false;
static bool sectionWatercolor = false;
static bool sectionImpressionist = false;
static bool sectionInkWash = false;
static bool sectionPencilSketch = false;
static bool sectionCrossHatching = false;

static void DrawArtisticOilPaint(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Oil Paint", categoryGlow, &sectionOilPaint)) {
    const bool wasEnabled = e->oilPaint.enabled;
    ImGui::Checkbox("Enabled##oilpaint", &e->oilPaint.enabled);
    if (!wasEnabled && e->oilPaint.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_OIL_PAINT);
    }
    if (e->oilPaint.enabled) {
      OilPaintConfig *op = &e->oilPaint;
      ModulatableSlider("Brush Size##oilpaint", &op->brushSize,
                        "oilPaint.brushSize", "%.2f", modSources);
      ModulatableSlider("Stroke Bend##oilpaint", &op->strokeBend,
                        "oilPaint.strokeBend", "%.2f", modSources);
      ModulatableSlider("Specular##oilpaint", &op->specular,
                        "oilPaint.specular", "%.2f", modSources);
      ImGui::SliderInt("Layers##oilpaint", &op->layers, 3, 11);
    }
    DrawSectionEnd();
  }
}

static void DrawArtisticWatercolor(EffectConfig *e,
                                   const ModSources *modSources,
                                   const ImU32 categoryGlow) {
  if (DrawSectionBegin("Watercolor", categoryGlow, &sectionWatercolor)) {
    const bool wasEnabled = e->watercolor.enabled;
    ImGui::Checkbox("Enabled##watercolor", &e->watercolor.enabled);
    if (!wasEnabled && e->watercolor.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_WATERCOLOR);
    }
    if (e->watercolor.enabled) {
      WatercolorConfig *wc = &e->watercolor;
      ImGui::SliderInt("Samples##wc", &wc->samples, 8, 32);
      ModulatableSlider("Stroke Step##wc", &wc->strokeStep,
                        "watercolor.strokeStep", "%.2f", modSources);
      ModulatableSlider("Wash Strength##wc", &wc->washStrength,
                        "watercolor.washStrength", "%.2f", modSources);
      ImGui::SliderFloat("Paper Scale##wc", &wc->paperScale, 1.0f, 20.0f,
                         "%.1f");
      ModulatableSlider("Paper Strength##wc", &wc->paperStrength,
                        "watercolor.paperStrength", "%.2f", modSources);
      ImGui::SliderFloat("Edge Pool##wc", &wc->edgePool, 0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Flow Center##wc", &wc->flowCenter, 0.5f, 1.2f,
                         "%.2f");
      ImGui::SliderFloat("Flow Width##wc", &wc->flowWidth, 0.05f, 0.5f, "%.2f");
    }
    DrawSectionEnd();
  }
}

static void DrawArtisticImpressionist(EffectConfig *e,
                                      const ModSources *modSources,
                                      const ImU32 categoryGlow) {
  if (DrawSectionBegin("Impressionist", categoryGlow, &sectionImpressionist)) {
    const bool wasEnabled = e->impressionist.enabled;
    ImGui::Checkbox("Enabled##impressionist", &e->impressionist.enabled);
    if (!wasEnabled && e->impressionist.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_IMPRESSIONIST);
    }
    if (e->impressionist.enabled) {
      ImpressionistConfig *imp = &e->impressionist;

      ModulatableSlider("Splat Size Max##impressionist", &imp->splatSizeMax,
                        "impressionist.splatSizeMax", "%.3f", modSources);
      ModulatableSlider("Stroke Freq##impressionist", &imp->strokeFreq,
                        "impressionist.strokeFreq", "%.0f", modSources);
      ModulatableSlider("Edge Strength##impressionist", &imp->edgeStrength,
                        "impressionist.edgeStrength", "%.2f", modSources);
      ModulatableSlider("Stroke Opacity##impressionist", &imp->strokeOpacity,
                        "impressionist.strokeOpacity", "%.2f", modSources);
      ImGui::SliderInt("Splat Count##impressionist", &imp->splatCount, 4, 16);
      ImGui::SliderFloat("Splat Size Min##impressionist", &imp->splatSizeMin,
                         0.01f, 0.1f, "%.3f");
      ImGui::SliderFloat("Outline Strength##impressionist",
                         &imp->outlineStrength, 0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Edge Max Darken##impressionist", &imp->edgeMaxDarken,
                         0.0f, 0.3f, "%.3f");
      ImGui::SliderFloat("Grain Scale##impressionist", &imp->grainScale, 100.0f,
                         800.0f, "%.0f");
      ImGui::SliderFloat("Grain Amount##impressionist", &imp->grainAmount, 0.0f,
                         0.2f, "%.3f");
      ImGui::SliderFloat("Exposure##impressionist", &imp->exposure, 0.5f, 2.0f,
                         "%.2f");
    }
    DrawSectionEnd();
  }
}

static void DrawArtisticInkWash(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Ink Wash", categoryGlow, &sectionInkWash)) {
    const bool wasEnabled = e->inkWash.enabled;
    ImGui::Checkbox("Enabled##inkwash", &e->inkWash.enabled);
    if (!wasEnabled && e->inkWash.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_INK_WASH);
    }
    if (e->inkWash.enabled) {
      ModulatableSlider("Strength##inkwash", &e->inkWash.strength,
                        "inkWash.strength", "%.2f", modSources);
      ModulatableSlider("Granulation##inkwash", &e->inkWash.granulation,
                        "inkWash.granulation", "%.2f", modSources);
      ModulatableSlider("Bleed##inkwash", &e->inkWash.bleedStrength,
                        "inkWash.bleedStrength", "%.2f", modSources);
      ModulatableSlider("Bleed Radius##inkwash", &e->inkWash.bleedRadius,
                        "inkWash.bleedRadius", "%.1f px", modSources);
      ModulatableSlider("Softness##inkwash", &e->inkWash.softness,
                        "inkWash.softness", "%.0f px", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawArtisticPencilSketch(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Pencil Sketch", categoryGlow, &sectionPencilSketch)) {
    const bool wasEnabled = e->pencilSketch.enabled;
    ImGui::Checkbox("Enabled##pencilsketch", &e->pencilSketch.enabled);
    if (!wasEnabled && e->pencilSketch.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PENCIL_SKETCH);
    }
    if (e->pencilSketch.enabled) {
      PencilSketchConfig *ps = &e->pencilSketch;

      ImGui::SliderInt("Angle Count##pencilsketch", &ps->angleCount, 2, 6);
      ImGui::SliderInt("Sample Count##pencilsketch", &ps->sampleCount, 8, 24);
      ModulatableSlider("Stroke Falloff##pencilsketch", &ps->strokeFalloff,
                        "pencilSketch.strokeFalloff", "%.2f", modSources);
      ImGui::SliderFloat("Gradient Eps##pencilsketch", &ps->gradientEps, 0.2f,
                         1.0f, "%.2f");
      ModulatableSlider("Paper Strength##pencilsketch", &ps->paperStrength,
                        "pencilSketch.paperStrength", "%.2f", modSources);
      ModulatableSlider("Vignette##pencilsketch", &ps->vignetteStrength,
                        "pencilSketch.vignetteStrength", "%.2f", modSources);

      if (TreeNodeAccented("Animation##pencilsketch", categoryGlow)) {
        ImGui::SliderFloat("Wobble Speed##pencilsketch", &ps->wobbleSpeed, 0.0f,
                           2.0f, "%.2f");
        ModulatableSlider("Wobble Amount##pencilsketch", &ps->wobbleAmount,
                          "pencilSketch.wobbleAmount", "%.1f px", modSources);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawArtisticCrossHatching(EffectConfig *e,
                                      const ModSources *modSources,
                                      const ImU32 categoryGlow) {
  if (DrawSectionBegin("Cross-Hatching", categoryGlow, &sectionCrossHatching)) {
    const bool wasEnabled = e->crossHatching.enabled;
    ImGui::Checkbox("Enabled##crosshatch", &e->crossHatching.enabled);
    if (!wasEnabled && e->crossHatching.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CROSS_HATCHING);
    }
    if (e->crossHatching.enabled) {
      CrossHatchingConfig *ch = &e->crossHatching;

      ModulatableSlider("Width##crosshatch", &ch->width, "crossHatching.width",
                        "%.2f px", modSources);
      ModulatableSlider("Threshold##crosshatch", &ch->threshold,
                        "crossHatching.threshold", "%.2f", modSources);
      ModulatableSlider("Noise##crosshatch", &ch->noise, "crossHatching.noise",
                        "%.2f", modSources);
      ModulatableSlider("Outline##crosshatch", &ch->outline,
                        "crossHatching.outline", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

void DrawArtisticCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(4);
  DrawCategoryHeader("ART", categoryGlow);
  DrawArtisticOilPaint(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawArtisticWatercolor(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawArtisticImpressionist(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawArtisticInkWash(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawArtisticPencilSketch(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawArtisticCrossHatching(e, modSources, categoryGlow);
}
