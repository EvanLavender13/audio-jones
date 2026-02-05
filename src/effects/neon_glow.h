// Neon glow effect module
// Sobel edge detection with colored glow and additive blending

#ifndef NEON_GLOW_H
#define NEON_GLOW_H

#include "raylib.h"
#include <stdbool.h>

struct NeonGlowConfig {
  bool enabled = false;
  float glowR = 0.0f;              // Glow color red (0.0-1.0)
  float glowG = 1.0f;              // Glow color green (0.0-1.0)
  float glowB = 1.0f;              // Glow color blue (0.0-1.0)
  float edgeThreshold = 0.1f;      // Noise suppression (0.0-0.5)
  float edgePower = 1.0f;          // Edge intensity curve (0.5-3.0)
  float glowIntensity = 2.0f;      // Brightness multiplier (0.5-5.0)
  float glowRadius = 2.0f;         // Blur spread in pixels (0.0-10.0)
  int glowSamples = 5;             // Cross-tap quality, odd (3-9)
  float originalVisibility = 0.0f; // Original image blend (0.0-1.0)
  int colorMode = 0;               // 0 = Custom color, 1 = Source-derived
  float saturationBoost = 0.5f;    // Extra saturation for source mode (0.0-1.0)
  float brightnessBoost = 0.5f;    // Extra brightness for source mode (0.0-1.0)
};

typedef struct NeonGlowEffect {
  Shader shader;
  int resolutionLoc;
  int glowColorLoc;
  int edgeThresholdLoc;
  int edgePowerLoc;
  int glowIntensityLoc;
  int glowRadiusLoc;
  int glowSamplesLoc;
  int originalVisibilityLoc;
  int colorModeLoc;
  int saturationBoostLoc;
  int brightnessBoostLoc;
} NeonGlowEffect;

bool NeonGlowEffectInit(NeonGlowEffect *e);
void NeonGlowEffectSetup(NeonGlowEffect *e, const NeonGlowConfig *cfg);
void NeonGlowEffectUninit(NeonGlowEffect *e);
NeonGlowConfig NeonGlowConfigDefault(void);

#endif // NEON_GLOW_H
