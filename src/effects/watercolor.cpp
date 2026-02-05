// Watercolor effect module implementation

#include "watercolor.h"
#include <stddef.h>

bool WatercolorEffectInit(WatercolorEffect *e) {
  e->shader = LoadShader(NULL, "shaders/watercolor.fs");
  if (e->shader.id == 0) {
    return false;
  }

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

WatercolorConfig WatercolorConfigDefault(void) { return WatercolorConfig{}; }
