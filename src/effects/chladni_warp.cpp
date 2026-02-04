#include "chladni_warp.h"

#include <stddef.h>

bool ChladniWarpEffectInit(ChladniWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/chladni_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->nLoc = GetShaderLocation(e->shader, "n");
  e->mLoc = GetShaderLocation(e->shader, "m");
  e->plateSizeLoc = GetShaderLocation(e->shader, "plateSize");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->modeLoc = GetShaderLocation(e->shader, "warpMode");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->animRangeLoc = GetShaderLocation(e->shader, "animRange");
  e->preFoldLoc = GetShaderLocation(e->shader, "preFold");

  e->phase = 0.0f;

  return true;
}

void ChladniWarpEffectSetup(ChladniWarpEffect *e, const ChladniWarpConfig *cfg,
                            float deltaTime) {
  e->phase += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->nLoc, &cfg->n, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->mLoc, &cfg->m, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->plateSizeLoc, &cfg->plateSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->modeLoc, &cfg->warpMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->animPhaseLoc, &e->phase, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->animRangeLoc, &cfg->animRange,
                 SHADER_UNIFORM_FLOAT);

  int preFold = cfg->preFold ? 1 : 0;
  SetShaderValue(e->shader, e->preFoldLoc, &preFold, SHADER_UNIFORM_INT);
}

void ChladniWarpEffectUninit(ChladniWarpEffect *e) { UnloadShader(e->shader); }

ChladniWarpConfig ChladniWarpConfigDefault(void) {
  ChladniWarpConfig cfg;
  cfg.enabled = false;
  cfg.n = 4.0f;
  cfg.m = 3.0f;
  cfg.plateSize = 1.0f;
  cfg.strength = 0.1f;
  cfg.warpMode = 0;
  cfg.speed = 0.0f;
  cfg.animRange = 0.0f;
  cfg.preFold = false;
  return cfg;
}
