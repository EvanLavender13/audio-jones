#ifndef PLASMA_CONFIG_H
#define PLASMA_CONFIG_H

#include "render/color_config.h"
#include <stdbool.h>

typedef struct PlasmaConfig {
  bool enabled = false;

  // Bolt configuration
  int boltCount = 3;  // Number of vertical bolts (1-8)
  int layerCount = 2; // Depth layers: background bolts at smaller scale (1-3)
  int octaves = 6; // FBM octaves: 1-3 smooth plasma, 6+ jagged lightning (1-10)
  int falloffType = 1; // 0=Sharp (1/d^2), 1=Linear (1/d), 2=Soft (1/sqrt(d))

  // Animation
  float driftSpeed = 0.5f;  // Horizontal wandering rate (0.0-2.0)
  float driftAmount = 0.3f; // Horizontal wandering distance (0.0-1.0)
  float animSpeed = 0.8f;   // Noise animation rate (0.0-5.0)

  // Appearance
  float displacement = 1.0f;   // Path distortion strength (0.0-2.0)
  float glowRadius = 0.07f;    // Halo width multiplier (0.01-0.3)
  float coreBrightness = 1.5f; // Overall intensity (0.5-3.0)
  float flickerAmount =
      0.2f; // Random intensity jitter, 0=smooth, 1=harsh (0.0-1.0)

  // Color (gradient sampled by distance: core -> halo)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

} PlasmaConfig;

#endif // PLASMA_CONFIG_H
