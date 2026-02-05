// Watercolor effect module
// Gradient-flow stroke tracing with paper granulation

#ifndef WATERCOLOR_H
#define WATERCOLOR_H

#include "raylib.h"
#include <stdbool.h>

struct WatercolorConfig {
  bool enabled = false;
  int samples = 24;        // Trace iterations per pixel (8-32)
  float strokeStep = 1.0f; // Outline trace step length (0.4-2.0)
  float washStrength =
      0.7f;                // Wash color blend (0.0=outline only, 1.0=full wash)
  float paperScale = 8.0f; // Paper texture frequency (1.0-20.0)
  float paperStrength = 0.4f; // Paper texture intensity (0.0-1.0)
  float edgePool = 0.3f;      // Pigment pooling at edges (0.0-1.0)
  float flowCenter = 0.9f;    // Wet-edge threshold center (0.5-1.2)
  float flowWidth = 0.2f;     // Wet-edge softness spread (0.05-0.5)
};

typedef struct WatercolorEffect {
  Shader shader;
  int samplesLoc;
  int strokeStepLoc;
  int washStrengthLoc;
  int paperScaleLoc;
  int paperStrengthLoc;
  int edgePoolLoc;
  int flowCenterLoc;
  int flowWidthLoc;
} WatercolorEffect;

// Returns true on success, false if shader fails to load
bool WatercolorEffectInit(WatercolorEffect *e);

// Sets all uniforms except resolution (ApplyHalfResEffect handles resolution)
void WatercolorEffectSetup(WatercolorEffect *e, const WatercolorConfig *cfg);

// Unloads shader
void WatercolorEffectUninit(WatercolorEffect *e);

// Returns default config
WatercolorConfig WatercolorConfigDefault(void);

#endif // WATERCOLOR_H
