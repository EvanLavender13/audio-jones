// Snake Skin generator
// Tiled scale grid with spoke ridges, horizontal wave sway, and FFT twinkle

#ifndef SNAKE_SKIN_H
#define SNAKE_SKIN_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SnakeSkinConfig {
  bool enabled = false;

  // Geometry
  float scaleSize = 10.0f;     // Grid density (3.0-30.0)
  float spikeAmount = 0.6f;    // Spike intensity (-1.0-2.0)
  float spikeFrequency = 1.0f; // Spoke ridges per scale (0.0-16.0)
  float sag = -0.8f;           // Scale taper (-2.0-0.0)

  // Motion (speeds accumulate on CPU into phases)
  float scrollSpeed = 0.0f;      // Vertical scroll rate (-5.0-5.0)
  float waveSpeed = 1.0f;        // Horizontal wave rate (0.1-5.0)
  float waveAmplitude = 0.1f;    // Wave displacement (0.0-0.5)
  float twinkleSpeed = 1.0f;     // Twinkle pulse rate (0.1-5.0)
  float twinkleIntensity = 0.5f; // Twinkle flash brightness (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Baseline brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend strength (0.0-5.0)
};

#define SNAKE_SKIN_CONFIG_FIELDS                                               \
  enabled, scaleSize, spikeAmount, spikeFrequency, sag, scrollSpeed,           \
      waveSpeed, waveAmplitude, twinkleSpeed, twinkleIntensity, baseFreq,      \
      maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SnakeSkinEffect {
  Shader shader;
  int resolutionLoc;
  int scaleSizeLoc;
  int spikeAmountLoc;
  int spikeFrequencyLoc;
  int sagLoc;
  int scrollPhaseLoc;
  int wavePhaseLoc;
  int waveAmplitudeLoc;
  int twinklePhaseLoc;
  int twinkleIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int fftTextureLoc;

  float scrollPhase;
  float wavePhase;
  float twinklePhase;

  ColorLUT *gradientLUT;
} SnakeSkinEffect;

// Returns true on success, false if shader fails to load
bool SnakeSkinEffectInit(SnakeSkinEffect *e, const SnakeSkinConfig *cfg);

// Binds all uniforms, updates LUT texture, and advances phase accumulators
void SnakeSkinEffectSetup(SnakeSkinEffect *e, const SnakeSkinConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void SnakeSkinEffectUninit(SnakeSkinEffect *e);

// Registers modulatable params with the modulation engine
void SnakeSkinRegisterParams(SnakeSkinConfig *cfg);

#endif // SNAKE_SKIN_H
