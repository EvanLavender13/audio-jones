// Lattice crush effect module implementation
// Lattice-based coordinate folding that crushes the image into crystalline
// cells

#include "lattice_crush.h"

#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

bool LatticeCrushEffectInit(LatticeCrushEffect *e) {
  e->shader = LoadShader(NULL, "shaders/lattice_crush.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->centerLoc = GetShaderLocation(e->shader, "center");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->cellSizeLoc = GetShaderLocation(e->shader, "cellSize");
  e->iterationsLoc = GetShaderLocation(e->shader, "iterations");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->walkModeLoc = GetShaderLocation(e->shader, "walkMode");
  e->mixLoc = GetShaderLocation(e->shader, "mixAmount");

  e->time = 0.0f;

  return true;
}

void LatticeCrushEffectSetup(LatticeCrushEffect *e,
                             const LatticeCrushConfig *cfg, float deltaTime) {
  e->time += cfg->speed * deltaTime;

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float center[2] = {0.5f, 0.5f};
  SetShaderValue(e->shader, e->centerLoc, center, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellSizeLoc, &cfg->cellSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->iterationsLoc, &cfg->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->walkModeLoc, &cfg->walkMode, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->mixLoc, &cfg->mix, SHADER_UNIFORM_FLOAT);
}

void LatticeCrushEffectUninit(LatticeCrushEffect *e) {
  UnloadShader(e->shader);
}

LatticeCrushConfig LatticeCrushConfigDefault(void) {
  return LatticeCrushConfig{};
}

void LatticeCrushRegisterParams(LatticeCrushConfig *cfg) {
  ModEngineRegisterParam("latticeCrush.scale", &cfg->scale, 0.05f, 1.0f);
  ModEngineRegisterParam("latticeCrush.cellSize", &cfg->cellSize, 2.0f, 32.0f);
  ModEngineRegisterParam("latticeCrush.speed", &cfg->speed, 0.1f, 5.0f);
  ModEngineRegisterParam("latticeCrush.mix", &cfg->mix, 0.0f, 1.0f);
}

void SetupLatticeCrush(PostEffect *pe) {
  LatticeCrushEffectSetup(&pe->latticeCrush, &pe->effects.latticeCrush,
                          pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_LATTICE_CRUSH, LatticeCrush, latticeCrush,
                "Lattice Crush", "RET", 6, EFFECT_FLAG_NONE,
                SetupLatticeCrush, NULL)
// clang-format on
