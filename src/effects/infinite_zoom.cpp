#include "infinite_zoom.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
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
  e->layerRotateLoc = GetShaderLocation(e->shader, "layerRotate");
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");

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
  SetShaderValue(e->shader, e->layerRotateLoc, &cfg->layerRotate,
                 SHADER_UNIFORM_FLOAT);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void InfiniteZoomEffectUninit(InfiniteZoomEffect *e) {
  UnloadShader(e->shader);
}

InfiniteZoomConfig InfiniteZoomConfigDefault(void) {
  return InfiniteZoomConfig{};
}

void InfiniteZoomRegisterParams(InfiniteZoomConfig *cfg) {
  ModEngineRegisterParam("infiniteZoom.spiralAngle", &cfg->spiralAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("infiniteZoom.spiralTwist", &cfg->spiralTwist,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("infiniteZoom.layerRotate", &cfg->layerRotate,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}

void SetupInfiniteZoom(PostEffect *pe) {
  InfiniteZoomEffectSetup(&pe->infiniteZoom, &pe->effects.infiniteZoom,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_INFINITE_ZOOM, InfiniteZoom, infiniteZoom,
                "Infinite Zoom", "MOT", 3, EFFECT_FLAG_NONE,
                SetupInfiniteZoom, NULL)
// clang-format on
