#include "interference_warp.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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

void InterferenceWarpRegisterParams(InterferenceWarpConfig *cfg) {
  ModEngineRegisterParam("interferenceWarp.amplitude", &cfg->amplitude, 0.0f,
                         0.5f);
  ModEngineRegisterParam("interferenceWarp.axisRotationSpeed",
                         &cfg->axisRotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("interferenceWarp.decay", &cfg->decay, 0.5f, 2.0f);
  ModEngineRegisterParam("interferenceWarp.scale", &cfg->scale, 0.5f, 10.0f);
  ModEngineRegisterParam("interferenceWarp.speed", &cfg->speed, 0.0f, 0.01f);
}

void SetupInterferenceWarp(PostEffect *pe) {
  InterferenceWarpEffectSetup(&pe->interferenceWarp,
                              &pe->effects.interferenceWarp,
                              pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_INTERFERENCE_WARP, InterferenceWarp, interferenceWarp,
                "Interference Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupInterferenceWarp, NULL)
// clang-format on
