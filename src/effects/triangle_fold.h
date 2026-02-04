#ifndef TRIANGLE_FOLD_H
#define TRIANGLE_FOLD_H

#include "raylib.h"
#include <stdbool.h>

// Config struct (user-facing parameters, serialized in presets)
struct TriangleFoldConfig {
  bool enabled = false;
  int iterations = 3;         // Recursion depth (1-6)
  float scale = 2.0f;         // Expansion per iteration (1.5-2.5)
  float offsetX = 0.5f;       // X translation after fold (0.0-2.0)
  float offsetY = 0.5f;       // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
};

// Runtime state (shader + uniforms + animation accumulators)
typedef struct TriangleFoldEffect {
  Shader shader;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc; // vec2: triangleOffset
  int rotationLoc;
  int twistAngleLoc;
  float rotation; // Animation accumulator
  float twist;    // Per-iteration rotation accumulator
} TriangleFoldEffect;

// Lifecycle functions
bool TriangleFoldEffectInit(TriangleFoldEffect *e);
void TriangleFoldEffectSetup(TriangleFoldEffect *e,
                             const TriangleFoldConfig *cfg, float deltaTime);
void TriangleFoldEffectUninit(TriangleFoldEffect *e);
TriangleFoldConfig TriangleFoldConfigDefault(void);

#endif
