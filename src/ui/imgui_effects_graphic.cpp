#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "config/halftone_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionToon = false;
static bool sectionNeonGlow = false;
static bool sectionKuwahara = false;
static bool sectionHalftone = false;
static bool sectionSynthwave = false;

static void DrawGraphicToon(EffectConfig *e, const ModSources *modSources,
                            const ImU32 categoryGlow) {
  (void)modSources;
  if (DrawSectionBegin("Toon", categoryGlow, &sectionToon)) {
    const bool wasEnabled = e->toon.enabled;
    ImGui::Checkbox("Enabled##toon", &e->toon.enabled);
    if (!wasEnabled && e->toon.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_TOON);
    }
    if (e->toon.enabled) {
      ToonConfig *t = &e->toon;

      ImGui::SliderInt("Levels##toon", &t->levels, 2, 16);
      ImGui::SliderFloat("Edge Threshold##toon", &t->edgeThreshold, 0.0f, 1.0f,
                         "%.2f");
      ImGui::SliderFloat("Edge Softness##toon", &t->edgeSoftness, 0.0f, 0.2f,
                         "%.3f");

      if (TreeNodeAccented("Brush Stroke##toon", categoryGlow)) {
        ImGui::SliderFloat("Thickness Variation##toon", &t->thicknessVariation,
                           0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Noise Scale##toon", &t->noiseScale, 1.0f, 20.0f,
                           "%.1f");
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGraphicNeonGlow(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Neon Glow", categoryGlow, &sectionNeonGlow)) {
    const bool wasEnabled = e->neonGlow.enabled;
    ImGui::Checkbox("Enabled##neonglow", &e->neonGlow.enabled);
    if (!wasEnabled && e->neonGlow.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_NEON_GLOW);
    }
    if (e->neonGlow.enabled) {
      NeonGlowConfig *ng = &e->neonGlow;

      const char *colorModeLabels[] = {"Custom Color", "Source Color"};
      ImGui::Combo("Color Mode##neonglow", &ng->colorMode, colorModeLabels, 2);

      if (ng->colorMode == 0) {
        float glowCol[3] = {ng->glowR, ng->glowG, ng->glowB};
        if (ImGui::ColorEdit3("Glow Color##neonglow", glowCol)) {
          ng->glowR = glowCol[0];
          ng->glowG = glowCol[1];
          ng->glowB = glowCol[2];
        }
      } else {
        ModulatableSlider("Saturation Boost##neonglow", &ng->saturationBoost,
                          "neonGlow.saturationBoost", "%.2f", modSources);
        ModulatableSlider("Brightness Boost##neonglow", &ng->brightnessBoost,
                          "neonGlow.brightnessBoost", "%.2f", modSources);
      }

      ModulatableSlider("Glow Intensity##neonglow", &ng->glowIntensity,
                        "neonGlow.glowIntensity", "%.2f", modSources);
      ModulatableSlider("Edge Threshold##neonglow", &ng->edgeThreshold,
                        "neonGlow.edgeThreshold", "%.3f", modSources);
      ModulatableSlider("Original Visibility##neonglow",
                        &ng->originalVisibility, "neonGlow.originalVisibility",
                        "%.2f", modSources);

      if (TreeNodeAccented("Advanced##neonglow", categoryGlow)) {
        ImGui::SliderFloat("Edge Power##neonglow", &ng->edgePower, 0.5f, 3.0f,
                           "%.2f");
        ImGui::SliderFloat("Glow Radius##neonglow", &ng->glowRadius, 0.0f,
                           10.0f, "%.1f");
        ImGui::SliderInt("Glow Samples##neonglow", &ng->glowSamples, 3, 9);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGraphicKuwahara(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Kuwahara", categoryGlow, &sectionKuwahara)) {
    const bool wasEnabled = e->kuwahara.enabled;
    ImGui::Checkbox("Enabled##kuwahara", &e->kuwahara.enabled);
    if (!wasEnabled && e->kuwahara.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_KUWAHARA);
    }
    if (e->kuwahara.enabled) {
      ModulatableSlider("Radius##kuwahara", &e->kuwahara.radius,
                        "kuwahara.radius", "%.0f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawGraphicHalftone(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Halftone", categoryGlow, &sectionHalftone)) {
    const bool wasEnabled = e->halftone.enabled;
    ImGui::Checkbox("Enabled##halftone", &e->halftone.enabled);
    if (!wasEnabled && e->halftone.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_HALFTONE);
    }
    if (e->halftone.enabled) {
      HalftoneConfig *ht = &e->halftone;

      ModulatableSlider("Dot Scale##halftone", &ht->dotScale,
                        "halftone.dotScale", "%.1f px", modSources);
      ImGui::SliderFloat("Dot Size##halftone", &ht->dotSize, 0.5f, 2.0f,
                         "%.2f");
      ModulatableSliderAngleDeg("Spin##halftone", &ht->rotationSpeed,
                                "halftone.rotationSpeed", modSources,
                                "%.1f °/s");
      ModulatableSliderAngleDeg("Angle##halftone", &ht->rotationAngle,
                                "halftone.rotationAngle", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawGraphicSynthwave(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Synthwave", categoryGlow, &sectionSynthwave)) {
    const bool wasEnabled = e->synthwave.enabled;
    ImGui::Checkbox("Enabled##synthwave", &e->synthwave.enabled);
    if (!wasEnabled && e->synthwave.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SYNTHWAVE);
    }
    if (e->synthwave.enabled) {
      SynthwaveConfig *sw = &e->synthwave;

      ModulatableSlider("Horizon##synthwave", &sw->horizonY,
                        "synthwave.horizonY", "%.2f", modSources);
      ModulatableSlider("Color Mix##synthwave", &sw->colorMix,
                        "synthwave.colorMix", "%.2f", modSources);

      if (TreeNodeAccented("Palette##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Phase R##synthwave", &sw->palettePhaseR, 0.0f, 1.0f,
                           "%.2f");
        ImGui::SliderFloat("Phase G##synthwave", &sw->palettePhaseG, 0.0f, 1.0f,
                           "%.2f");
        ImGui::SliderFloat("Phase B##synthwave", &sw->palettePhaseB, 0.0f, 1.0f,
                           "%.2f");
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Grid##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Spacing##synthwave", &sw->gridSpacing, 2.0f, 20.0f,
                           "%.1f");
        ImGui::SliderFloat("Line Width##synthwave", &sw->gridThickness, 0.01f,
                           0.1f, "%.3f");
        ModulatableSlider("Opacity##synthwave_grid", &sw->gridOpacity,
                          "synthwave.gridOpacity", "%.2f", modSources);
        ModulatableSlider("Glow##synthwave", &sw->gridGlow,
                          "synthwave.gridGlow", "%.2f", modSources);
        float gridCol[3] = {sw->gridR, sw->gridG, sw->gridB};
        if (ImGui::ColorEdit3("Color##synthwave_grid", gridCol)) {
          sw->gridR = gridCol[0];
          sw->gridG = gridCol[1];
          sw->gridB = gridCol[2];
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Sun Stripes##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Count##synthwave", &sw->stripeCount, 4.0f, 20.0f,
                           "%.0f");
        ImGui::SliderFloat("Softness##synthwave", &sw->stripeSoftness, 0.0f,
                           0.3f, "%.2f");
        ModulatableSlider("Intensity##synthwave_stripe", &sw->stripeIntensity,
                          "synthwave.stripeIntensity", "%.2f", modSources);
        float sunCol[3] = {sw->sunR, sw->sunG, sw->sunB};
        if (ImGui::ColorEdit3("Color##synthwave_sun", sunCol)) {
          sw->sunR = sunCol[0];
          sw->sunG = sunCol[1];
          sw->sunB = sunCol[2];
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Horizon Glow##synthwave", categoryGlow)) {
        ModulatableSlider("Intensity##synthwave_horizon", &sw->horizonIntensity,
                          "synthwave.horizonIntensity", "%.2f", modSources);
        ImGui::SliderFloat("Falloff##synthwave", &sw->horizonFalloff, 5.0f,
                           30.0f, "%.1f");
        float horizonCol[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
        if (ImGui::ColorEdit3("Color##synthwave_horizon", horizonCol)) {
          sw->horizonR = horizonCol[0];
          sw->horizonG = horizonCol[1];
          sw->horizonB = horizonCol[2];
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Animation##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Grid Scroll##synthwave", &sw->gridScrollSpeed, 0.0f,
                           2.0f, "%.2f");
        ImGui::SliderFloat("Stripe Scroll##synthwave", &sw->stripeScrollSpeed,
                           0.0f, 0.5f, "%.3f");
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

void DrawGraphicCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(5);
  DrawCategoryHeader("Graphic", categoryGlow);
  DrawGraphicToon(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGraphicNeonGlow(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGraphicKuwahara(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGraphicHalftone(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGraphicSynthwave(e, modSources, categoryGlow);
}
