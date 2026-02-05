// Disco ball effect module implementation

#include "disco_ball.h"
#include <stdlib.h>

bool DiscoBallEffectInit(DiscoBallEffect *e) {
  e->shader = LoadShader(NULL, "shaders/disco_ball.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->sphereRadiusLoc = GetShaderLocation(e->shader, "sphereRadius");
  e->tileSizeLoc = GetShaderLocation(e->shader, "tileSize");
  e->sphereAngleLoc = GetShaderLocation(e->shader, "sphereAngle");
  e->bumpHeightLoc = GetShaderLocation(e->shader, "bumpHeight");
  e->reflectIntensityLoc = GetShaderLocation(e->shader, "reflectIntensity");
  e->spotIntensityLoc = GetShaderLocation(e->shader, "spotIntensity");
  e->spotFalloffLoc = GetShaderLocation(e->shader, "spotFalloff");
  e->brightnessThresholdLoc =
      GetShaderLocation(e->shader, "brightnessThreshold");

  e->angle = 0.0f;

  return true;
}

void DiscoBallEffectSetup(DiscoBallEffect *e, const DiscoBallConfig *cfg,
                          float deltaTime) {
  e->angle += cfg->rotationSpeed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->sphereRadiusLoc, &cfg->sphereRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tileSizeLoc, &cfg->tileSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereAngleLoc, &e->angle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bumpHeightLoc, &cfg->bumpHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->reflectIntensityLoc, &cfg->reflectIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spotIntensityLoc, &cfg->spotIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->spotFalloffLoc, &cfg->spotFalloff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessThresholdLoc,
                 &cfg->brightnessThreshold, SHADER_UNIFORM_FLOAT);
}

void DiscoBallEffectUninit(DiscoBallEffect *e) { UnloadShader(e->shader); }

DiscoBallConfig DiscoBallConfigDefault(void) { return DiscoBallConfig{}; }
