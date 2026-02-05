// LEGO bricks effect module implementation

#include "lego_bricks.h"
#include <stdlib.h>

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
