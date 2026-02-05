// Impressionist effect module
// Applies painterly brush-stroke stylization with configurable splat size,
// edges, and grain

#ifndef IMPRESSIONIST_H
#define IMPRESSIONIST_H

#include "raylib.h"
#include <stdbool.h>

struct ImpressionistConfig {
  bool enabled = false;
  int splatCount = 11;
  float splatSizeMin = 0.018f;
  float splatSizeMax = 0.1f;
  float strokeFreq = 1200.0f;
  float strokeOpacity = 0.7f;
  float outlineStrength = 1.0f;
  float edgeStrength = 4.0f;
  float edgeMaxDarken = 0.13f;
  float grainScale = 400.0f;
  float grainAmount = 0.1f;
  float exposure = 1.28f;
};

typedef struct ImpressionistEffect {
  Shader shader;
  int resolutionLoc;
  int splatCountLoc;
  int splatSizeMinLoc;
  int splatSizeMaxLoc;
  int strokeFreqLoc;
  int strokeOpacityLoc;
  int outlineStrengthLoc;
  int edgeStrengthLoc;
  int edgeMaxDarkenLoc;
  int grainScaleLoc;
  int grainAmountLoc;
  int exposureLoc;
} ImpressionistEffect;

// Returns true on success, false if shader fails to load
bool ImpressionistEffectInit(ImpressionistEffect *e);

// Sets all uniforms from config including resolution
void ImpressionistEffectSetup(ImpressionistEffect *e,
                              const ImpressionistConfig *cfg);

// Unloads shader
void ImpressionistEffectUninit(ImpressionistEffect *e);

// Returns default config
ImpressionistConfig ImpressionistConfigDefault(void);

#endif // IMPRESSIONIST_H
