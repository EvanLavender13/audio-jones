// Toon cartoon-style effect module implementation

#include "toon.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void ToonEffectSetup(ToonEffect *e, const ToonConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
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

void ToonEffectUninit(ToonEffect *e) { UnloadShader(e->shader); }

ToonConfig ToonConfigDefault(void) { return ToonConfig{}; }

void ToonRegisterParams(ToonConfig *cfg) { (void)cfg; }

void SetupToon(PostEffect *pe) {
  ToonEffectSetup(&pe->toon, &pe->effects.toon);
}
// clang-format off
REGISTER_EFFECT(TRANSFORM_TOON, Toon, toon, "Toon", "GFX", 5,
                EFFECT_FLAG_NONE, SetupToon, NULL)
// clang-format on
