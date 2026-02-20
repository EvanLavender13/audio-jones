// Bit crush effect module
// Lattice-based pixelation with FFT-driven glow and iterative folding

#ifndef BIT_CRUSH_H
#define BIT_CRUSH_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct BitCrushConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency (Hz) (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.0f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.05f; // Baseline brightness for silent cells (0.0-1.0)

  // Lattice
  float scale = 0.3f;    // Overall lattice zoom (0.05-1.0)
  float cellSize = 8.0f; // Grid cell size in pixels (2.0-32.0)
  int iterations = 32;   // Fold/lattice iterations (4-64)
  float speed = 1.0f;    // Animation speed multiplier (0.1-5.0)
  int walkMode = 0;      // Walk variant (0-5)

  // Glow
  float glowIntensity = 1.0f; // Cell glow strength (0.0-3.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define BIT_CRUSH_CONFIG_FIELDS                                                \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, scale, cellSize,        \
      iterations, speed, walkMode, glowIntensity, blendIntensity, gradient,    \
      blendMode

typedef struct ColorLUT ColorLUT;

typedef struct BitCrushEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // Accumulated animation time
  int resolutionLoc;
  int centerLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int scaleLoc;
  int cellSizeLoc;
  int iterationsLoc;
  int timeLoc;
  int walkModeLoc;
  int glowIntensityLoc;
  int gradientLUTLoc;
} BitCrushEffect;

// Returns true on success, false if shader fails to load
bool BitCrushEffectInit(BitCrushEffect *e, const BitCrushConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void BitCrushEffectSetup(BitCrushEffect *e, const BitCrushConfig *cfg,
                         float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void BitCrushEffectUninit(BitCrushEffect *e);

// Returns default config
BitCrushConfig BitCrushConfigDefault(void);

// Registers modulatable params with the modulation engine
void BitCrushRegisterParams(BitCrushConfig *cfg);

#endif // BIT_CRUSH_H
