// Subdivide generator - recursive BSP with squishy Catmull-Rom cut positions

#ifndef SUBDIVIDE_H
#define SUBDIVIDE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct SubdivideConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Geometry
  float speed = 0.45f;     // Animation rate (0.1-2.0)
  float squish = 0.01f;    // Cut line waviness (0.001-0.05)
  float threshold = 0.15f; // Subdivision probability cutoff (0.01-0.9)
  int maxIterations = 14;  // Maximum BSP recursion depth (2-20)

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

#define SUBDIVIDE_CONFIG_FIELDS                                                \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, speed, squish,          \
      threshold, maxIterations, edgeDarken, areaFade, desatThreshold,          \
      desatAmount, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SubdivideEffect {
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
  int squishLoc;
  int thresholdLoc;
  int maxIterationsLoc;
  int edgeDarkenLoc;
  int areaFadeLoc;
  int desatThresholdLoc;
  int desatAmountLoc;
  int gradientLUTLoc;
} SubdivideEffect;

// Returns true on success, false if shader fails to load
bool SubdivideEffectInit(SubdivideEffect *e, const SubdivideConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SubdivideEffectSetup(SubdivideEffect *e, const SubdivideConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void SubdivideEffectUninit(SubdivideEffect *e);

// Registers modulatable params with the modulation engine
void SubdivideRegisterParams(SubdivideConfig *cfg);

SubdivideEffect *GetSubdivideEffect(PostEffect *pe);

#endif // SUBDIVIDE_H
