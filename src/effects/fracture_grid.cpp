#include "fracture_grid.h"

#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool FractureGridEffectInit(FractureGridEffect *e) {
  e->shader = LoadShader(NULL, "shaders/fracture_grid.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->subdivisionLoc = GetShaderLocation(e->shader, "subdivision");
  e->staggerLoc = GetShaderLocation(e->shader, "stagger");
  e->offsetScaleLoc = GetShaderLocation(e->shader, "offsetScale");
  e->rotationScaleLoc = GetShaderLocation(e->shader, "rotationScale");
  e->zoomScaleLoc = GetShaderLocation(e->shader, "zoomScale");
  e->tessellationLoc = GetShaderLocation(e->shader, "tessellation");
  e->waveTimeLoc = GetShaderLocation(e->shader, "waveTime");
  e->waveTime = 0.0f;

  return true;
}

void FractureGridEffectSetup(FractureGridEffect *e,
                             const FractureGridConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight) {
  e->waveTime += cfg->waveSpeed * deltaTime;

  float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->subdivisionLoc, &cfg->subdivision,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->staggerLoc, &cfg->stagger, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetScaleLoc, &cfg->offsetScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationScaleLoc, &cfg->rotationScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->zoomScaleLoc, &cfg->zoomScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tessellationLoc, &cfg->tessellation,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->waveTimeLoc, &e->waveTime, SHADER_UNIFORM_FLOAT);
}

void FractureGridEffectUninit(FractureGridEffect *e) {
  UnloadShader(e->shader);
}

FractureGridConfig FractureGridConfigDefault(void) {
  return FractureGridConfig{};
}

void FractureGridRegisterParams(FractureGridConfig *cfg) {
  ModEngineRegisterParam("fractureGrid.subdivision", &cfg->subdivision, 0.0f,
                         20.0f);
  ModEngineRegisterParam("fractureGrid.stagger", &cfg->stagger, 0.0f, 1.0f);
  ModEngineRegisterParam("fractureGrid.offsetScale", &cfg->offsetScale, 0.0f,
                         1.0f);
  ModEngineRegisterParam("fractureGrid.rotationScale", &cfg->rotationScale,
                         0.0f, PI_F);
  ModEngineRegisterParam("fractureGrid.zoomScale", &cfg->zoomScale, 0.0f, 4.0f);
  ModEngineRegisterParam("fractureGrid.waveSpeed", &cfg->waveSpeed, 0.0f, 5.0f);
}

void SetupFractureGrid(PostEffect *pe) {
  FractureGridEffectSetup(&pe->fractureGrid, &pe->effects.fractureGrid,
                          pe->currentDeltaTime, pe->screenWidth,
                          pe->screenHeight);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_FRACTURE_GRID, FractureGrid, fractureGrid, "Fracture Grid",
                "CELL", 2, EFFECT_FLAG_NONE, SetupFractureGrid, NULL)
// clang-format on
