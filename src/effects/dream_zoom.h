#ifndef DREAM_ZOOM_H
#define DREAM_ZOOM_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct DreamZoomConfig {
  bool enabled = false;

  // Variant - dropdown only, not modulatable
  int variant = 0; // 0 = DREAM, 1 = JACOBI

  // Animation (CPU-accumulated phases)
  float zoomSpeed =
      0.1f; // Continuous zoom rate (-1.0 to 1.0; +zoom out, -zoom in)
  float globalRotationSpeed = 0.1f; // UV plane rotation rate, rad/s
  float rotationSpeed = 0.0f;       // Inner fractal rotation rate, rad/s

  // Tiling
  float jacobiRepeats = 1.0f; // cn-tile cycles per zoom revolution (0.5-4.0)

  // Iteration
  float formulaMix = 0.25f; // exp <-> squaring blend (0-1)
  int iterations = 64;      // Per-pixel iteration cap (16-256)

  // Coloring
  float cmapScale = 85.0f; // LUT remap amplitude on trap minimum (1-200)
  float cmapOffset = 5.4f; // LUT remap phase shift (0-10)

  // 2D positions (DualLissajous-driven)
  DualLissajousConfig trapOffset = {.amplitude = 0.05f}; // Orbit-trap point
  DualLissajousConfig origin = {.amplitude = 0.3f}; // Pre-iteration translation
  DualLissajousConfig constantOffset = {.amplitude =
                                            0.03f}; // Per-iteration constant

  // Audio (FFT)
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DREAM_ZOOM_CONFIG_FIELDS                                               \
  enabled, variant, zoomSpeed, globalRotationSpeed, rotationSpeed,             \
      jacobiRepeats, formulaMix, iterations, cmapScale, cmapOffset,            \
      trapOffset, origin, constantOffset, baseFreq, maxFreq, gain, curve,      \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DreamZoomEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases
  float zoomPhase; // += zoomSpeed * dt; wrapped to [-1024, 1024]
  float
      globalRotationPhase; // += globalRotationSpeed * dt; wrapped to [-PI, PI]
  float rotationPhase;     // += rotationSpeed * dt; wrapped to [-PI, PI]

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;

  int variantLoc;
  int zoomPhaseLoc;
  int globalRotationPhaseLoc;
  int rotationPhaseLoc;
  int jacobiRepeatsLoc;
  int formulaMixLoc;
  int iterationsLoc;
  int cmapScaleLoc;
  int cmapOffsetLoc;
  int trapOffsetLoc;     // vec2 from DualLissajousUpdate(&cfg->trapOffset, ...)
  int originLoc;         // vec2 from DualLissajousUpdate(&cfg->origin, ...)
  int constantOffsetLoc; // vec2 from DualLissajousUpdate(&cfg->constantOffset,
                         // ...)

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DreamZoomEffect;

bool DreamZoomEffectInit(DreamZoomEffect *e, const DreamZoomConfig *cfg);
void DreamZoomEffectSetup(DreamZoomEffect *e, DreamZoomConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);
void DreamZoomEffectUninit(DreamZoomEffect *e);
void DreamZoomRegisterParams(DreamZoomConfig *cfg);

DreamZoomEffect *GetDreamZoomEffect(PostEffect *pe);

#endif // DREAM_ZOOM_H
