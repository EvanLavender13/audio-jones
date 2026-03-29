// Toon cartoon-style effect module implementation

#include "toon.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool ToonEffectInit(ToonEffect *e) {
  e->shader = LoadShader(NULL, "shaders/toon.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->levelsLoc = GetShaderLocation(e->shader, "levels");
  e->edgeThresholdLoc = GetShaderLocation(e->shader, "edgeThreshold");
  e->edgeSoftnessLoc = GetShaderLocation(e->shader, "edgeSoftness");
  e->thicknessVariationLoc = GetShaderLocation(e->shader, "thicknessVariation");
  e->noiseScaleLoc = GetShaderLocation(e->shader, "noiseScale");

  return true;
}

void ToonEffectSetup(const ToonEffect *e, const ToonConfig *cfg) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->levelsLoc, &cfg->levels, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeThresholdLoc, &cfg->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeSoftnessLoc, &cfg->edgeSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thicknessVariationLoc, &cfg->thicknessVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseScaleLoc, &cfg->noiseScale,
                 SHADER_UNIFORM_FLOAT);
}

void ToonEffectUninit(const ToonEffect *e) { UnloadShader(e->shader); }

void ToonRegisterParams(ToonConfig *cfg) { (void)cfg; }

void SetupToon(PostEffect *pe) {
  ToonEffectSetup(&pe->toon, &pe->effects.toon);
}

// === UI ===

static void DrawToonParams(EffectConfig *e, const ModSources *ms, ImU32 glow) {
  (void)ms;
  (void)glow;
  ToonConfig *t = &e->toon;

  ImGui::SliderInt("Levels##toon", &t->levels, 2, 16);
  ImGui::SliderFloat("Edge Threshold##toon", &t->edgeThreshold, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Edge Softness##toon", &t->edgeSoftness, 0.0f, 0.2f,
                     "%.3f");

  ImGui::SeparatorText("Brush Stroke");
  ImGui::SliderFloat("Thickness Variation##toon", &t->thicknessVariation, 0.0f,
                     1.0f, "%.2f");
  ImGui::SliderFloat("Noise Scale##toon", &t->noiseScale, 1.0f, 20.0f, "%.1f");
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_TOON, Toon, toon, "Toon", "PRT", 5,
                EFFECT_FLAG_NONE, SetupToon, NULL, DrawToonParams)
// clang-format on
