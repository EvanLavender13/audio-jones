// Vignette effect module
// Edge darkening with configurable shape, softness, and tint color

#ifndef VIGNETTE_EFFECT_H
#define VIGNETTE_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

// Vignette: Darkens or tints screen edges with adjustable radius, softness,
// and shape roundness
struct VignetteConfig {
  bool enabled = false;
  float intensity =
      0.5f; // How strongly edges blend toward vignette color (0.0-1.0)
  float radius = 0.5f; // Distance from center where darkening begins (0.0-1.0)
  float softness = 0.4f;  // Width of falloff gradient (0.01-1.0)
  float roundness = 1.0f; // Shape: 0=rectangular, 1=circular (0.0-1.0)
  float colorR = 0.0f;    // Vignette color red (0.0-1.0)
  float colorG = 0.0f;    // Vignette color green (0.0-1.0)
  float colorB = 0.0f;    // Vignette color blue (0.0-1.0)
};

#define VIGNETTE_CONFIG_FIELDS                                                 \
  enabled, intensity, radius, softness, roundness, colorR, colorG, colorB

typedef struct VignetteEffect {
  Shader shader;
  int intensityLoc;
  int radiusLoc;
  int softnessLoc;
  int roundnessLoc;
  int colorLoc;
} VignetteEffect;

// Returns true on success, false if shader fails to load
bool VignetteEffectInit(VignetteEffect *e);

// Sets all uniforms
void VignetteEffectSetup(const VignetteEffect *e, const VignetteConfig *cfg);

// Unloads shader
void VignetteEffectUninit(const VignetteEffect *e);

// Registers modulatable params with the modulation engine
void VignetteRegisterParams(VignetteConfig *cfg);

#endif // VIGNETTE_EFFECT_H
