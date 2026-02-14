// Dot matrix effect module implementation

#include "dot_matrix.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include "render/shader_setup_cellular.h"
#include <stddef.h>

bool DotMatrixEffectInit(DotMatrixEffect *e) {
  e->shader = LoadShader(NULL, "shaders/dot_matrix.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->dotScaleLoc = GetShaderLocation(e->shader, "dotScale");
  e->softnessLoc = GetShaderLocation(e->shader, "softness");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");

  e->rotation = 0.0f;

  return true;
}

void DotMatrixEffectSetup(DotMatrixEffect *e, const DotMatrixConfig *cfg,
                          float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;

  float finalRotation = e->rotation + cfg->rotationAngle;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->dotScaleLoc, &cfg->dotScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->softnessLoc, &cfg->softness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &finalRotation,
                 SHADER_UNIFORM_FLOAT);
}

void DotMatrixEffectUninit(DotMatrixEffect *e) { UnloadShader(e->shader); }

DotMatrixConfig DotMatrixConfigDefault(void) { return DotMatrixConfig{}; }

void DotMatrixRegisterParams(DotMatrixConfig *cfg) {
  ModEngineRegisterParam("dotMatrix.dotScale", &cfg->dotScale, 4.0f, 80.0f);
  ModEngineRegisterParam("dotMatrix.softness", &cfg->softness, 0.2f, 4.0f);
  ModEngineRegisterParam("dotMatrix.brightness", &cfg->brightness, 0.5f, 8.0f);
  ModEngineRegisterParam("dotMatrix.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("dotMatrix.rotationAngle", &cfg->rotationAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DOT_MATRIX, DotMatrix, dotMatrix, "Dot Matrix",
                "CELL", 2, EFFECT_FLAG_NONE, SetupDotMatrix, NULL)
// clang-format on
