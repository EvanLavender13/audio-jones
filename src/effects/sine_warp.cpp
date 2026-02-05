#include "sine_warp.h"

#include "automation/modulation_engine.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool SineWarpEffectInit(SineWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/sine_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->octavesLoc = GetShaderLocation(e->shader, "octaves");
  e->strengthLoc = GetShaderLocation(e->shader, "strength");
  e->octaveRotationLoc = GetShaderLocation(e->shader, "octaveRotation");
  e->radialModeLoc = GetShaderLocation(e->shader, "radialMode");
  e->depthBlendLoc = GetShaderLocation(e->shader, "depthBlend");

  e->time = 0.0f;

  return true;
}

void SineWarpEffectSetup(SineWarpEffect *e, const SineWarpConfig *cfg,
                         float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octavesLoc, &cfg->octaves, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->strengthLoc, &cfg->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->octaveRotationLoc, &cfg->octaveRotation,
                 SHADER_UNIFORM_FLOAT);

  int radialMode = cfg->radialMode ? 1 : 0;
  SetShaderValue(e->shader, e->radialModeLoc, &radialMode, SHADER_UNIFORM_INT);

  int depthBlend = cfg->depthBlend ? 1 : 0;
  SetShaderValue(e->shader, e->depthBlendLoc, &depthBlend, SHADER_UNIFORM_INT);
}

void SineWarpEffectUninit(SineWarpEffect *e) { UnloadShader(e->shader); }

SineWarpConfig SineWarpConfigDefault(void) { return SineWarpConfig{}; }

void SineWarpRegisterParams(SineWarpConfig *cfg) {
  ModEngineRegisterParam("sineWarp.strength", &cfg->strength, 0.0f, 2.0f);
  ModEngineRegisterParam("sineWarp.octaveRotation", &cfg->octaveRotation,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}
