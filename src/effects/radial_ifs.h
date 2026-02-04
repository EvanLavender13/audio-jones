#ifndef RADIAL_IFS_H
#define RADIAL_IFS_H

#include "raylib.h"
#include <stdbool.h>

// Config struct (user-facing parameters, serialized in presets)
struct RadialIfsConfig {
  bool enabled = false;
  int segments = 6;           // Wedge count per fold (3-12)
  int iterations = 4;         // Recursion depth (1-8)
  float scale = 1.8f;         // Expansion per iteration (1.2-2.5)
  float offset = 0.5f;        // Translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  float smoothing = 0.0f;     // Blend width at wedge seams (0.0-0.5)
};

// Runtime state (shader + uniforms + animation accumulators)
typedef struct RadialIfsEffect {
  Shader shader;
  int segmentsLoc;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc;
  int rotationLoc;
  int twistAngleLoc;
  int smoothingLoc;
  float rotation; // Animation accumulator
  float twist;    // Per-iteration rotation accumulator
} RadialIfsEffect;

// Lifecycle functions
bool RadialIfsEffectInit(RadialIfsEffect *e);
void RadialIfsEffectSetup(RadialIfsEffect *e, const RadialIfsConfig *cfg,
                          float deltaTime);
void RadialIfsEffectUninit(RadialIfsEffect *e);
RadialIfsConfig RadialIfsConfigDefault(void);

#endif
