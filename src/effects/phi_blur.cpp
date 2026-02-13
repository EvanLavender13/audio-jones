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
  e->modeLoc = GetShaderLocation(e->shader, "mode");
  e->radiusLoc = GetShaderLocation(e->shader, "radius");
  e->angleLoc = GetShaderLocation(e->shader, "angle");
  e->aspectRatioLoc = GetShaderLocation(e->shader, "aspectRatio");
  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->gammaLoc = GetShaderLocation(e->shader, "gamma");

  return true;
}

void PhiBlurEffectSetup(PhiBlurEffect *e, const PhiBlurConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->radiusLoc, &cfg->radius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->angleLoc, &cfg->angle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->aspectRatioLoc, &cfg->aspectRatio,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->gammaLoc, &cfg->gamma, SHADER_UNIFORM_FLOAT);
}

void PhiBlurEffectUninit(PhiBlurEffect *e) { UnloadShader(e->shader); }

PhiBlurConfig PhiBlurConfigDefault(void) { return PhiBlurConfig{}; }

void PhiBlurRegisterParams(PhiBlurConfig *cfg) {
  ModEngineRegisterParam("phiBlur.radius", &cfg->radius, 0.0f, 50.0f);
  ModEngineRegisterParam("phiBlur.angle", &cfg->angle, 0.0f,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("phiBlur.aspectRatio", &cfg->aspectRatio, 0.1f, 10.0f);
  ModEngineRegisterParam("phiBlur.gamma", &cfg->gamma, 1.0f, 6.0f);
}
