#ifndef DREAM_ZOOM_H
#define DREAM_ZOOM_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct DreamZoomConfig {
  bool enabled = false;

  // Animation (CPU-accumulated phases)
  float zoomSpeed = 0.125f;         // Continuous zoom rate (-1.0 to 1.0)
  float globalRotationSpeed = 0.1f; // Outer plane rotation rate, rad/s
  float rotationSpeed = 0.0f;       // Inner per-pixel rotation rate, rad/s

  // Tiling
  int jacobiRepeats = 1; // cn-tile cycles per zoom revolution (1-8); int only,
                         // fractional values break the periodicity
  float spiralWrap = 4.0f; // Zoom wrap divisor (1.0-8.0); reference uses 4

  // Iteration
  float formulaMix = 0.25f; // Per-iter map blend: 0 = pure complex exp(z),
                            // 1 = pure z*z (Mandelbrot-like)
  int iterations = 128;     // Per-pixel iteration cap (16-1024)

  // Per-pixel UV transform (reference iCoordinateScale, iOffset)
  float coordinateScale = 0.77f; // Pre-iteration scale exponent (0.0-2.0)
  float offsetX = -1252.38f;     // Pre-iteration UV shift X (-2000 to 2000)
  float offsetY = -1818.59f;     // Pre-iteration UV shift Y (-2000 to 2000)

  // Coloring (reference iCMAPScale, iCMAPOffset)
  float cmapScale = 85.11f; // LUT remap amplitude on trap minimum (1-300)
  float cmapOffset = 5.43f; // LUT remap phase shift (0-10)

  // Static fractal params (reference iTrapOffset, iOrigin, iConstantMapOffset)
  float trapOffsetX = 0.04f; // Moebius trap singularity X (-2.0 to 2.0)
  float trapOffsetY = -1.2f; // Moebius trap singularity Y (-2.0 to 2.0)
  float originX = 34.132f;   // Per-iter shift X, scaled by 0.01 (-100 to 100)
  float originY = -9.922f;   // Per-iter shift Y, scaled by 0.01 (-100 to 100)
  float constantOffsetX = 3.04f; // Per-iter add X, scaled by 0.01 (-300 to 300)
  float constantOffsetY =
      -2.36f; // Per-iter add Y, scaled by 0.01 (-300 to 300)

  // Polish
  int sampleCount = 2;        // Vogel-disk DOF samples per pixel (1-8)
  float grainAmount = 0.025f; // Hash grain amplitude (0-0.1)

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
  enabled, zoomSpeed, globalRotationSpeed, rotationSpeed, jacobiRepeats,       \
      spiralWrap, formulaMix, iterations, coordinateScale, offsetX, offsetY,   \
      cmapScale, cmapOffset, trapOffsetX, trapOffsetY, originX, originY,       \
      constantOffsetX, constantOffsetY, sampleCount, grainAmount, baseFreq,    \
      maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DreamZoomEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases
  float zoomPhase;
  float globalRotationPhase;
  float rotationPhase;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;

  int zoomPhaseLoc;
  int globalRotationPhaseLoc;
  int rotationPhaseLoc;
  int jacobiRepeatsLoc;
  int spiralWrapLoc;
  int formulaMixLoc;
  int iterationsLoc;
  int coordinateScaleLoc;
  int offsetLoc;
  int cmapScaleLoc;
  int cmapOffsetLoc;
  int trapOffsetLoc;
  int originLoc;
  int constantOffsetLoc;
  int sampleCountLoc;
  int grainAmountLoc;

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DreamZoomEffect;

bool DreamZoomEffectInit(DreamZoomEffect *e, const DreamZoomConfig *cfg);
void DreamZoomEffectSetup(DreamZoomEffect *e, const DreamZoomConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);
void DreamZoomEffectUninit(DreamZoomEffect *e);
void DreamZoomRegisterParams(DreamZoomConfig *cfg);

DreamZoomEffect *GetDreamZoomEffect(PostEffect *pe);

#endif // DREAM_ZOOM_H
