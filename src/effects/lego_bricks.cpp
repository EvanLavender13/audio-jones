// LEGO bricks effect module implementation

#include "lego_bricks.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool LegoBricksEffectInit(LegoBricksEffect *e) {
  e->shader = LoadShader(NULL, "shaders/lego_bricks.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->brickScaleLoc = GetShaderLocation(e->shader, "brickScale");
  e->studHeightLoc = GetShaderLocation(e->shader, "studHeight");
  e->edgeShadowLoc = GetShaderLocation(e->shader, "edgeShadow");
  e->colorThresholdLoc = GetShaderLocation(e->shader, "colorThreshold");
  e->maxBrickSizeLoc = GetShaderLocation(e->shader, "maxBrickSize");
  e->lightAngleLoc = GetShaderLocation(e->shader, "lightAngle");

  return true;
}

void LegoBricksEffectSetup(LegoBricksEffect *e, const LegoBricksConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->brickScaleLoc, &cfg->brickScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->studHeightLoc, &cfg->studHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeShadowLoc, &cfg->edgeShadow,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorThresholdLoc, &cfg->colorThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxBrickSizeLoc, &cfg->maxBrickSize,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lightAngleLoc, &cfg->lightAngle,
                 SHADER_UNIFORM_FLOAT);
}

void LegoBricksEffectUninit(LegoBricksEffect *e) { UnloadShader(e->shader); }

LegoBricksConfig LegoBricksConfigDefault(void) { return LegoBricksConfig{}; }

void LegoBricksRegisterParams(LegoBricksConfig *cfg) {
  ModEngineRegisterParam("legoBricks.brickScale", &cfg->brickScale, 0.01f,
                         0.2f);
  ModEngineRegisterParam("legoBricks.studHeight", &cfg->studHeight, 0.0f, 1.0f);
  ModEngineRegisterParam("legoBricks.lightAngle", &cfg->lightAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

void SetupLegoBricks(PostEffect *pe) {
  LegoBricksEffectSetup(&pe->legoBricks, &pe->effects.legoBricks);
}
// clang-format off
REGISTER_EFFECT(TRANSFORM_LEGO_BRICKS, LegoBricks, legoBricks, "LEGO Bricks",
                "GFX", 5, EFFECT_FLAG_NONE, SetupLegoBricks, NULL)
// clang-format on
