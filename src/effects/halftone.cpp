// Halftone effect module implementation

#include "halftone.h"
#include <stddef.h>

bool HalftoneEffectInit(HalftoneEffect *e) {
  e->shader = LoadShader(NULL, "shaders/halftone.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->dotScaleLoc = GetShaderLocation(e->shader, "dotScale");
  e->dotSizeLoc = GetShaderLocation(e->shader, "dotSize");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");

  e->rotation = 0.0f;

  return true;
}

void HalftoneEffectSetup(HalftoneEffect *e, const HalftoneConfig *cfg,
                         float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;

  float finalRotation = e->rotation + cfg->rotationAngle;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->dotScaleLoc, &cfg->dotScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dotSizeLoc, &cfg->dotSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &finalRotation,
                 SHADER_UNIFORM_FLOAT);
}

void HalftoneEffectUninit(HalftoneEffect *e) { UnloadShader(e->shader); }

HalftoneConfig HalftoneConfigDefault(void) { return HalftoneConfig{}; }
