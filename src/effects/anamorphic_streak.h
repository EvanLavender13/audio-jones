// Anamorphic streak effect module
// Mip-chain pipeline: prefilter extraction, progressive downsample, upsample
// blend, composite

#ifndef ANAMORPHIC_STREAK_H
#define ANAMORPHIC_STREAK_H

#include "raylib.h"
#include <stdbool.h>

static const int STREAK_MIP_COUNT = 7;

struct AnamorphicStreakConfig {
  bool enabled = false;
  float threshold = 0.8f; // Brightness cutoff (0.0-2.0)
  float knee = 0.5f;      // Soft threshold falloff (0.0-1.0)
  float intensity = 0.5f; // Streak brightness in composite (0.0-2.0)
  float stretch = 0.8f;   // Upsample blend: favors wider blur levels (0.0-1.0)
  float tintR = 0.55f;    // Streak color red channel (0.0-1.0)
  float tintG = 0.65f;    // Streak color green channel (0.0-1.0)
  float tintB = 1.0f;     // Streak color blue channel (0.0-1.0)
  int iterations = 5;     // Mip chain depth (3-7)
};

typedef struct AnamorphicStreakEffect {
  Shader prefilterShader;
  Shader downsampleShader;
  Shader upsampleShader;
  Shader compositeShader;
  RenderTexture2D
      mips[STREAK_MIP_COUNT]; // Downsample chain (read-only during upsample)
  RenderTexture2D mipsUp[STREAK_MIP_COUNT]; // Upsample chain (write targets)

  // Prefilter shader uniform locations
  int thresholdLoc;
  int kneeLoc;

  // Downsample shader uniform locations
  int downsampleTexelLoc;

  // Upsample shader uniform locations
  int upsampleTexelLoc;
  int highResTexLoc;
  int stretchLoc;

  // Composite shader uniform locations
  int intensityLoc;
  int tintLoc;
  int streakTexLoc;
} AnamorphicStreakEffect;

// Loads 4 shaders, caches uniform locations, allocates mip chain
bool AnamorphicStreakEffectInit(AnamorphicStreakEffect *e, int width,
                                int height);

// Binds composite uniforms (intensity, tint, streak texture)
void AnamorphicStreakEffectSetup(AnamorphicStreakEffect *e,
                                 const AnamorphicStreakConfig *cfg);

// Unloads mips, reallocates at new dimensions
void AnamorphicStreakEffectResize(AnamorphicStreakEffect *e, int width,
                                  int height);

// Unloads 4 shaders and mip chain
void AnamorphicStreakEffectUninit(AnamorphicStreakEffect *e);

// Returns default config
AnamorphicStreakConfig AnamorphicStreakConfigDefault(void);

// Registers modulatable params with the modulation engine
void AnamorphicStreakRegisterParams(AnamorphicStreakConfig *cfg);

#endif // ANAMORPHIC_STREAK_H
