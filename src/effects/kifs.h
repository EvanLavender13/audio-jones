#ifndef KIFS_H
#define KIFS_H

#include "raylib.h"
#include <stdbool.h>

// Config struct (user-facing parameters, serialized in presets)
struct KifsConfig {
  bool enabled = false;
  int iterations = 4;         // Fold/scale/translate cycles (1-6)
  float scale = 2.0f;         // Expansion factor per iteration (1.5-2.5)
  float offsetX = 1.0f;       // X translation after fold (0.0-2.0)
  float offsetY = 1.0f;       // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  bool octantFold = false;    // Enable 8-way octant symmetry
  bool polarFold = false;     // Enable polar coordinate pre-fold
  int polarFoldSegments = 6;  // Wedge count for polar fold (2-12)
  float polarFoldSmoothing = 0.0f; // Blend width at polar fold seams (0.0-0.5)
};

// Runtime state (shader + uniforms + animation accumulators)
typedef struct KifsEffect {
  Shader shader;
  int rotationLoc;
  int twistAngleLoc;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc; // vec2: kifsOffset
  int octantFoldLoc;
  int polarFoldLoc;
  int polarFoldSegmentsLoc;
  int polarFoldSmoothingLoc;
  float rotation; // Animation accumulator
  float twist;    // Per-iteration rotation accumulator
} KifsEffect;

// Lifecycle functions
bool KifsEffectInit(KifsEffect *e);
void KifsEffectSetup(KifsEffect *e, const KifsConfig *cfg, float deltaTime);
void KifsEffectUninit(KifsEffect *e);
KifsConfig KifsConfigDefault(void);

#endif
