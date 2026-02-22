#include "lens_space.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool LensSpaceEffectInit(LensSpaceEffect *e) {
  e->shader = LoadShader(NULL, "shaders/lens_space.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->sphereOffsetLoc = GetShaderLocation(e->shader, "sphereOffset");
  e->pLoc = GetShaderLocation(e->shader, "p");
  e->qLoc = GetShaderLocation(e->shader, "q");
  e->sphereRadiusLoc = GetShaderLocation(e->shader, "sphereRadius");
  e->boundaryRadiusLoc = GetShaderLocation(e->shader, "boundaryRadius");
  e->rotAngleLoc = GetShaderLocation(e->shader, "rotAngle");
  e->maxReflectionsLoc = GetShaderLocation(e->shader, "maxReflections");
  e->dimmingLoc = GetShaderLocation(e->shader, "dimming");
  e->zoomLoc = GetShaderLocation(e->shader, "zoom");
  e->projScaleLoc = GetShaderLocation(e->shader, "projScale");

  e->rotAngle = 0.0f;

  return true;
}

void LensSpaceEffectSetup(LensSpaceEffect *e, const LensSpaceConfig *cfg,
                          float deltaTime, int screenWidth, int screenHeight) {
  e->rotAngle += cfg->rotationSpeed * deltaTime;

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float center[2] = {cfg->centerX, cfg->centerY};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  float sphereOffset[2] = {cfg->sphereOffsetX, cfg->sphereOffsetY};
  SetShaderValue(e->shader, e->sphereOffsetLoc, sphereOffset,
                 SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->pLoc, &cfg->p, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->qLoc, &cfg->q, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->sphereRadiusLoc, &cfg->sphereRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->boundaryRadiusLoc, &cfg->boundaryRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotAngleLoc, &e->rotAngle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxReflectionsLoc, &cfg->maxReflections,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dimmingLoc, &cfg->dimming, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomLoc, &cfg->zoom, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->projScaleLoc, &cfg->projScale,
                 SHADER_UNIFORM_FLOAT);
}

void LensSpaceEffectUninit(LensSpaceEffect *e) { UnloadShader(e->shader); }

LensSpaceConfig LensSpaceConfigDefault(void) { return LensSpaceConfig{}; }

void LensSpaceRegisterParams(LensSpaceConfig *cfg) {
  ModEngineRegisterParam("lensSpace.centerX", &cfg->centerX, 0.0f, 1.0f);
  ModEngineRegisterParam("lensSpace.centerY", &cfg->centerY, 0.0f, 1.0f);
  ModEngineRegisterParam("lensSpace.p", &cfg->p, 2.0f, 12.0f);
  ModEngineRegisterParam("lensSpace.q", &cfg->q, 1.0f, 11.0f);
  ModEngineRegisterParam("lensSpace.sphereOffsetX", &cfg->sphereOffsetX, -0.5f,
                         0.5f);
  ModEngineRegisterParam("lensSpace.sphereOffsetY", &cfg->sphereOffsetY, -0.5f,
                         0.5f);
  ModEngineRegisterParam("lensSpace.sphereRadius", &cfg->sphereRadius, 0.05f,
                         0.8f);
  ModEngineRegisterParam("lensSpace.boundaryRadius", &cfg->boundaryRadius, 0.5f,
                         2.0f);
  ModEngineRegisterParam("lensSpace.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("lensSpace.maxReflections", &cfg->maxReflections, 2.0f,
                         20.0f);
  ModEngineRegisterParam("lensSpace.dimming", &cfg->dimming, 0.01f, 0.15f);
  ModEngineRegisterParam("lensSpace.zoom", &cfg->zoom, 0.5f, 3.0f);
  ModEngineRegisterParam("lensSpace.projScale", &cfg->projScale, 0.1f, 1.0f);
}

void SetupLensSpace(PostEffect *pe) {
  LensSpaceEffectSetup(&pe->lensSpace, &pe->effects.lensSpace,
                       pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_LENS_SPACE, LensSpace, lensSpace,
                "Lens Space", "WARP", 1, EFFECT_FLAG_NONE,
                SetupLensSpace, NULL)
// clang-format on
