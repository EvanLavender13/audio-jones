#ifndef SINE_WARP_H
#define SINE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Sine Warp with depth accumulation
// Stacks sine-based coordinate shifts with rotation to create organic swirl
// patterns. Supports Cartesian or Polar (radial) coordinate modes.
// depthBlend samples color at each octave for layered effect; disable for
// single final sample.
struct SineWarpConfig {
  bool enabled = false;
  int octaves = 4;             // Number of cascade octaves (1-8)
  float strength = 0.5f;       // Distortion intensity (0.0-2.0)
  float speed = 0.3f;          // Animation rate (radians/second)
  float octaveRotation = 0.5f; // Rotation per octave in radians (±π)
  bool radialMode = false;     // false=Cartesian warp, true=Polar warp
  bool depthBlend = true;      // true=sample each octave, false=sample once
};

typedef struct SineWarpEffect {
  Shader shader;
  int timeLoc;
  int rotationLoc;
  int octavesLoc;
  int strengthLoc;
  int octaveRotationLoc;
  int radialModeLoc;
  int depthBlendLoc;
  float time; // Animation accumulator
} SineWarpEffect;

// Returns true on success, false if shader fails to load
bool SineWarpEffectInit(SineWarpEffect *e);

// Accumulates time, sets all uniforms
void SineWarpEffectSetup(SineWarpEffect *e, const SineWarpConfig *cfg,
                         float deltaTime);

// Unloads shader
void SineWarpEffectUninit(SineWarpEffect *e);

// Returns default config
SineWarpConfig SineWarpConfigDefault(void);

#endif // SINE_WARP_H
