// Quadtree generator - recursive subdivision around moving source points

#ifndef QUADTREE_H
#define QUADTREE_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct QuadtreeConfig {
  bool enabled = false;

  // Audio (standard generator FFT)
  float baseFreq = 55.0f;   // 27.5-440
  float maxFreq = 14000.0f; // 1000-16000
  float gain = 2.0f;        // 0.1-10
  float curve = 1.5f;       // 0.1-3
  float baseBright = 0.15f; // 0-1

  // Geometry
  int maxIterations = 8;       // 1-8 (quadtree depth cap)
  float lineWidth = 2.0f;      // 0.5-4.0 (pixels)
  float cellFillAmount = 0.0f; // 0.0-1.0 (interior fill brightness)

  // Sources
  int pointCount = 6;            // 1-8
  DualLissajousConfig lissajous; // Shared Lissajous controls (motion)

  // Color
  int colorMode = 0; // 0=depth, 1=hash

  // Color/Output (standard generator)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // 0.0-5.0
};

#define QUADTREE_CONFIG_FIELDS                                                 \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, maxIterations,          \
      lineWidth, cellFillAmount, pointCount, lissajous, colorMode, gradient,   \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct QuadtreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  int resolutionLoc;
  int sourcesLoc;
  int pointCountLoc;
  int maxIterationsLoc;
  int lineWidthLoc;
  int cellFillAmountLoc;
  int colorModeLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} QuadtreeEffect;

// Returns true on success, false if shader fails to load
bool QuadtreeEffectInit(QuadtreeEffect *e, const QuadtreeConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const cfg because Lissajous mutates phase each frame.
void QuadtreeEffectSetup(QuadtreeEffect *e, QuadtreeConfig *cfg,
                         float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void QuadtreeEffectUninit(QuadtreeEffect *e);

// Registers modulatable params with the modulation engine
void QuadtreeRegisterParams(QuadtreeConfig *cfg);

QuadtreeEffect *GetQuadtreeEffect(PostEffect *pe);

#endif // QUADTREE_H
