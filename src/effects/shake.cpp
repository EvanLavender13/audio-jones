// Shake effect module implementation

#include "shake.h"

#include "automation/modulation_engine.h"
#include <stddef.h>

bool ShakeEffectInit(ShakeEffect *e) {
  e->shader = LoadShader(NULL, "shaders/shake.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->rateLoc = GetShaderLocation(e->shader, "rate");
  e->gaussianLoc = GetShaderLocation(e->shader, "gaussian");

  e->time = 0.0f;

  return true;
}

void ShakeEffectSetup(ShakeEffect *e, const ShakeConfig *cfg, float deltaTime) {
  e->time += deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);

  int samples = (int)cfg->samples;
  SetShaderValue(e->shader, e->samplesLoc, &samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->rateLoc, &cfg->rate, SHADER_UNIFORM_FLOAT);

  int gaussian = cfg->gaussian ? 1 : 0;
  SetShaderValue(e->shader, e->gaussianLoc, &gaussian, SHADER_UNIFORM_INT);
}

void ShakeEffectUninit(ShakeEffect *e) { UnloadShader(e->shader); }

ShakeConfig ShakeConfigDefault(void) { return ShakeConfig{}; }

void ShakeRegisterParams(ShakeConfig *cfg) {
  ModEngineRegisterParam("shake.intensity", &cfg->intensity, 0.0f, 0.2f);
  ModEngineRegisterParam("shake.rate", &cfg->rate, 1.0f, 60.0f);
  ModEngineRegisterParam("shake.samples", &cfg->samples, 1.0f, 16.0f);
}
