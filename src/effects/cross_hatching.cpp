// Cross hatching effect module implementation

#include "cross_hatching.h"
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
  CrossHatchingConfig cfg = {};
  cfg.enabled = false;
  cfg.width = 1.5f;
  cfg.threshold = 1.0f;
  cfg.noise = 0.5f;
  cfg.outline = 0.5f;
  return cfg;
}
