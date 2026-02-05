// Anamorphic streak effect module
// Three-pass pipeline: prefilter extraction, horizontal blur, composite blend

#ifndef ANAMORPHIC_STREAK_H
#define ANAMORPHIC_STREAK_H

#include "raylib.h"
#include <stdbool.h>

struct AnamorphicStreakConfig {
  bool enabled = false;
  float threshold = 0.8f; // Brightness cutoff (0.0-2.0)
  float knee = 0.5f;      // Soft threshold falloff (0.0-1.0)
  float intensity = 0.5f; // Streak brightness in composite (0.0-2.0)
  float stretch = 1.5f;   // Horizontal extent multiplier (0.5-8.0)
  float sharpness =
      0.3f;           // Kernel falloff: soft (0) to hard lines (1) (0.0-1.0)
  int iterations = 4; // Blur pass count (2-6)
};

typedef struct AnamorphicStreakEffect {
  Shader prefilterShader;
  Shader blurShader;
  Shader compositeShader;

  // Prefilter shader uniform locations
  int thresholdLoc;
  int kneeLoc;

  // Blur shader uniform locations
  int resolutionLoc;
  int offsetLoc;
  int sharpnessLoc;

  // Composite shader uniform locations
  int intensityLoc;
  int streakTexLoc;
} AnamorphicStreakEffect;

// Loads 3 shaders and caches 7 uniform locations
bool AnamorphicStreakEffectInit(AnamorphicStreakEffect *e);

// Binds composite uniforms (intensity + streak texture)
void AnamorphicStreakEffectSetup(AnamorphicStreakEffect *e,
                                 const AnamorphicStreakConfig *cfg,
                                 Texture2D streakTex);

// Unloads 3 shaders
void AnamorphicStreakEffectUninit(AnamorphicStreakEffect *e);

// Returns default config
AnamorphicStreakConfig AnamorphicStreakConfigDefault(void);

// Registers modulatable params with the modulation engine
void AnamorphicStreakRegisterParams(AnamorphicStreakConfig *cfg);

#endif // ANAMORPHIC_STREAK_H
