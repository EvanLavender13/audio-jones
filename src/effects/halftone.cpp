// Halftone effect module implementation

#include "halftone.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void HalftoneRegisterParams(HalftoneConfig *cfg) {
  ModEngineRegisterParam("halftone.dotScale", &cfg->dotScale, 2.0f, 20.0f);
  ModEngineRegisterParam("halftone.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("halftone.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

void SetupHalftone(PostEffect *pe) {
  HalftoneEffectSetup(&pe->halftone, &pe->effects.halftone,
                      pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_HALFTONE, Halftone, halftone, "Halftone", "GFX", 5,
                EFFECT_FLAG_NONE, SetupHalftone, NULL)
// clang-format on
