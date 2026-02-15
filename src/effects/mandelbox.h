#ifndef MANDELBOX_H
#define MANDELBOX_H

#include "raylib.h"
#include <stdbool.h>

// Config struct (user-facing parameters, serialized in presets)
struct MandelboxConfig {
  bool enabled = false;
  int iterations = 2;           // Fold/scale/translate cycles (1-6)
  float boxLimit = 1.0f;        // Box fold boundary (0.5-2.0)
  float sphereMin = 0.5f;       // Inner sphere radius (0.1-0.5)
  float sphereMax = 1.0f;       // Outer sphere radius (0.5-2.0)
  float scale = -2.0f;          // Scale factor per iteration (-3.0 to 3.0)
  float offsetX = 1.0f;         // X translation after fold (0.0-2.0)
  float offsetY = 1.0f;         // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f;   // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;      // Per-iteration rotation rate (radians/second)
  float boxIntensity = 1.0f;    // Box fold contribution (0.0-1.0)
  float sphereIntensity = 1.0f; // Sphere fold contribution (0.0-1.0)
  bool polarFold = false;       // Enable polar coordinate pre-fold
  int polarFoldSegments = 6;    // Wedge count for polar fold (2-12)
};

#define MANDELBOX_CONFIG_FIELDS                                                \
  enabled, iterations, boxLimit, sphereMin, sphereMax, scale, offsetX,         \
      offsetY, rotationSpeed, twistSpeed, boxIntensity, sphereIntensity,       \
      polarFold, polarFoldSegments

// Runtime state (shader + uniforms + animation accumulators)
typedef struct MandelboxEffect {
  Shader shader;
  int iterationsLoc;
  int boxLimitLoc;
  int sphereMinLoc;
  int sphereMaxLoc;
  int scaleLoc;
  int offsetLoc; // vec2: mandelboxOffset
  int rotationLoc;
  int twistAngleLoc;
  int boxIntensityLoc;
  int sphereIntensityLoc;
  int polarFoldLoc;
  int polarFoldSegmentsLoc;
  float rotation; // Animation accumulator
  float twist;    // Per-iteration rotation accumulator
} MandelboxEffect;

// Lifecycle functions
bool MandelboxEffectInit(MandelboxEffect *e);
void MandelboxEffectSetup(MandelboxEffect *e, const MandelboxConfig *cfg,
                          float deltaTime);
void MandelboxEffectUninit(MandelboxEffect *e);
MandelboxConfig MandelboxConfigDefault(void);

// Registers modulatable params with the modulation engine
void MandelboxRegisterParams(MandelboxConfig *cfg);

#endif
