#include "automation/mod_sources.h"
#include "config/disco_ball_config.h"
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
static bool sectionDiscoBall = false;
static bool sectionLegoBricks = false;

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

static void DrawGraphicDiscoBall(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Disco Ball", categoryGlow, &sectionDiscoBall)) {
    const bool wasEnabled = e->discoBall.enabled;
    ImGui::Checkbox("Enabled##disco", &e->discoBall.enabled);
    if (!wasEnabled && e->discoBall.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_DISCO_BALL);
    }
    if (e->discoBall.enabled) {
      DiscoBallConfig *db = &e->discoBall;

      ModulatableSlider("Sphere Radius##disco", &db->sphereRadius,
                        "discoBall.sphereRadius", "%.2f", modSources);
      ModulatableSlider("Tile Size##disco", &db->tileSize, "discoBall.tileSize",
                        "%.3f", modSources);
      ModulatableSliderAngleDeg("Spin##disco", &db->rotationSpeed,
                                "discoBall.rotationSpeed", modSources,
                                "%.1f °/s");
      ModulatableSlider("Bevel##disco", &db->bumpHeight, "discoBall.bumpHeight",
                        "%.3f", modSources);
      ModulatableSlider("Intensity##disco", &db->reflectIntensity,
                        "discoBall.reflectIntensity", "%.2f", modSources);

      if (TreeNodeAccented("Light Spots##disco", categoryGlow)) {
        ModulatableSlider("Intensity##spot", &db->spotIntensity,
                          "discoBall.spotIntensity", "%.2f", modSources);
        ModulatableSlider("Softness##spot", &db->spotFalloff,
                          "discoBall.spotFalloff", "%.2f", modSources);
        ModulatableSlider("Threshold##spot", &db->brightnessThreshold,
                          "discoBall.brightnessThreshold", "%.2f", modSources);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGraphicLegoBricks(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("LEGO Bricks", categoryGlow, &sectionLegoBricks)) {
    const bool wasEnabled = e->legoBricks.enabled;
    ImGui::Checkbox("Enabled##legobricks", &e->legoBricks.enabled);
    if (!wasEnabled && e->legoBricks.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_LEGO_BRICKS);
    }
    if (e->legoBricks.enabled) {
      ModulatableSlider("Brick Scale##legobricks", &e->legoBricks.brickScale,
                        "legoBricks.brickScale", "%.3f", modSources);
      ModulatableSlider("Stud Height##legobricks", &e->legoBricks.studHeight,
                        "legoBricks.studHeight", "%.2f", modSources);
      ImGui::SliderFloat("Edge Shadow##legobricks", &e->legoBricks.edgeShadow,
                         0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Color Threshold##legobricks",
                         &e->legoBricks.colorThreshold, 0.0f, 0.5f, "%.3f");
      ImGui::SliderInt("Max Brick Size##legobricks",
                       &e->legoBricks.maxBrickSize, 1, 4);
      ModulatableSliderAngleDeg("Light Angle##legobricks",
                                &e->legoBricks.lightAngle,
                                "legoBricks.lightAngle", modSources);
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
  DrawGraphicDiscoBall(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGraphicLegoBricks(e, modSources, categoryGlow);
}
