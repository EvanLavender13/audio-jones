// Spark Flash — twinkling four-arm crosshair sparks with sine lifecycle and
// FFT-reactive brightness

#ifndef SPARK_FLASH_H
#define SPARK_FLASH_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SparkFlashConfig {
  bool enabled = false;

  // Geometry
  int layers = 7;        // Concurrent sparks (2-16)
  float lifetime = 0.2f; // Spark lifecycle duration in seconds (0.05-2.0)
  float armThickness = 0.00002f;  // Cross arm glow epsilon (0.00001-0.001)
  float starSize = 0.00005f;      // Center star epsilon (0.00001-0.001)
  float armBrightness = 0.00035f; // Arm glow intensity scalar (0.0001-0.002)
  float starBrightness =
      0.00018f;          // Star point intensity scalar (0.00005-0.001)
  float armReach = 1.0f; // Arm extension distance multiplier (0.1-2.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest mapped pitch in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest mapped pitch in Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.0f;       // Contrast exponent on FFT magnitudes (0.1-3.0)
  float baseBright = 0.1f;  // Glow when semitone is silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SPARK_FLASH_CONFIG_FIELDS                                              \
  enabled, layers, lifetime, armThickness, starSize, armBrightness,            \
      starBrightness, armReach, baseFreq, maxFreq, gain, curve, baseBright,    \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SparkFlashEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // Master time accumulator
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeLoc;
  int layersLoc;
  int lifetimeLoc;
  int armThicknessLoc;
  int starSizeLoc;
  int armBrightnessLoc;
  int starBrightnessLoc;
  int armReachLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SparkFlashEffect;

// Returns true on success, false if shader fails to load
bool SparkFlashEffectInit(SparkFlashEffect *e, const SparkFlashConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SparkFlashEffectSetup(SparkFlashEffect *e, const SparkFlashConfig *cfg,
                           float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void SparkFlashEffectUninit(SparkFlashEffect *e);

// Returns default config
SparkFlashConfig SparkFlashConfigDefault(void);

// Registers modulatable params with the modulation engine
void SparkFlashRegisterParams(SparkFlashConfig *cfg);

#endif // SPARK_FLASH_H
