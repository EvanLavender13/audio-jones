// Lattice Fold: Grid-based tiling symmetry (square, hexagon)

#include "lattice_fold.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include "render/shader_setup_cellular.h"
#include <stdlib.h>

bool LatticeFoldEffectInit(LatticeFoldEffect *e) {
  e->shader = LoadShader(NULL, "shaders/lattice_fold.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->cellTypeLoc = GetShaderLocation(e->shader, "cellType");
  e->cellScaleLoc = GetShaderLocation(e->shader, "cellScale");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->smoothingLoc = GetShaderLocation(e->shader, "smoothing");
  e->rotation = 0.0f;

  return true;
}

void LatticeFoldEffectSetup(LatticeFoldEffect *e, const LatticeFoldConfig *cfg,
                            float deltaTime, float transformTime) {
  e->rotation += cfg->rotationSpeed * deltaTime;

  SetShaderValue(e->shader, e->cellTypeLoc, &cfg->cellType, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->cellScaleLoc, &cfg->cellScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &e->rotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &transformTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->smoothingLoc, &cfg->smoothing,
                 SHADER_UNIFORM_FLOAT);
}

void LatticeFoldEffectUninit(LatticeFoldEffect *e) { UnloadShader(e->shader); }

LatticeFoldConfig LatticeFoldConfigDefault(void) { return LatticeFoldConfig{}; }

void LatticeFoldRegisterParams(LatticeFoldConfig *cfg) {
  ModEngineRegisterParam("latticeFold.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("latticeFold.cellScale", &cfg->cellScale, 1.0f, 20.0f);
  ModEngineRegisterParam("latticeFold.smoothing", &cfg->smoothing, 0.0f, 0.5f);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_LATTICE_FOLD, LatticeFold, latticeFold,
                "Lattice Fold", "CELL", 2, EFFECT_FLAG_NONE,
                SetupLatticeFold, NULL)
// clang-format on
