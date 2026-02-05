#include "radial_streak.h"
#include "automation/modulation_engine.h"

#include <stddef.h>

bool RadialStreakEffectInit(RadialStreakEffect *e) {
  e->shader = LoadShader(NULL, "shaders/radial_streak.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->samplesLoc = GetShaderLocation(e->shader, "samples");
  e->streakLengthLoc = GetShaderLocation(e->shader, "streakLength");

  return true;
}

void RadialStreakEffectSetup(RadialStreakEffect *e,
                             const RadialStreakConfig *cfg, float deltaTime) {
  (void)deltaTime;

  SetShaderValue(e->shader, e->samplesLoc, &cfg->samples, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->streakLengthLoc, &cfg->streakLength,
                 SHADER_UNIFORM_FLOAT);
}

void RadialStreakEffectUninit(RadialStreakEffect *e) {
  UnloadShader(e->shader);
}

RadialStreakConfig RadialStreakConfigDefault(void) {
  return RadialStreakConfig{};
}

void RadialStreakRegisterParams(RadialStreakConfig *cfg) { (void)cfg; }
