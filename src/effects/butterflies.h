// Butterflies effect module
// FFT-driven Voronoi tile generator producing scrolling wing-shaped cells with
// gradient output

#ifndef BUTTERFLIES_H
#define BUTTERFLIES_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct ButterfliesConfig {
  bool enabled = false;

  // Audio
  float baseFreq = 55.0f;   // Lowest mapped pitch Hz (27.5-440)
  float maxFreq = 14000.0f; // Highest mapped pitch Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplification (0.1-10)
  float curve = 1.5f;       // Contrast exponent (0.1-3)
  float baseBright = 0.15f; // Floor brightness (0-1)

  // Geometry
  float spread = 1.0f;        // Tile-space multiplier; higher = more, smaller
                              // butterflies (0.2-5.0)
  float patternDetail = 1.0f; // Voronoi density multiplier (0.1-3.0)
  float wingShape = 1.0f;     // Wing curvature exponent (0.2-3.0)

  // Animation
  float scrollSpeed = 0.3f; // Vertical scroll rate tiles/s (-2.0-2.0)
  float driftSpeed = 0.0f;  // Horizontal drift rate tiles/s (-1.0-1.0)
  float flapSpeed = 0.05f;  // Wing flap frequency cycles/s (0.0-1.0)
  float shiftSpeed = 0.5f;  // Pattern mutation rate steps/s (0.1-4.0)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define BUTTERFLIES_CONFIG_FIELDS                                              \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, spread, patternDetail,  \
      wingShape, scrollSpeed, driftSpeed, flapSpeed, shiftSpeed, gradient,     \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ButterfliesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  // CPU-accumulated phases (avoid jumps when speed slider moves)
  float scrollPhase;
  float driftPhase;
  float flapPhase;
  float shiftPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int scrollPhaseLoc;
  int driftPhaseLoc;
  int flapPhaseLoc;
  int shiftPhaseLoc;
  int flapSpeedLoc;
  int spreadLoc;
  int patternDetailLoc;
  int wingShapeLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ButterfliesEffect;

// Returns true on success, false if shader fails to load
bool ButterfliesEffectInit(ButterfliesEffect *e, const ButterfliesConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void ButterfliesEffectSetup(ButterfliesEffect *e, const ButterfliesConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void ButterfliesEffectUninit(ButterfliesEffect *e);

// Registers modulatable params with the modulation engine
void ButterfliesRegisterParams(ButterfliesConfig *cfg);

ButterfliesEffect *GetButterfliesEffect(PostEffect *pe);

#endif // BUTTERFLIES_H
