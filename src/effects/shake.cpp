// Shake effect module implementation

#include "shake.h"
#include <stdlib.h>

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

ShakeConfig ShakeConfigDefault(void) {
  ShakeConfig cfg = {};
  cfg.enabled = false;
  cfg.intensity = 0.02f;
  cfg.samples = 4.0f;
  cfg.rate = 12.0f;
  cfg.gaussian = false;
  return cfg;
}
