#ifndef DOMAIN_WARP_H
#define DOMAIN_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Domain Warp with iterative noise displacement
// Applies fractal noise-based coordinate warping with configurable iterations.
// Drift accumulates over time to animate the warp pattern in a specified
// direction.
struct DomainWarpConfig {
  bool enabled = false;
  float warpStrength = 0.1f; // 0.0 to 0.5
  float warpScale = 4.0f;    // 1.0 to 10.0
  int warpIterations = 2;    // 1 to 3
  float falloff = 0.5f;      // 0.3 to 0.8
  float driftSpeed = 0.0f;   // Units/second for drift accumulation
  float driftAngle = 0.0f;   // Direction of drift (radians)
};

#define DOMAIN_WARP_CONFIG_FIELDS                                              \
  enabled, warpStrength, warpScale, warpIterations, falloff, driftSpeed,       \
      driftAngle

typedef struct DomainWarpEffect {
  Shader shader;
  int warpStrengthLoc;
  int warpScaleLoc;
  int warpIterationsLoc;
  int falloffLoc;
  int timeOffsetLoc;
  float drift; // Accumulated drift distance
} DomainWarpEffect;

// Returns true on success, false if shader fails to load
bool DomainWarpEffectInit(DomainWarpEffect *e);

// Accumulates drift, computes timeOffset, sets all uniforms
void DomainWarpEffectSetup(DomainWarpEffect *e, const DomainWarpConfig *cfg,
                           float deltaTime);

// Unloads shader
void DomainWarpEffectUninit(DomainWarpEffect *e);

// Returns default config
DomainWarpConfig DomainWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void DomainWarpRegisterParams(DomainWarpConfig *cfg);

#endif // DOMAIN_WARP_H
