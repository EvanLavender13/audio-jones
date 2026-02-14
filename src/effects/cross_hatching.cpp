// Cross hatching effect module implementation

#include "cross_hatching.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool CrossHatchingEffectInit(CrossHatchingEffect *e) {
  e->shader = LoadShader(NULL, "shaders/cross_hatching.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->widthLoc = GetShaderLocation(e->shader, "width");
  e->thresholdLoc = GetShaderLocation(e->shader, "threshold");
  e->noiseLoc = GetShaderLocation(e->shader, "noise");
  e->outlineLoc = GetShaderLocation(e->shader, "outline");

  e->time = 0.0f;

  return true;
}

void CrossHatchingEffectSetup(CrossHatchingEffect *e,
                              const CrossHatchingConfig *cfg, float deltaTime) {
  e->time += deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->widthLoc, &cfg->width, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thresholdLoc, &cfg->threshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseLoc, &cfg->noise, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->outlineLoc, &cfg->outline, SHADER_UNIFORM_FLOAT);
}

void CrossHatchingEffectUninit(CrossHatchingEffect *e) {
  UnloadShader(e->shader);
}

CrossHatchingConfig CrossHatchingConfigDefault(void) {
  return CrossHatchingConfig{};
}

void CrossHatchingRegisterParams(CrossHatchingConfig *cfg) {
  ModEngineRegisterParam("crossHatching.width", &cfg->width, 0.5f, 4.0f);
  ModEngineRegisterParam("crossHatching.threshold", &cfg->threshold, 0.0f,
                         2.0f);
  ModEngineRegisterParam("crossHatching.noise", &cfg->noise, 0.0f, 1.0f);
  ModEngineRegisterParam("crossHatching.outline", &cfg->outline, 0.0f, 1.0f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_CROSS_HATCHING, CrossHatching, crossHatching,
                "Cross-Hatching", "ART", 4, EFFECT_FLAG_NONE,
                SetupCrossHatching, NULL)
// clang-format on
