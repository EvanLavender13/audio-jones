// Toon cartoon-style effect module implementation

#include "toon.h"
#include <stdlib.h>

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
