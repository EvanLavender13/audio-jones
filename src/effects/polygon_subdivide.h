// Polygon Subdivide generator - recursive convex polygon subdivision with
// arbitrary-angle cuts

#ifndef POLYGON_SUBDIVIDE_H
#define POLYGON_SUBDIVIDE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PolygonSubdivideConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Geometry
  float speed = 0.45f;     // Animation rate (0.1-2.0)
  float threshold = 0.15f; // Subdivision probability cutoff (0.01-0.9)
  int maxIterations = 8;   // Maximum subdivision depth (2-20)

  // Visual
  float edgeDarken = 0.75f; // SDF edge/shadow darkening intensity (0.0-1.0)
  float areaFade = 0.0007f; // Area below which cells dissolve (0.0001-0.005)
  float desatThreshold =
      0.5f;                 // Hash above which desaturation applies (0.0-1.0)
  float desatAmount = 0.9f; // Strength of desaturation on gated cells (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define POLYGON_SUBDIVIDE_CONFIG_FIELDS                                        \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, speed, threshold,       \
      maxIterations, edgeDarken, areaFade, desatThreshold, desatAmount,        \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct PolygonSubdivideEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // CPU-accumulated animation phase
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int thresholdLoc;
  int maxIterationsLoc;
  int edgeDarkenLoc;
  int areaFadeLoc;
  int desatThresholdLoc;
  int desatAmountLoc;
  int gradientLUTLoc;
} PolygonSubdivideEffect;

// Returns true on success, false if shader fails to load
bool PolygonSubdivideEffectInit(PolygonSubdivideEffect *e,
                                const PolygonSubdivideConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void PolygonSubdivideEffectSetup(PolygonSubdivideEffect *e,
                                 const PolygonSubdivideConfig *cfg,
                                 float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void PolygonSubdivideEffectUninit(PolygonSubdivideEffect *e);

// Registers modulatable params with the modulation engine
void PolygonSubdivideRegisterParams(PolygonSubdivideConfig *cfg);

#endif // POLYGON_SUBDIVIDE_H
