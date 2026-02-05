// Impressionist effect module implementation

#include "impressionist.h"
#include <stddef.h>

bool ImpressionistEffectInit(ImpressionistEffect *e) {
  e->shader = LoadShader(NULL, "shaders/impressionist.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->splatCountLoc = GetShaderLocation(e->shader, "splatCount");
  e->splatSizeMinLoc = GetShaderLocation(e->shader, "splatSizeMin");
  e->splatSizeMaxLoc = GetShaderLocation(e->shader, "splatSizeMax");
  e->strokeFreqLoc = GetShaderLocation(e->shader, "strokeFreq");
  e->strokeOpacityLoc = GetShaderLocation(e->shader, "strokeOpacity");
  e->outlineStrengthLoc = GetShaderLocation(e->shader, "outlineStrength");
  e->edgeStrengthLoc = GetShaderLocation(e->shader, "edgeStrength");
  e->edgeMaxDarkenLoc = GetShaderLocation(e->shader, "edgeMaxDarken");
  e->grainScaleLoc = GetShaderLocation(e->shader, "grainScale");
  e->grainAmountLoc = GetShaderLocation(e->shader, "grainAmount");
  e->exposureLoc = GetShaderLocation(e->shader, "exposure");

  return true;
}

void ImpressionistEffectSetup(ImpressionistEffect *e,
                              const ImpressionistConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->splatCountLoc, &cfg->splatCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->splatSizeMinLoc, &cfg->splatSizeMin,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->splatSizeMaxLoc, &cfg->splatSizeMax,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeFreqLoc, &cfg->strokeFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strokeOpacityLoc, &cfg->strokeOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->outlineStrengthLoc, &cfg->outlineStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeStrengthLoc, &cfg->edgeStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeMaxDarkenLoc, &cfg->edgeMaxDarken,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainScaleLoc, &cfg->grainScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->grainAmountLoc, &cfg->grainAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->exposureLoc, &cfg->exposure,
                 SHADER_UNIFORM_FLOAT);
}

void ImpressionistEffectUninit(ImpressionistEffect *e) {
  UnloadShader(e->shader);
}

ImpressionistConfig ImpressionistConfigDefault(void) {
  ImpressionistConfig cfg = {};
  cfg.enabled = false;
  cfg.splatCount = 11;
  cfg.splatSizeMin = 0.018f;
  cfg.splatSizeMax = 0.1f;
  cfg.strokeFreq = 1200.0f;
  cfg.strokeOpacity = 0.7f;
  cfg.outlineStrength = 1.0f;
  cfg.edgeStrength = 4.0f;
  cfg.edgeMaxDarken = 0.13f;
  cfg.grainScale = 400.0f;
  cfg.grainAmount = 0.1f;
  cfg.exposure = 1.28f;
  return cfg;
}
