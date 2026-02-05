// Bokeh depth-of-field effect module
// Golden-angle Vogel disc sampling with brightness-weighted blur

#ifndef BOKEH_H
#define BOKEH_H

#include "raylib.h"
#include <stdbool.h>

// Bokeh: Simulates out-of-focus camera blur with golden-angle Vogel disc
// sampling Bright spots bloom into soft circular highlights weighted by
// brightness
struct BokehConfig {
  bool enabled = false;
  float radius = 0.02f; // Blur disc size in UV space (0.0-0.1)
  int iterations =
      64; // Sample count (16-150). Higher = better quality, slower.
  float brightnessPower =
      4.0f; // Brightness weighting exponent (1.0-8.0). Higher = more "pop".
};

typedef struct BokehEffect {
  Shader shader;
  int resolutionLoc;
  int radiusLoc;
  int iterationsLoc;
  int brightnessPowerLoc;
} BokehEffect;

// Returns true on success, false if shader fails to load
bool BokehEffectInit(BokehEffect *e);

// Sets all uniforms
void BokehEffectSetup(BokehEffect *e, const BokehConfig *cfg);

// Unloads shader
void BokehEffectUninit(BokehEffect *e);

// Returns default config
BokehConfig BokehConfigDefault(void);

#endif // BOKEH_H
