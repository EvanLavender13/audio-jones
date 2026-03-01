#ifndef TRIANGLE_WAVE_WARP_H
#define TRIANGLE_WAVE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Triangle Wave Warp — folded triangle-wave displacement with rotating creases
struct TriangleWaveWarpConfig {
  bool enabled = false;
  float noiseStrength = 0.4f; // Displacement magnitude (0.0-1.0)
  float noiseSpeed = 4.5f;    // Crease rotation rate rad/s (0.0-10.0)
};

#define TRIANGLE_WAVE_WARP_CONFIG_FIELDS enabled, noiseStrength, noiseSpeed

typedef struct TriangleWaveWarpEffect {
  Shader shader;
  int noiseStrengthLoc;
  int timeLoc;
  float time; // Accumulated animation time
} TriangleWaveWarpEffect;

// Returns true on success, false if shader fails to load
bool TriangleWaveWarpEffectInit(TriangleWaveWarpEffect *e);

// Accumulates time, sets all uniforms
void TriangleWaveWarpEffectSetup(TriangleWaveWarpEffect *e,
                                 const TriangleWaveWarpConfig *cfg,
                                 float deltaTime);

// Unloads shader
void TriangleWaveWarpEffectUninit(TriangleWaveWarpEffect *e);

// Returns default config
TriangleWaveWarpConfig TriangleWaveWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void TriangleWaveWarpRegisterParams(TriangleWaveWarpConfig *cfg);

#endif // TRIANGLE_WAVE_WARP_H
