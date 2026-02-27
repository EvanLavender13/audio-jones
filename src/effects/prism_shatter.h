// Prism Shatter effect module
// Raymarched cross-product crystal geometry with gradient coloring

#ifndef PRISM_SHATTER_H
#define PRISM_SHATTER_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PrismShatterConfig {
  bool enabled = false;

  // Geometry
  float displacementScale =
      4.0f;                 // Cross-product displacement magnitude (1.0-10.0)
  float stepSize = 2.0f;    // Ray march step distance (0.5-5.0)
  int iterations = 128;     // Ray march steps (64-256)
  int displacementMode = 0; // Displacement operation (0-5)

  // Camera
  float cameraSpeed = 0.125f; // Camera orbit speed (-1.0 to 1.0)
  float orbitRadius = 16.0f;  // Camera distance from origin (4.0-32.0)
  float fov = 1.5f;           // Focal length / field of view (0.5-3.0)

  // Structure
  float structureSpeed = 0.0f; // Crystal phase animation speed (-1.0 to 1.0)

  // Rendering
  float brightness = 1.0f;      // Overall brightness multiplier (0.5-3.0)
  float saturationPower = 2.0f; // Edge-to-void contrast exponent (1.0-4.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define PRISM_SHATTER_CONFIG_FIELDS                                            \
  enabled, displacementScale, stepSize, iterations, displacementMode,          \
      cameraSpeed, orbitRadius, fov, structureSpeed, brightness,               \
      saturationPower, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct PrismShatterEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float cameraTime;    // CPU-accumulated camera orbit phase
  float structureTime; // CPU-accumulated structure animation phase
  int resolutionLoc;
  int cameraTimeLoc, structureTimeLoc;
  int displacementScaleLoc, stepSizeLoc, iterationsLoc, displacementModeLoc;
  int orbitRadiusLoc, fovLoc;
  int brightnessLoc, saturationPowerLoc;
  int gradientLUTLoc;
} PrismShatterEffect;

// Returns true on success, false if shader fails to load
bool PrismShatterEffectInit(PrismShatterEffect *e,
                            const PrismShatterConfig *cfg);

// Binds all uniforms, updates LUT texture
void PrismShatterEffectSetup(PrismShatterEffect *e,
                             const PrismShatterConfig *cfg, float deltaTime);

// Unloads shader and frees LUT
void PrismShatterEffectUninit(PrismShatterEffect *e);

// Returns default config
PrismShatterConfig PrismShatterConfigDefault(void);

// Registers modulatable params with the modulation engine
void PrismShatterRegisterParams(PrismShatterConfig *cfg);

#endif // PRISM_SHATTER_H
