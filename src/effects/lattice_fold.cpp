// Lattice Fold: Grid-based tiling symmetry (square, hexagon)

#include "lattice_fold.h"
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
