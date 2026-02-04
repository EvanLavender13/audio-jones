// Lattice Fold: Grid-based tiling symmetry (square, hexagon)

#ifndef LATTICE_FOLD_H
#define LATTICE_FOLD_H

#include "raylib.h"
#include <stdbool.h>

struct LatticeFoldConfig {
  bool enabled = false;
  int cellType = 6;
  float cellScale = 8.0f;
  float rotationSpeed = 0.0f;
  float smoothing = 0.0f;
};

typedef struct LatticeFoldEffect {
  Shader shader;
  int cellTypeLoc;
  int cellScaleLoc;
  int rotationLoc;
  int timeLoc;
  int smoothingLoc;
  float rotation; // Animation accumulator
} LatticeFoldEffect;

bool LatticeFoldEffectInit(LatticeFoldEffect *e);
void LatticeFoldEffectSetup(LatticeFoldEffect *e, const LatticeFoldConfig *cfg,
                            float deltaTime, float transformTime);
void LatticeFoldEffectUninit(LatticeFoldEffect *e);
LatticeFoldConfig LatticeFoldConfigDefault(void);

#endif // LATTICE_FOLD_H
