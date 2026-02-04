#include "surface_warp.h"

#include <stddef.h>

bool SurfaceWarpEffectInit(SurfaceWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/surface_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->angleLoc = GetShaderLocation(e->shader, "angle");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->scrollOffsetLoc = GetShaderLocation(e->shader, "scrollOffset");
  e->depthShadeLoc = GetShaderLocation(e->shader, "depthShade");

  e->rotation = 0.0f;
  e->scrollOffset = 0.0f;

  return true;
}

void SurfaceWarpEffectSetup(SurfaceWarpEffect *e, const SurfaceWarpConfig *cfg,
                            float deltaTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;
  e->scrollOffset += cfg->scrollSpeed * deltaTime;

  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->angleLoc, &cfg->angle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollOffsetLoc, &e->scrollOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->depthShadeLoc, &cfg->depthShade,
                 SHADER_UNIFORM_FLOAT);
}

void SurfaceWarpEffectUninit(SurfaceWarpEffect *e) { UnloadShader(e->shader); }

SurfaceWarpConfig SurfaceWarpConfigDefault(void) {
  SurfaceWarpConfig cfg;
  cfg.enabled = false;
  cfg.intensity = 0.5f;
  cfg.angle = 0.0f;
  cfg.rotationSpeed = 0.0f;
  cfg.scrollSpeed = 0.5f;
  cfg.depthShade = 0.3f;
  return cfg;
}
