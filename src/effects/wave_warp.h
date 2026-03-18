#ifndef WAVE_WARP_H
#define WAVE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Wave Warp effect module
// Displaces coordinates using fBM built from selectable basis functions
// (triangle, sine, sawtooth, square) to create crease-like warp patterns.
struct WaveWarpConfig {
  bool enabled = false;
  int waveType = 0;      // Basis function: 0=tri, 1=sine, 2=saw, 3=square
  float strength = 0.4f; // Displacement magnitude (0.0-1.0)
  float speed = 4.5f;    // Crease rotation rate rad/s (0.0-10.0)
  int octaves = 4;       // fBM iteration count (1-6)
  float scale = 0.8f;    // Initial noise frequency (0.1-4.0)
};

#define WAVE_WARP_CONFIG_FIELDS                                                \
  enabled, waveType, strength, speed, octaves, scale

typedef struct WaveWarpEffect {
  Shader shader;
  int waveTypeLoc;
  int strengthLoc;
  int timeLoc;
  int octavesLoc;
  int scaleLoc;
  float time; // Accumulated animation time
} WaveWarpEffect;

// Returns true on success, false if shader fails to load
bool WaveWarpEffectInit(WaveWarpEffect *e);

// Accumulates time, sets all uniforms
void WaveWarpEffectSetup(WaveWarpEffect *e, const WaveWarpConfig *cfg,
                         float deltaTime);

// Unloads shader
void WaveWarpEffectUninit(const WaveWarpEffect *e);

// Registers modulatable params with the modulation engine
void WaveWarpRegisterParams(WaveWarpConfig *cfg);

#endif // WAVE_WARP_H
