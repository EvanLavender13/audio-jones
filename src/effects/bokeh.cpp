// Bokeh depth-of-field effect module implementation

#include "bokeh.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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
  e->shapeLoc = GetShaderLocation(e->shader, "shape");
  e->shapeAngleLoc = GetShaderLocation(e->shader, "shapeAngle");
  e->starPointsLoc = GetShaderLocation(e->shader, "starPoints");
  e->starInnerRadiusLoc = GetShaderLocation(e->shader, "starInnerRadius");

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
  SetShaderValue(e->shader, e->shapeLoc, &cfg->shape, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->shapeAngleLoc, &cfg->shapeAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starPointsLoc, &cfg->starPoints,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->starInnerRadiusLoc, &cfg->starInnerRadius,
                 SHADER_UNIFORM_FLOAT);
}

void BokehEffectUninit(BokehEffect *e) { UnloadShader(e->shader); }

BokehConfig BokehConfigDefault(void) { return BokehConfig{}; }

void BokehRegisterParams(BokehConfig *cfg) {
  ModEngineRegisterParam("bokeh.radius", &cfg->radius, 0.0f, 0.1f);
  ModEngineRegisterParam("bokeh.brightnessPower", &cfg->brightnessPower, 1.0f,
                         8.0f);
  ModEngineRegisterParam("bokeh.shapeAngle", &cfg->shapeAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("bokeh.starInnerRadius", &cfg->starInnerRadius, 0.1f,
                         0.9f);
}

void SetupBokeh(PostEffect *pe) {
  BokehEffectSetup(&pe->bokeh, &pe->effects.bokeh);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_BOKEH, Bokeh, bokeh, "Bokeh", "OPT", 7,
                EFFECT_FLAG_NONE, SetupBokeh, NULL)
// clang-format on
