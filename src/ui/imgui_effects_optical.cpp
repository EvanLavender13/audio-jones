#include "automation/mod_sources.h"
#include "config/bloom_config.h"
#include "config/bokeh_config.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionBloom = false;
static bool sectionBokeh = false;
static bool sectionHeightfieldRelief = false;

static void DrawOpticalBloom(EffectConfig *e, const ModSources *modSources,
                             const ImU32 categoryGlow) {
  if (DrawSectionBegin("Bloom", categoryGlow, &sectionBloom)) {
    const bool wasEnabled = e->bloom.enabled;
    ImGui::Checkbox("Enabled##bloom", &e->bloom.enabled);
    if (!wasEnabled && e->bloom.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_BLOOM);
    }
    if (e->bloom.enabled) {
      BloomConfig *b = &e->bloom;

      ModulatableSlider("Threshold##bloom", &b->threshold, "bloom.threshold",
                        "%.2f", modSources);
      ImGui::SliderFloat("Knee##bloom", &b->knee, 0.0f, 1.0f, "%.2f");
      ModulatableSlider("Intensity##bloom", &b->intensity, "bloom.intensity",
                        "%.2f", modSources);
      ImGui::SliderInt("Iterations##bloom", &b->iterations, 3, 5);
    }
    DrawSectionEnd();
  }
}

static void DrawOpticalBokeh(EffectConfig *e, const ModSources *modSources,
                             const ImU32 categoryGlow) {
  if (DrawSectionBegin("Bokeh", categoryGlow, &sectionBokeh)) {
    const bool wasEnabled = e->bokeh.enabled;
    ImGui::Checkbox("Enabled##bokeh", &e->bokeh.enabled);
    if (!wasEnabled && e->bokeh.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_BOKEH);
    }
    if (e->bokeh.enabled) {
      BokehConfig *b = &e->bokeh;

      ModulatableSlider("Radius##bokeh", &b->radius, "bokeh.radius", "%.3f",
                        modSources);
      ImGui::SliderInt("Iterations##bokeh", &b->iterations, 16, 150);
      ModulatableSlider("Brightness##bokeh", &b->brightnessPower,
                        "bokeh.brightnessPower", "%.1f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawOpticalHeightfieldRelief(EffectConfig *e,
                                         const ModSources *modSources,
                                         const ImU32 categoryGlow) {
  if (DrawSectionBegin("Heightfield Relief", categoryGlow,
                       &sectionHeightfieldRelief)) {
    const bool wasEnabled = e->heightfieldRelief.enabled;
    ImGui::Checkbox("Enabled##relief", &e->heightfieldRelief.enabled);
    if (!wasEnabled && e->heightfieldRelief.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_HEIGHTFIELD_RELIEF);
    }
    if (e->heightfieldRelief.enabled) {
      HeightfieldReliefConfig *h = &e->heightfieldRelief;

      ModulatableSlider("Intensity##relief", &h->intensity,
                        "heightfieldRelief.intensity", "%.2f", modSources);
      ImGui::SliderFloat("Relief Scale##relief", &h->reliefScale, 0.02f, 1.0f,
                         "%.2f");
      ModulatableSliderAngleDeg("Light Angle##relief", &h->lightAngle,
                                "heightfieldRelief.lightAngle", modSources);
      ImGui::SliderFloat("Light Height##relief", &h->lightHeight, 0.1f, 2.0f,
                         "%.2f");
      ImGui::SliderFloat("Shininess##relief", &h->shininess, 1.0f, 128.0f,
                         "%.0f");
    }
    DrawSectionEnd();
  }
}

void DrawOpticalCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(7);
  DrawCategoryHeader("Optical", categoryGlow);
  DrawOpticalBloom(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawOpticalBokeh(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawOpticalHeightfieldRelief(e, modSources, categoryGlow);
}
