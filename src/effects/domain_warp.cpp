#include "domain_warp.h"

#include <math.h>
#include <stddef.h>

bool DomainWarpEffectInit(DomainWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/domain_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->warpStrengthLoc = GetShaderLocation(e->shader, "warpStrength");
  e->warpScaleLoc = GetShaderLocation(e->shader, "warpScale");
  e->warpIterationsLoc = GetShaderLocation(e->shader, "warpIterations");
  e->falloffLoc = GetShaderLocation(e->shader, "falloff");
  e->timeOffsetLoc = GetShaderLocation(e->shader, "timeOffset");

  e->drift = 0.0f;

  return true;
}

void DomainWarpEffectSetup(DomainWarpEffect *e, const DomainWarpConfig *cfg,
                           float deltaTime) {
  e->drift += cfg->driftSpeed * deltaTime;

  float timeOffset[2] = {cosf(cfg->driftAngle) * e->drift,
                         sinf(cfg->driftAngle) * e->drift};

  SetShaderValue(e->shader, e->warpStrengthLoc, &cfg->warpStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpScaleLoc, &cfg->warpScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->warpIterationsLoc, &cfg->warpIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->falloffLoc, &cfg->falloff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeOffsetLoc, timeOffset, SHADER_UNIFORM_VEC2);
}

void DomainWarpEffectUninit(DomainWarpEffect *e) { UnloadShader(e->shader); }

DomainWarpConfig DomainWarpConfigDefault(void) {
  DomainWarpConfig cfg;
  cfg.enabled = false;
  cfg.warpStrength = 0.1f;
  cfg.warpScale = 4.0f;
  cfg.warpIterations = 2;
  cfg.falloff = 0.5f;
  cfg.driftSpeed = 0.0f;
  cfg.driftAngle = 0.0f;
  return cfg;
}
