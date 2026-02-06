// Multi-Scale Grid: Layered grid overlay with drift, warp, and edge glow

#include "multi_scale_grid.h"
#include "automation/modulation_engine.h"
#include <stdlib.h>

bool MultiScaleGridEffectInit(MultiScaleGridEffect *e) {
  e->shader = LoadShader(NULL, "shaders/multi_scale_grid.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->scale1Loc = GetShaderLocation(e->shader, "scale1");
  e->scale2Loc = GetShaderLocation(e->shader, "scale2");
  e->scale3Loc = GetShaderLocation(e->shader, "scale3");
  e->scrollSpeedLoc = GetShaderLocation(e->shader, "scrollSpeed");
  e->warpAmountLoc = GetShaderLocation(e->shader, "warpAmount");
  e->edgeContrastLoc = GetShaderLocation(e->shader, "edgeContrast");
  e->edgePowerLoc = GetShaderLocation(e->shader, "edgePower");
  e->glowThresholdLoc = GetShaderLocation(e->shader, "glowThreshold");
  e->glowAmountLoc = GetShaderLocation(e->shader, "glowAmount");
  e->glowModeLoc = GetShaderLocation(e->shader, "glowMode");
  e->cellVariationLoc = GetShaderLocation(e->shader, "cellVariation");
  e->timeLoc = GetShaderLocation(e->shader, "time");

  return true;
}

void MultiScaleGridEffectSetup(MultiScaleGridEffect *e,
                               const MultiScaleGridConfig *cfg, float deltaTime,
                               float transformTime) {
  SetShaderValue(e->shader, e->scale1Loc, &cfg->scale1, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scale2Loc, &cfg->scale2, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scale3Loc, &cfg->scale3, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollSpeedLoc, &cfg->scrollSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpAmountLoc, &cfg->warpAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeContrastLoc, &cfg->edgeContrast,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgePowerLoc, &cfg->edgePower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowThresholdLoc, &cfg->glowThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowAmountLoc, &cfg->glowAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowModeLoc, &cfg->glowMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->cellVariationLoc, &cfg->cellVariation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &transformTime, SHADER_UNIFORM_FLOAT);
}

void MultiScaleGridEffectUninit(MultiScaleGridEffect *e) {
  UnloadShader(e->shader);
}

MultiScaleGridConfig MultiScaleGridConfigDefault(void) {
  return MultiScaleGridConfig{};
}

void MultiScaleGridRegisterParams(MultiScaleGridConfig *cfg) {
  ModEngineRegisterParam("multiScaleGrid.scale1", &cfg->scale1, 5.0f, 20.0f);
  ModEngineRegisterParam("multiScaleGrid.scale2", &cfg->scale2, 15.0f, 40.0f);
  ModEngineRegisterParam("multiScaleGrid.scale3", &cfg->scale3, 30.0f, 80.0f);
  ModEngineRegisterParam("multiScaleGrid.scrollSpeed", &cfg->scrollSpeed, 0.0f,
                         0.1f);
  ModEngineRegisterParam("multiScaleGrid.warpAmount", &cfg->warpAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("multiScaleGrid.edgeContrast", &cfg->edgeContrast,
                         0.0f, 0.5f);
  ModEngineRegisterParam("multiScaleGrid.edgePower", &cfg->edgePower, 1.0f,
                         5.0f);
  ModEngineRegisterParam("multiScaleGrid.glowThreshold", &cfg->glowThreshold,
                         0.1f, 1.0f);
  ModEngineRegisterParam("multiScaleGrid.glowAmount", &cfg->glowAmount, 1.0f,
                         4.0f);
  ModEngineRegisterParam("multiScaleGrid.cellVariation", &cfg->cellVariation,
                         0.0f, 1.0f);
}
