// Ink wash effect module implementation

#include "ink_wash.h"
#include <stddef.h>

bool InkWashEffectInit(InkWashEffect *e) {
  e->shader = LoadShader(NULL, "shaders/ink_wash.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->granulationLoc = GetShaderLocation(e->shader, "granulation");
  e->bleedStrengthLoc = GetShaderLocation(e->shader, "bleedStrength");
  e->bleedRadiusLoc = GetShaderLocation(e->shader, "bleedRadius");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");

  return true;
}

void InkWashEffectSetup(InkWashEffect *e, const InkWashConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->granulationLoc, &cfg->granulation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bleedStrengthLoc, &cfg->bleedStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bleedRadiusLoc, &cfg->bleedRadius,
                 SHADER_UNIFORM_FLOAT);

  // Cast float softness to int before sending to shader
  int softness = (int)cfg->softness;
  SetShaderValue(e->shader, e->softnessLoc, &softness, SHADER_UNIFORM_INT);
}

void InkWashEffectUninit(InkWashEffect *e) { UnloadShader(e->shader); }

InkWashConfig InkWashConfigDefault(void) { return InkWashConfig{}; }
