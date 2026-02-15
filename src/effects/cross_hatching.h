// Cross hatching effect module
// NPR effect with hand-drawn aesthetic via temporal stutter and varied stroke
// angles. Maps luminance to 4 angle-varied layers with Sobel edge outlines.

#ifndef CROSS_HATCHING_H
#define CROSS_HATCHING_H

#include "raylib.h"
#include <stdbool.h>

struct CrossHatchingConfig {
  bool enabled = false;
  float width = 1.5f;     // Base line thickness in pixels (0.5-4.0)
  float threshold = 1.0f; // Global luminance sensitivity multiplier (0.0-2.0)
  float noise = 0.5f;     // Per-pixel irregularity for organic feel (0.0-1.0)
  float outline = 0.5f;   // Sobel edge outline strength (0.0-1.0)
};

#define CROSS_HATCHING_CONFIG_FIELDS enabled, width, threshold, noise, outline

typedef struct CrossHatchingEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int widthLoc;
  int thresholdLoc;
  int noiseLoc;
  int outlineLoc;
  float time;
} CrossHatchingEffect;

// Returns true on success, false if shader fails to load
bool CrossHatchingEffectInit(CrossHatchingEffect *e);

// Accumulates time, sets all uniforms including resolution
void CrossHatchingEffectSetup(CrossHatchingEffect *e,
                              const CrossHatchingConfig *cfg, float deltaTime);

// Unloads shader
void CrossHatchingEffectUninit(CrossHatchingEffect *e);

// Returns default config
CrossHatchingConfig CrossHatchingConfigDefault(void);

// Registers modulatable params with the modulation engine
void CrossHatchingRegisterParams(CrossHatchingConfig *cfg);

#endif // CROSS_HATCHING_H
