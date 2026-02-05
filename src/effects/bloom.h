// Bloom effect module
// Dual Kawase blur with soft threshold extraction for HDR-style glow

#ifndef BLOOM_H
#define BLOOM_H

#include "raylib.h"
#include <stdbool.h>

static const int BLOOM_MIP_COUNT = 5;

struct BloomConfig {
  bool enabled = false;
  float threshold = 0.8f; // Brightness cutoff for extraction (0.0-2.0)
  float knee = 0.5f;      // Soft threshold falloff (0.0-1.0)
  float intensity = 0.5f; // Final glow strength (0.0-2.0)
  int iterations = 5;     // Mip chain depth (3-5)
};

typedef struct BloomEffect {
  Shader prefilterShader;
  Shader downsampleShader;
  Shader upsampleShader;
  Shader compositeShader;
  RenderTexture2D mips[BLOOM_MIP_COUNT];

  // Prefilter shader uniform locations
  int thresholdLoc;
  int kneeLoc;

  // Downsample shader uniform locations
  int downsampleHalfpixelLoc;

  // Upsample shader uniform locations
  int upsampleHalfpixelLoc;

  // Composite shader uniform locations
  int intensityLoc;
  int bloomTexLoc;
} BloomEffect;

// Loads 4 shaders, caches uniform locations, allocates mip chain
bool BloomEffectInit(BloomEffect *e, int width, int height);

// Binds composite uniforms (intensity + bloom texture)
void BloomEffectSetup(BloomEffect *e, const BloomConfig *cfg);

// Unloads mips, reallocates at new dimensions
void BloomEffectResize(BloomEffect *e, int width, int height);

// Unloads 4 shaders and mip chain
void BloomEffectUninit(BloomEffect *e);

// Returns default config
BloomConfig BloomConfigDefault(void);

#endif // BLOOM_H
