#include "interference_warp.h"

#include <stddef.h>

bool InterferenceWarpEffectInit(InterferenceWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/interference_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->amplitudeLoc = GetShaderLocation(e->shader, "amplitude");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->axesLoc = GetShaderLocation(e->shader, "axes");
  e->axisRotationLoc = GetShaderLocation(e->shader, "axisRotation");
  e->harmonicsLoc = GetShaderLocation(e->shader, "harmonics");
  e->decayLoc = GetShaderLocation(e->shader, "decay");
  e->driftLoc = GetShaderLocation(e->shader, "drift");

  e->time = 0.0f;
  e->axisRotation = 0.0f;

  return true;
}

void InterferenceWarpEffectSetup(InterferenceWarpEffect *e,
                                 const InterferenceWarpConfig *cfg,
                                 float deltaTime) {
  e->time += cfg->speed * deltaTime;
  e->axisRotation += cfg->axisRotationSpeed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->amplitudeLoc, &cfg->amplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->axesLoc, &cfg->axes, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->axisRotationLoc, &e->axisRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->harmonicsLoc, &cfg->harmonics,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->decayLoc, &cfg->decay, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftLoc, &cfg->drift, SHADER_UNIFORM_FLOAT);
}

void InterferenceWarpEffectUninit(InterferenceWarpEffect *e) {
  UnloadShader(e->shader);
}

InterferenceWarpConfig InterferenceWarpConfigDefault(void) {
  return InterferenceWarpConfig{};
}
