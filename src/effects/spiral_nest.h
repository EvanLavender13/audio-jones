// SpiralNest effect module
// Nested spiraling fractal arms with FFT-driven glow and configurable zoom

#ifndef SPIRAL_NEST_H
#define SPIRAL_NEST_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct SpiralNestConfig {
  bool enabled = false;

  // Geometry
  float zoom = 100.0f; // Base spiral arm count (10.0-400.0)

  // Animation
  float animSpeed = 0.05f; // Time evolution rate (0.01-1.0)

  // Glow
  float glowIntensity = 2.0f; // Overall brightness multiplier (0.5-10.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SPIRAL_NEST_CONFIG_FIELDS                                              \
  enabled, zoom, animSpeed, glowIntensity, baseFreq, maxFreq, gain, curve,     \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpiralNestEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float timeAccum; // CPU-accumulated animation time
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int zoomLoc;
  int timeAccumLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SpiralNestEffect;

// Returns true on success, false if shader fails to load
bool SpiralNestEffectInit(SpiralNestEffect *e, const SpiralNestConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SpiralNestEffectSetup(SpiralNestEffect *e, const SpiralNestConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void SpiralNestEffectUninit(SpiralNestEffect *e);

// Registers modulatable params with the modulation engine
void SpiralNestRegisterParams(SpiralNestConfig *cfg);

SpiralNestEffect *GetSpiralNestEffect(PostEffect *pe);

#endif // SPIRAL_NEST_H
