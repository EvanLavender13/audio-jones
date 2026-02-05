// Watercolor effect module implementation

#include "watercolor.h"
#include <stddef.h>

bool WatercolorEffectInit(WatercolorEffect *e) {
  e->shader = LoadShader(NULL, "shaders/watercolor.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->strokeStepLoc = GetShaderLocation(e->shader, "strokeStep");
  e->washStrengthLoc = GetShaderLocation(e->shader, "washStrength");
  e->paperScaleLoc = GetShaderLocation(e->shader, "paperScale");
  e->paperStrengthLoc = GetShaderLocation(e->shader, "paperStrength");
  e->edgePoolLoc = GetShaderLocation(e->shader, "edgePool");
  e->flowCenterLoc = GetShaderLocation(e->shader, "flowCenter");
  e->flowWidthLoc = GetShaderLocation(e->shader, "flowWidth");

  return true;
}

void WatercolorEffectSetup(WatercolorEffect *e, const WatercolorConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strokeStepLoc, &cfg->strokeStep,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->washStrengthLoc, &cfg->washStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperScaleLoc, &cfg->paperScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->paperStrengthLoc, &cfg->paperStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgePoolLoc, &cfg->edgePool,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowCenterLoc, &cfg->flowCenter,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flowWidthLoc, &cfg->flowWidth,
                 SHADER_UNIFORM_FLOAT);
}

void WatercolorEffectUninit(WatercolorEffect *e) { UnloadShader(e->shader); }

WatercolorConfig WatercolorConfigDefault(void) {
  WatercolorConfig cfg = {};
  cfg.enabled = false;
  cfg.samples = 24;
  cfg.strokeStep = 1.0f;
  cfg.washStrength = 0.7f;
  cfg.paperScale = 8.0f;
  cfg.paperStrength = 0.4f;
  cfg.edgePool = 0.3f;
  cfg.flowCenter = 0.9f;
  cfg.flowWidth = 0.2f;
  return cfg;
}
