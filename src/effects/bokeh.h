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
  int shape = 0;                // 0=Disc, 1=Box, 2=Hex, 3=Star
  float shapeAngle = 0.0f;      // Kernel rotation in radians (0-2pi)
  int starPoints = 5;           // Star point count (3-8)
  float starInnerRadius = 0.4f; // Star valley depth (0.1-0.9)
};

#define BOKEH_CONFIG_FIELDS                                                    \
  enabled, radius, iterations, brightnessPower, shape, shapeAngle, starPoints, \
      starInnerRadius

typedef struct BokehEffect {
  Shader shader;
  int resolutionLoc;
  int radiusLoc;
  int iterationsLoc;
  int brightnessPowerLoc;
  int shapeLoc;
  int shapeAngleLoc;
  int starPointsLoc;
  int starInnerRadiusLoc;
} BokehEffect;

// Returns true on success, false if shader fails to load
bool BokehEffectInit(BokehEffect *e);

// Sets all uniforms
void BokehEffectSetup(BokehEffect *e, const BokehConfig *cfg);

// Unloads shader
void BokehEffectUninit(BokehEffect *e);

// Returns default config
BokehConfig BokehConfigDefault(void);

// Registers modulatable params with the modulation engine
void BokehRegisterParams(BokehConfig *cfg);

#endif // BOKEH_H
