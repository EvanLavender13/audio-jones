#include "infinite_zoom.h"

#include <stddef.h>

bool InfiniteZoomEffectInit(InfiniteZoomEffect *e) {
  e->shader = LoadShader(NULL, "shaders/infinite_zoom.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->zoomDepthLoc = GetShaderLocation(e->shader, "zoomDepth");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->spiralAngleLoc = GetShaderLocation(e->shader, "spiralAngle");
  e->spiralTwistLoc = GetShaderLocation(e->shader, "spiralTwist");

  e->time = 0.0f;

  return true;
}

void InfiniteZoomEffectSetup(InfiniteZoomEffect *e,
                             const InfiniteZoomConfig *cfg, float deltaTime) {
  e->time += cfg->speed * deltaTime;

  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomDepthLoc, &cfg->zoomDepth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->spiralAngleLoc, &cfg->spiralAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spiralTwistLoc, &cfg->spiralTwist,
                 SHADER_UNIFORM_FLOAT);
}

void InfiniteZoomEffectUninit(InfiniteZoomEffect *e) {
  UnloadShader(e->shader);
}

InfiniteZoomConfig InfiniteZoomConfigDefault(void) {
  InfiniteZoomConfig cfg;
  cfg.enabled = false;
  cfg.speed = 1.0f;
  cfg.zoomDepth = 3.0f;
  cfg.layers = 6;
  cfg.spiralAngle = 0.0f;
  cfg.spiralTwist = 0.0f;
  return cfg;
}
