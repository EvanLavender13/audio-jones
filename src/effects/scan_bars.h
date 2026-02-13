// Scan bars effect module
// Generates scrolling bar patterns (linear, spokes, rings) with palette-driven
// color chaos

#ifndef SCAN_BARS_H
#define SCAN_BARS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ScanBarsConfig {
  bool enabled = false;

  int mode = 0;                   // 0=Linear, 1=Spokes, 2=Rings
  float angle = 0.0f;             // Bar orientation in radians (linear mode)
  float barDensity = 10.0f;       // Number of bars across viewport
  float convergence = 0.5f;       // tan() bunching strength
  float convergenceFreq = 5.0f;   // Spatial frequency of convergence warping
  float convergenceOffset = 0.0f; // Focal point offset from center
  float sharpness = 0.1f;         // Bar edge hardness (smoothstep width)
  float scrollSpeed = 0.2f;       // Bar position scroll rate
  float colorSpeed = 1.0f;        // LUT index drift rate
  float chaosFreq = 10.0f;        // Frequency multiplier for color chaos math
  float chaosIntensity = 1.0f; // How wildly adjacent bars jump across palette
  float snapAmount = 0.0f;     // Time quantization (0=smooth, higher=stutter)

  // Audio
  float baseFreq = 55.0f;   // Lowest mapped frequency in Hz (A1)
  float numOctaves = 5.0f;  // Octave range mapped across bars (1.0-8.0)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color (palette sampled via LUT)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct ScanBarsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float scrollPhase; // Bar position accumulator
  float colorPhase;  // LUT index drift accumulator
  int resolutionLoc;
  int modeLoc;
  int angleLoc;
  int barDensityLoc;
  int convergenceLoc;
  int convergenceFreqLoc;
  int convergenceOffsetLoc;
  int sharpnessLoc;
  int scrollPhaseLoc;
  int colorPhaseLoc;
  int chaosFreqLoc;
  int chaosIntensityLoc;
  int snapAmountLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ScanBarsEffect;

// Returns true on success, false if shader fails to load
bool ScanBarsEffectInit(ScanBarsEffect *e, const ScanBarsConfig *cfg);

// Binds all uniforms, advances phase accumulators, updates LUT texture
void ScanBarsEffectSetup(ScanBarsEffect *e, const ScanBarsConfig *cfg,
                         float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void ScanBarsEffectUninit(ScanBarsEffect *e);

// Returns default config
ScanBarsConfig ScanBarsConfigDefault(void);

// Registers modulatable params with the modulation engine
void ScanBarsRegisterParams(ScanBarsConfig *cfg);

#endif // SCAN_BARS_H
