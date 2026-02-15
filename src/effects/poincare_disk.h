#ifndef POINCARE_DISK_H
#define POINCARE_DISK_H

#include "raylib.h"
#include <stdbool.h>

// Poincare Disk: Hyperbolic tiling with Mobius translation and fundamental
// domain folding
struct PoincareDiskConfig {
  bool enabled = false;
  int tileP = 4;             // Angle at origin vertex (pi/P), range 2-12
  int tileQ = 4;             // Angle at second vertex (pi/Q), range 2-12
  int tileR = 4;             // Angle at third vertex (pi/R), range 2-12
  float translationX = 0.0f; // Mobius translation center X (-0.9 to 0.9)
  float translationY = 0.0f; // Mobius translation center Y (-0.9 to 0.9)
  float translationSpeed =
      0.0f; // Circular motion angular velocity (radians/second)
  float translationAmplitude = 0.0f; // Circular motion radius (0.0-0.9)
  float diskScale = 1.0f;            // Disk size relative to screen (0.5-2.0)
  float rotationSpeed = 0.0f; // Euclidean rotation speed (radians/second)
};

#define POINCARE_DISK_CONFIG_FIELDS                                            \
  enabled, tileP, tileQ, tileR, translationX, translationY, translationSpeed,  \
      translationAmplitude, diskScale, rotationSpeed

typedef struct PoincareDiskEffect {
  Shader shader;
  int tilePLoc;
  int tileQLoc;
  int tileRLoc;
  int translationLoc;
  int rotationLoc;
  int diskScaleLoc;
  float time;                  // For circular translation motion
  float rotation;              // Euclidean rotation accumulator
  float currentTranslation[2]; // Computed translation vec2
} PoincareDiskEffect;

// Returns true on success, false if shader fails to load
bool PoincareDiskEffectInit(PoincareDiskEffect *e);

// Accumulates time and rotation, computes circular translation, sets all
// uniforms
void PoincareDiskEffectSetup(PoincareDiskEffect *e,
                             const PoincareDiskConfig *cfg, float deltaTime);

// Unloads shader
void PoincareDiskEffectUninit(PoincareDiskEffect *e);

// Returns default config
PoincareDiskConfig PoincareDiskConfigDefault(void);

// Registers modulatable params with the modulation engine
void PoincareDiskRegisterParams(PoincareDiskConfig *cfg);

#endif // POINCARE_DISK_H
