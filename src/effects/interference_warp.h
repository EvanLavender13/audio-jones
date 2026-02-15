#ifndef INTERFERENCE_WARP_H
#define INTERFERENCE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Interference Warp - multi-axis harmonic UV displacement
// Sums sine waves across configurable axes to create lattice-like distortion.
// Higher axes produce more complex quasicrystal patterns.
struct InterferenceWarpConfig {
  bool enabled = false;
  float amplitude = 0.1f;         // Displacement strength (0.0-0.5)
  float scale = 2.0f;             // Pattern frequency/density (0.5-10.0)
  int axes = 3;                   // Lattice symmetry type (2-8)
  float axisRotationSpeed = 0.0f; // Pattern rotation rate (radians/second)
  int harmonics = 64;             // Detail level (8-256)
  float decay = 1.0f;             // Amplitude falloff exponent (0.5-2.0)
  float speed = 0.0003f;          // Time evolution rate (0.0-0.01)
  float drift = 2.0f;             // Harmonic phase drift exponent (1.0-3.0)
};

#define INTERFERENCE_WARP_CONFIG_FIELDS                                        \
  enabled, amplitude, scale, axes, axisRotationSpeed, harmonics, decay, speed, \
      drift

typedef struct InterferenceWarpEffect {
  Shader shader;
  int timeLoc;
  int amplitudeLoc;
  int scaleLoc;
  int axesLoc;
  int axisRotationLoc;
  int harmonicsLoc;
  int decayLoc;
  int driftLoc;
  float time;         // Animation accumulator
  float axisRotation; // Rotation accumulator
} InterferenceWarpEffect;

// Returns true on success, false if shader fails to load
bool InterferenceWarpEffectInit(InterferenceWarpEffect *e);

// Accumulates time and axisRotation, sets all uniforms
void InterferenceWarpEffectSetup(InterferenceWarpEffect *e,
                                 const InterferenceWarpConfig *cfg,
                                 float deltaTime);

// Unloads shader
void InterferenceWarpEffectUninit(InterferenceWarpEffect *e);

// Returns default config
InterferenceWarpConfig InterferenceWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void InterferenceWarpRegisterParams(InterferenceWarpConfig *cfg);

#endif // INTERFERENCE_WARP_H
