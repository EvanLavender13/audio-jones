#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/nebula.h"
#include "effects/solid_color.h"
#include "imgui.h"
#include "render/blend_mode.h"
#include "ui/imgui_effects_generators.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionNebula = false;
static bool sectionSolidColor = false;

static void DrawGeneratorsNebula(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Nebula", categoryGlow, &sectionNebula,
                       e->nebula.enabled)) {
    const bool wasEnabled = e->nebula.enabled;
    ImGui::Checkbox("Enabled##nebula", &e->nebula.enabled);
    if (!wasEnabled && e->nebula.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_NEBULA_BLEND);
    }
    if (e->nebula.enabled) {
      NebulaConfig *n = &e->nebula;

      // FFT
      ImGui::SeparatorText("Audio");
      ImGui::SliderInt("Octaves##nebula", &n->numOctaves, 1, 8);
      ModulatableSlider("Base Freq (Hz)##nebula", &n->baseFreq,
                        "nebula.baseFreq", "%.1f", modSources);
      ModulatableSlider("Gain##nebula", &n->gain, "nebula.gain", "%.1f",
                        modSources);
      ModulatableSlider("Contrast##nebula", &n->curve, "nebula.curve", "%.2f",
                        modSources);
      ModulatableSlider("Base Bright##nebula", &n->baseBright,
                        "nebula.baseBright", "%.2f", modSources);

      // Layers
      ImGui::SeparatorText("Layers");
      ImGui::Combo("Noise Type##nebula", &n->noiseType, "Kaliset\0FBM\0");
      ModulatableSlider("Front Scale##nebula", &n->frontScale,
                        "nebula.frontScale", "%.1f", modSources);
      ModulatableSlider("Mid Scale##nebula", &n->midScale, "nebula.midScale",
                        "%.1f", modSources);
      ModulatableSlider("Back Scale##nebula", &n->backScale, "nebula.backScale",
                        "%.1f", modSources);
      if (n->noiseType == 1) {
        ImGui::SliderInt("Front Octaves##nebula", &n->fbmFrontOct, 2, 8);
        ImGui::SliderInt("Mid Octaves##nebula", &n->fbmMidOct, 2, 8);
        ImGui::SliderInt("Back Octaves##nebula", &n->fbmBackOct, 2, 8);
      } else {
        ImGui::SliderInt("Front Iterations##nebula", &n->frontIter, 6, 40);
        ImGui::SliderInt("Mid Iterations##nebula", &n->midIter, 6, 40);
        ImGui::SliderInt("Back Iterations##nebula", &n->backIter, 6, 40);
      }

      // Dust
      ImGui::SeparatorText("Dust");
      ModulatableSlider("Dust Scale##nebula", &n->dustScale, "nebula.dustScale",
                        "%.1f", modSources);
      ModulatableSlider("Dust Strength##nebula", &n->dustStrength,
                        "nebula.dustStrength", "%.2f", modSources);
      ModulatableSlider("Dust Edge##nebula", &n->dustEdge, "nebula.dustEdge",
                        "%.2f", modSources);

      // Stars
      ImGui::SeparatorText("Stars");
      ModulatableSlider("Star Density##nebula", &n->starDensity,
                        "nebula.starDensity", "%.0f", modSources);
      ModulatableSlider("Star Rarity##nebula", &n->starSharpness,
                        "nebula.starSharpness", "%.1f", modSources);
      ModulatableSlider("Glow Width##nebula", &n->glowWidth, "nebula.glowWidth",
                        "%.2f", modSources);
      ModulatableSlider("Glow Intensity##nebula", &n->glowIntensity,
                        "nebula.glowIntensity", "%.1f", modSources);

      // Spikes
      ImGui::SeparatorText("Spikes");
      ModulatableSlider("Spike Intensity##nebula", &n->spikeIntensity,
                        "nebula.spikeIntensity", "%.2f", modSources);
      ModulatableSlider("Spike Sharpness##nebula", &n->spikeSharpness,
                        "nebula.spikeSharpness", "%.1f", modSources);

      // Animation
      ImGui::SeparatorText("Animation");
      ModulatableSlider("Drift Speed##nebula", &n->driftSpeed,
                        "nebula.driftSpeed", "%.3f", modSources);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Brightness##nebula", &n->brightness,
                        "nebula.brightness", "%.2f", modSources);
      ImGuiDrawColorMode(&n->gradient);
      ModulatableSlider("Blend Intensity##nebula", &n->blendIntensity,
                        "nebula.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)n->blendMode;
      if (ImGui::Combo("Blend Mode##nebula", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        n->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsSolidColor(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Solid Color", categoryGlow, &sectionSolidColor,
                       e->solidColor.enabled)) {
    const bool wasEnabled = e->solidColor.enabled;
    ImGui::Checkbox("Enabled##solidcolor", &e->solidColor.enabled);
    if (!wasEnabled && e->solidColor.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SOLID_COLOR);
    }
    if (e->solidColor.enabled) {
      SolidColorConfig *sc = &e->solidColor;

      ImGuiDrawColorMode(&sc->color);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##solidcolor", &sc->blendIntensity,
                        "solidColor.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)sc->blendMode;
      if (ImGui::Combo("Blend Mode##solidcolor", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        sc->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

void DrawGeneratorsAtmosphere(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(3);
  DrawCategoryHeader("Atmosphere", categoryGlow);
  DrawGeneratorsNebula(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsSolidColor(e, modSources, categoryGlow);
}
