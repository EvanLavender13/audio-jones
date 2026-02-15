#ifndef FLUX_WARP_H
#define FLUX_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Flux Warp — coupled trig-based UV distortion with morphing cell geometry
// and amplitude gating for a flickering crystalline warp field.
struct FluxWarpConfig {
  bool enabled = false;
  float warpStrength = 0.15f; // 0.0 to 0.5 — displacement amplitude
  float cellScale = 6.0f;  // 1.0 to 20.0 — UV multiplier (more/smaller cells)
  float coupling = 0.7f;   // 0.0 to 1.0 — x->y wave dependency
  float waveFreq = 200.0f; // 10.0 to 500.0 — trig oscillation frequency
  float animSpeed = 1.0f;  // 0.0 to 2.0 — overall time multiplier
  float divisorSpeed = 0.3f; // 0.0 to 1.0 — cell geometry morph rate
  float gateSpeed = 0.15f;   // 0.0 to 0.5 — amplitude modulation rate
};

#define FLUX_WARP_CONFIG_FIELDS                                                \
  enabled, warpStrength, cellScale, coupling, waveFreq, animSpeed,             \
      divisorSpeed, gateSpeed

typedef struct FluxWarpEffect {
  Shader shader;
  int resolutionLoc;
  int warpStrengthLoc;
  int cellScaleLoc;
  int couplingLoc;
  int waveFreqLoc;
  int timeLoc;
  int divisorSpeedLoc;
  int gateSpeedLoc;
  float time; // Accumulated animation time
} FluxWarpEffect;

// Returns true on success, false if shader fails to load
bool FluxWarpEffectInit(FluxWarpEffect *e);

// Accumulates time, sets all uniforms
void FluxWarpEffectSetup(FluxWarpEffect *e, const FluxWarpConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight);

// Unloads shader
void FluxWarpEffectUninit(FluxWarpEffect *e);

// Returns default config
FluxWarpConfig FluxWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void FluxWarpRegisterParams(FluxWarpConfig *cfg);

#endif // FLUX_WARP_H
