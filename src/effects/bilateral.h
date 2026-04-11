// Bilateral filter effect module
// Edge-preserving blur using spatial and range Gaussian weighting

#ifndef BILATERAL_H
#define BILATERAL_H

#include "raylib.h"
#include <stdbool.h>

struct BilateralConfig {
  bool enabled = false;
  float spatialSigma = 4.0f; // Blur radius in pixels (1.0-10.0)
  float rangeSigma = 0.1f;   // Edge sensitivity (0.01-0.5)
};

#define BILATERAL_CONFIG_FIELDS enabled, spatialSigma, rangeSigma

typedef struct BilateralEffect {
  Shader shader;
  int resolutionLoc;
  int spatialSigmaLoc;
  int rangeSigmaLoc;
} BilateralEffect;

// Returns true on success, false if shader fails to load
bool BilateralEffectInit(BilateralEffect *e);

// Sets all uniforms
void BilateralEffectSetup(const BilateralEffect *e, const BilateralConfig *cfg);

// Unloads shader
void BilateralEffectUninit(const BilateralEffect *e);

// Registers modulatable params with the modulation engine
void BilateralRegisterParams(BilateralConfig *cfg);

#endif // BILATERAL_H
