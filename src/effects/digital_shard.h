// Digital Shard generator
// Noise-driven angular shards with per-cell FFT reactivity

#ifndef DIGITAL_SHARD_H
#define DIGITAL_SHARD_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct DigitalShardConfig {
  bool enabled = false;

  // Geometry
  int iterations = 100;           // Loop count (20-100)
  float zoom = 0.4f;              // Base coordinate scale (0.1-2.0)
  float aberrationSpread = 0.01f; // Per-iteration position offset (0.001-0.05)
  float noiseScale = 64.0f;       // Noise texture tiling (16.0-256.0)
  float rotationLevels = 8.0f;    // Angle quantization steps (2.0-16.0)
  float softness = 0.0f;          // Hard binary to smooth gradient (0.0-1.0)
  float speed = 1.0f;             // Animation rate (0.1-5.0)

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

#define DIGITAL_SHARD_CONFIG_FIELDS                                            \
  enabled, iterations, zoom, aberrationSpread, noiseScale, rotationLevels,     \
      softness, speed, baseFreq, maxFreq, gain, curve, baseBright, gradient,   \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DigitalShardEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int iterationsLoc;
  int zoomLoc;
  int aberrationSpreadLoc;
  int noiseScaleLoc;
  int rotationLevelsLoc;
  int softnessLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  float time;
  ColorLUT *gradientLUT;
} DigitalShardEffect;

// Returns true on success, false if shader fails to load
bool DigitalShardEffectInit(DigitalShardEffect *e,
                            const DigitalShardConfig *cfg);

// Binds all uniforms, updates LUT texture, and advances time accumulators
void DigitalShardEffectSetup(DigitalShardEffect *e,
                             const DigitalShardConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture);

// Unloads shader and frees LUT
void DigitalShardEffectUninit(DigitalShardEffect *e);

// Registers modulatable params with the modulation engine
void DigitalShardRegisterParams(DigitalShardConfig *cfg);

#endif // DIGITAL_SHARD_H
