// Lattice crush transform effect
// Lattice-based coordinate folding that crushes the image into crystalline
// cells

#ifndef LATTICE_CRUSH_H
#define LATTICE_CRUSH_H

#include "raylib.h"
#include <stdbool.h>

struct LatticeCrushConfig {
  bool enabled = false;

  float scale = 0.3f;    // Coordinate zoom (0.05-1.0)
  float cellSize = 8.0f; // Grid quantization coarseness (2.0-32.0)
  int iterations = 32;   // Walk steps (4-64)
  float speed = 1.0f;    // Animation rate (0.1-5.0)
  float mix = 1.0f;      // Blend crushed with original (0.0-1.0)
};

#define LATTICE_CRUSH_CONFIG_FIELDS                                            \
  enabled, scale, cellSize, iterations, speed, mix

typedef struct LatticeCrushEffect {
  Shader shader;
  float time; // Accumulated animation time
  int resolutionLoc;
  int centerLoc;
  int scaleLoc;
  int cellSizeLoc;
  int iterationsLoc;
  int timeLoc;
  int mixLoc;
} LatticeCrushEffect;

// Returns true on success, false if shader fails to load
bool LatticeCrushEffectInit(LatticeCrushEffect *e);

// Binds all uniforms and advances animation time
void LatticeCrushEffectSetup(LatticeCrushEffect *e,
                             const LatticeCrushConfig *cfg, float deltaTime);

// Unloads shader
void LatticeCrushEffectUninit(LatticeCrushEffect *e);

// Returns default config
LatticeCrushConfig LatticeCrushConfigDefault(void);

// Registers modulatable params with the modulation engine
void LatticeCrushRegisterParams(LatticeCrushConfig *cfg);

#endif // LATTICE_CRUSH_H
