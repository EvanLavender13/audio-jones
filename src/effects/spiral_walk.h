// SpiralWalk effect module
// Spiraling chain of glowing segments with per-segment FFT brightness

#ifndef SPIRAL_WALK_H
#define SPIRAL_WALK_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SpiralWalkConfig {
  bool enabled = false;

  // Geometry
  int segments = 100;       // Number of line segments in the chain (10-200)
  float growthRate = 0.02f; // Length increment per segment (0.005-0.1)
  float angleOffset =
      1.5708f; // Base angle step between segments in radians (-PI..PI)

  // Animation
  float rotationSpeed =
      0.0f; // Rotation rate for angle morphing rad/s (-PI..PI)

  // Glow
  float glowIntensity = 2.0f; // Brightness multiplier (0.5-10.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum ember brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SPIRAL_WALK_CONFIG_FIELDS                                              \
  enabled, segments, growthRate, angleOffset, rotationSpeed, glowIntensity,    \
      baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode,         \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpiralWalkEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int segmentsLoc;
  int growthRateLoc;
  int angleOffsetLoc;
  int rotationAccumLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SpiralWalkEffect;

// Returns true on success, false if shader fails to load
bool SpiralWalkEffectInit(SpiralWalkEffect *e, const SpiralWalkConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SpiralWalkEffectSetup(SpiralWalkEffect *e, const SpiralWalkConfig *cfg,
                           float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void SpiralWalkEffectUninit(SpiralWalkEffect *e);

// Returns default config
SpiralWalkConfig SpiralWalkConfigDefault(void);

// Registers modulatable params with the modulation engine
void SpiralWalkRegisterParams(SpiralWalkConfig *cfg);

#endif // SPIRAL_WALK_H
