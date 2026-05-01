#ifndef KALEIDOSCOPE_H
#define KALEIDOSCOPE_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

// Kaleidoscope (Polar): Wedge-based radial mirroring with optional smooth edges
struct KaleidoscopeConfig {
  bool enabled = false;
  int segments = 6;           // Mirror segments / wedge count (1-12)
  float rotationSpeed = 0.0f; // Rotation rate (radians/second)
  float twistAngle = 0.0f;    // Radial twist offset (radians)
  float smoothing = 0.0f; // Blend width at wedge seams (0.0-0.5, 0 = hard edge)
};

#define KALEIDOSCOPE_CONFIG_FIELDS                                             \
  enabled, segments, rotationSpeed, twistAngle, smoothing

typedef struct KaleidoscopeEffect {
  Shader shader;
  int segmentsLoc;
  int rotationLoc;
  int twistAngleLoc;
  int smoothingLoc;
  float rotation; // Animation accumulator
} KaleidoscopeEffect;

// Returns true on success, false if shader fails to load
bool KaleidoscopeEffectInit(KaleidoscopeEffect *e);

// Accumulates rotation, sets all uniforms
void KaleidoscopeEffectSetup(KaleidoscopeEffect *e,
                             const KaleidoscopeConfig *cfg, float deltaTime);

// Unloads shader
void KaleidoscopeEffectUninit(const KaleidoscopeEffect *e);

// Registers modulatable params with the modulation engine
void KaleidoscopeRegisterParams(KaleidoscopeConfig *cfg);

KaleidoscopeEffect *GetKaleidoscopeEffect(PostEffect *pe);

#endif // KALEIDOSCOPE_H
