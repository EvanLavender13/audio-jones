#include "chladni_warp.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

ChladniWarpConfig ChladniWarpConfigDefault(void) { return ChladniWarpConfig{}; }

void ChladniWarpRegisterParams(ChladniWarpConfig *cfg) {
  ModEngineRegisterParam("chladniWarp.n", &cfg->n, 1.0f, 12.0f);
  ModEngineRegisterParam("chladniWarp.m", &cfg->m, 1.0f, 12.0f);
  ModEngineRegisterParam("chladniWarp.strength", &cfg->strength, 0.0f, 0.5f);
  ModEngineRegisterParam("chladniWarp.animRange", &cfg->animRange, 0.0f, 5.0f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CHLADNI_WARP, ChladniWarp, chladniWarp,
                "Chladni Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupChladniWarp, NULL)
// clang-format on
