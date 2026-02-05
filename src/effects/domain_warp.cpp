#include "domain_warp.h"

#include "automation/modulation_engine.h"
#include "ui/ui_units.h"
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

DomainWarpConfig DomainWarpConfigDefault(void) { return DomainWarpConfig{}; }

void DomainWarpRegisterParams(DomainWarpConfig *cfg) {
  ModEngineRegisterParam("domainWarp.warpStrength", &cfg->warpStrength, 0.0f,
                         0.5f);
  ModEngineRegisterParam("domainWarp.falloff", &cfg->falloff, 0.3f, 0.8f);
  ModEngineRegisterParam("domainWarp.driftSpeed", &cfg->driftSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("domainWarp.driftAngle", &cfg->driftAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}
