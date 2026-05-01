// Fractal tree effect module
// KIFS-based fractal tree with FFT-driven branching and zoom animation

#ifndef FRACTAL_TREE_H
#define FRACTAL_TREE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct FractalTreeConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Geometry
  float thickness = 1.0f; // Branch thickness multiplier (0.5-4.0)
  int maxIterations = 16; // Base KIFS iteration count (8-32)

  // Animation
  float zoomSpeed = 0.5f; // Zoom animation rate (-3.0-3.0, negative=outward)
  float rotationSpeed = 0.0f; // Global rotation rate rad/s (-PI..PI)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define FRACTAL_TREE_CONFIG_FIELDS                                             \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, thickness,              \
      maxIterations, zoomSpeed, rotationSpeed, gradient, blendMode,            \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct FractalTreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float zoomAccum;     // CPU-accumulated: zoomAccum += zoomSpeed * deltaTime
  float rotationAccum; // CPU-accumulated: rotationAccum += rotationSpeed * dt
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int thicknessLoc;
  int maxIterLoc;
  int zoomAccumLoc;
  int rotationAccumLoc;
  int gradientLUTLoc;
} FractalTreeEffect;

// Returns true on success, false if shader fails to load
bool FractalTreeEffectInit(FractalTreeEffect *e, const FractalTreeConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void FractalTreeEffectSetup(FractalTreeEffect *e, const FractalTreeConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void FractalTreeEffectUninit(FractalTreeEffect *e);

// Registers modulatable params with the modulation engine
void FractalTreeRegisterParams(FractalTreeConfig *cfg);

FractalTreeEffect *GetFractalTreeEffect(PostEffect *pe);

#endif // FRACTAL_TREE_H
