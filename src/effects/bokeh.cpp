// Bokeh depth-of-field effect module implementation

#include "bokeh.h"
#include <stddef.h>

bool BokehEffectInit(BokehEffect *e) {
  e->shader = LoadShader(NULL, "shaders/bokeh.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->brightnessPowerLoc = GetShaderLocation(e->shader, "brightnessPower");

  return true;
}

void BokehEffectSetup(BokehEffect *e, const BokehConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->radiusLoc, &cfg->radius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->brightnessPowerLoc, &cfg->brightnessPower,
                 SHADER_UNIFORM_FLOAT);
}

void BokehEffectUninit(BokehEffect *e) { UnloadShader(e->shader); }

BokehConfig BokehConfigDefault(void) { return BokehConfig{}; }
