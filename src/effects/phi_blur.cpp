// Phi Blur effect module implementation

#include "phi_blur.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include <stddef.h>

bool PhiBlurEffectInit(PhiBlurEffect *e) {
  e->shader = LoadShader(NULL, "shaders/phi_blur.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->shapeLoc = GetShaderLocation(e->shader, "shape");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");
  e->shapeAngleLoc = GetShaderLocation(e->shader, "shapeAngle");
  e->starPointsLoc = GetShaderLocation(e->shader, "starPoints");
  e->starInnerRadiusLoc = GetShaderLocation(e->shader, "starInner");
  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->gammaLoc = GetShaderLocation(e->shader, "gamma");

  return true;
}

void PhiBlurEffectSetup(PhiBlurEffect *e, const PhiBlurConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->shapeLoc, &cfg->shape, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->radiusLoc, &cfg->radius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->shapeAngleLoc, &cfg->shapeAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starPointsLoc, &cfg->starPoints,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->starInnerRadiusLoc, &cfg->starInnerRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gammaLoc, &cfg->gamma, SHADER_UNIFORM_FLOAT);
}

void PhiBlurEffectUninit(PhiBlurEffect *e) { UnloadShader(e->shader); }

PhiBlurConfig PhiBlurConfigDefault(void) { return PhiBlurConfig{}; }

void PhiBlurRegisterParams(PhiBlurConfig *cfg) {
  ModEngineRegisterParam("phiBlur.radius", &cfg->radius, 0.0f, 50.0f);
  ModEngineRegisterParam("phiBlur.shapeAngle", &cfg->shapeAngle, 0.0f,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("phiBlur.starInnerRadius", &cfg->starInnerRadius, 0.1f,
                         0.9f);
  ModEngineRegisterParam("phiBlur.gamma", &cfg->gamma, 1.0f, 6.0f);
}
