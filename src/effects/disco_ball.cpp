// Disco ball effect module implementation

#include "disco_ball.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

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

void DiscoBallRegisterParams(DiscoBallConfig *cfg) {
  ModEngineRegisterParam("discoBall.sphereRadius", &cfg->sphereRadius, 0.2f,
                         1.5f);
  ModEngineRegisterParam("discoBall.tileSize", &cfg->tileSize, 0.05f, 0.3f);
  ModEngineRegisterParam("discoBall.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("discoBall.bumpHeight", &cfg->bumpHeight, 0.0f, 0.2f);
  ModEngineRegisterParam("discoBall.reflectIntensity", &cfg->reflectIntensity,
                         0.5f, 5.0f);
  ModEngineRegisterParam("discoBall.spotIntensity", &cfg->spotIntensity, 0.0f,
                         3.0f);
  ModEngineRegisterParam("discoBall.spotFalloff", &cfg->spotFalloff, 0.5f,
                         3.0f);
  ModEngineRegisterParam("discoBall.brightnessThreshold",
                         &cfg->brightnessThreshold, 0.0f, 0.5f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_DISCO_BALL, DiscoBall, discoBall, "Disco Ball",
                "GFX", 5, EFFECT_FLAG_NONE, SetupDiscoBall, NULL)
// clang-format on
