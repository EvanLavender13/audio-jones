// Neon glow effect module
// Sobel edge detection with colored glow and additive blending

#ifndef NEON_GLOW_H
#define NEON_GLOW_H

#include "raylib.h"
#include <stdbool.h>

struct NeonGlowConfig {
  bool enabled = false;
  float glowIntensity = 2.0f;   // Overall glow brightness (0.5-5.0)
  float glowRadius = 3.0f;      // Halo spread distance (1.0-8.0)
  float coreSharpness = 0.5f;   // Sharp core vs soft halo balance (0.0-1.0)
  float edgeThreshold = 0.1f;   // Noise suppression (0.0-0.5)
  float edgePower = 1.0f;       // Edge intensity curve (0.5-3.0)
  float saturationBoost = 0.5f; // Extra saturation for glow color (0.0-1.0)
  float brightnessBoost = 0.5f; // Lifts dark colors for visible glow (0.0-1.0)
  float originalVisibility = 0.0f; // Blend original image underneath (0.0-1.0)
};

#define NEON_GLOW_CONFIG_FIELDS                                                \
  enabled, glowIntensity, glowRadius, coreSharpness, edgeThreshold, edgePower, \
      saturationBoost, brightnessBoost, originalVisibility

typedef struct NeonGlowEffect {
  Shader shader;
  int resolutionLoc;
  int glowIntensityLoc;
  int glowRadiusLoc;
  int coreSharpnessLoc;
  int edgeThresholdLoc;
  int edgePowerLoc;
  int saturationBoostLoc;
  int brightnessBoostLoc;
  int originalVisibilityLoc;
} NeonGlowEffect;

// Returns true on success, false if shader fails to load
bool NeonGlowEffectInit(NeonGlowEffect *e);

// Sets all uniforms
void NeonGlowEffectSetup(NeonGlowEffect *e, const NeonGlowConfig *cfg);

// Unloads shader
void NeonGlowEffectUninit(NeonGlowEffect *e);

// Returns default config
NeonGlowConfig NeonGlowConfigDefault(void);

// Registers modulatable params with the modulation engine
void NeonGlowRegisterParams(NeonGlowConfig *cfg);

#endif // NEON_GLOW_H
