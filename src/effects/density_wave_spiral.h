// Density Wave Spiral
// Simulates galactic density wave arms radiating from a configurable center.
// rotationSpeed spins the spiral pattern; globalRotationSpeed rotates the
// entire field. tightness controls arm winding; ringCount sets concentric
// density bands.

#ifndef DENSITY_WAVE_SPIRAL_H
#define DENSITY_WAVE_SPIRAL_H

#include "raylib.h"
#include <stdbool.h>

struct DensityWaveSpiralConfig {
  bool enabled = false;
  float centerX = 0.0f;
  float centerY = 0.0f;
  float aspectX = 0.5f;
  float aspectY = 0.3f;
  float tightness = -1.5708f;
  float rotationSpeed = 0.5f;
  float globalRotationSpeed = 0.0f;
  float thickness = 0.3f;
  int ringCount = 30;
  float falloff = 1.0f;
};

typedef struct DensityWaveSpiralEffect {
  Shader shader;
  int centerLoc;
  int aspectLoc;
  int tightnessLoc;
  int rotationAccumLoc;
  int globalRotationAccumLoc;
  int thicknessLoc;
  int ringCountLoc;
  int falloffLoc;
  float rotation;       // Spiral rotation accumulator
  float globalRotation; // Global rotation accumulator
} DensityWaveSpiralEffect;

// Returns true on success, false if shader fails to load
bool DensityWaveSpiralEffectInit(DensityWaveSpiralEffect *e);

// Accumulates rotation, sets all uniforms
void DensityWaveSpiralEffectSetup(DensityWaveSpiralEffect *e,
                                  const DensityWaveSpiralConfig *cfg,
                                  float deltaTime);

// Unloads shader
void DensityWaveSpiralEffectUninit(DensityWaveSpiralEffect *e);

// Returns default config
DensityWaveSpiralConfig DensityWaveSpiralConfigDefault(void);

#endif // DENSITY_WAVE_SPIRAL_H
