#ifndef WAVE_DRIFT_H
#define WAVE_DRIFT_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

// Wave Drift with depth accumulation
// Stacks waveform-based coordinate shifts with rotation to create organic drift
// patterns. Supports multiple basis functions (tri, sine, saw, square) and
// Cartesian or Polar (radial) coordinate modes. depthBlend samples color at
// each octave for layered effect; disable for single final sample.
struct WaveDriftConfig {
  bool enabled = false;
  int waveType = 1;            // Basis function: 0=tri, 1=sine, 2=saw, 3=square
  int octaves = 4;             // Number of cascade octaves (1-8)
  float strength = 0.5f;       // Distortion intensity (0.0-2.0)
  float speed = 0.3f;          // Animation rate (radians/second)
  float octaveRotation = 0.5f; // Rotation per octave in radians (+-pi)
  bool radialMode = false;     // false=Cartesian warp, true=Polar warp
  bool depthBlend = true;      // true=sample each octave, false=sample once
};

#define WAVE_DRIFT_CONFIG_FIELDS                                               \
  enabled, waveType, octaves, strength, speed, octaveRotation, radialMode,     \
      depthBlend

typedef struct WaveDriftEffect {
  Shader shader;
  int timeLoc;
  int rotationLoc;
  int waveTypeLoc;
  int octavesLoc;
  int strengthLoc;
  int octaveRotationLoc;
  int radialModeLoc;
  int depthBlendLoc;
  float time; // Animation accumulator
} WaveDriftEffect;

// Returns true on success, false if shader fails to load
bool WaveDriftEffectInit(WaveDriftEffect *e);

// Accumulates time, sets all uniforms
void WaveDriftEffectSetup(WaveDriftEffect *e, const WaveDriftConfig *cfg,
                          float deltaTime);

// Unloads shader
void WaveDriftEffectUninit(const WaveDriftEffect *e);

// Registers modulatable params with the modulation engine
void WaveDriftRegisterParams(WaveDriftConfig *cfg);

WaveDriftEffect *GetWaveDriftEffect(PostEffect *pe);

#endif // WAVE_DRIFT_H
