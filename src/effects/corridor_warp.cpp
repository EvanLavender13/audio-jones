#include "corridor_warp.h"

#include <stddef.h>

bool CorridorWarpEffectInit(CorridorWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/corridor_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->horizonLoc = GetShaderLocation(e->shader, "horizon");
  e->perspectiveStrengthLoc =
      GetShaderLocation(e->shader, "perspectiveStrength");
  e->modeLoc = GetShaderLocation(e->shader, "mode");
  e->viewRotationLoc = GetShaderLocation(e->shader, "viewRotation");
  e->planeRotationLoc = GetShaderLocation(e->shader, "planeRotation");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->scrollOffsetLoc = GetShaderLocation(e->shader, "scrollOffset");
  e->strafeOffsetLoc = GetShaderLocation(e->shader, "strafeOffset");
  e->fogStrengthLoc = GetShaderLocation(e->shader, "fogStrength");

  e->viewRotation = 0.0f;
  e->planeRotation = 0.0f;
  e->scrollOffset = 0.0f;
  e->strafeOffset = 0.0f;

  return true;
}

void CorridorWarpEffectSetup(CorridorWarpEffect *e,
                             const CorridorWarpConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight) {
  e->viewRotation += cfg->viewRotationSpeed * deltaTime;
  e->planeRotation += cfg->planeRotationSpeed * deltaTime;
  e->scrollOffset += cfg->scrollSpeed * deltaTime;
  e->strafeOffset += cfg->strafeSpeed * deltaTime;

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->horizonLoc, &cfg->horizon, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perspectiveStrengthLoc,
                 &cfg->perspectiveStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->viewRotationLoc, &e->viewRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->planeRotationLoc, &e->planeRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollOffsetLoc, &e->scrollOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->strafeOffsetLoc, &e->strafeOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->fogStrengthLoc, &cfg->fogStrength,
                 SHADER_UNIFORM_FLOAT);
}

void CorridorWarpEffectUninit(CorridorWarpEffect *e) {
  UnloadShader(e->shader);
}

CorridorWarpConfig CorridorWarpConfigDefault(void) {
  CorridorWarpConfig cfg;
  cfg.enabled = false;
  cfg.horizon = 0.5f;
  cfg.perspectiveStrength = 1.0f;
  cfg.mode = CORRIDOR_WARP_CORRIDOR;
  cfg.viewRotationSpeed = 0.0f;
  cfg.planeRotationSpeed = 0.0f;
  cfg.scale = 2.0f;
  cfg.scrollSpeed = 0.5f;
  cfg.strafeSpeed = 0.0f;
  cfg.fogStrength = 1.0f;
  return cfg;
}
