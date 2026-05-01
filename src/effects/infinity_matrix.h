// Infinity Matrix - infinite recursive fractal zoom through self-similar binary
// digit glyphs

#ifndef INFINITY_MATRIX_H
#define INFINITY_MATRIX_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct InfinityMatrixConfig {
  bool enabled = false;

  // Zoom
  float zoomSpeed = 1.0f; // Zoom rate in levels/s (-3.0 to 3.0)
  float zoomScale = 0.1f; // Base zoom multiplier (0.01-1.0)
  int recursionDepth = 5; // Fractal levels to render (2-8)
  float fadeDepth =
      3.0f; // Levels visible simultaneously before fading (1.0-6.0)

  // Wave
  float waveAmplitude = 0.1f; // UV wave distortion amount (0.0-0.5)
  float waveFreq = 2.0f;      // UV wave spatial frequency (0.5-8.0)
  float waveSpeed = 1.0f;     // UV wave animation rate (0.1-3.0)

  // Audio
  bool colorFreqMap =
      false;              // Map FFT bands to glyph hash color instead of depth
  float baseFreq = 55.0f; // Low FFT frequency (27.5-440)
  float maxFreq = 14000.0f; // High FFT frequency (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10)
  float curve = 1.5f;       // FFT response curve (0.1-3)
  float baseBright = 0.15f; // Floor brightness without audio (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define INFINITY_MATRIX_CONFIG_FIELDS                                          \
  enabled, zoomSpeed, zoomScale, recursionDepth, fadeDepth, waveAmplitude,     \
      waveFreq, waveSpeed, colorFreqMap, baseFreq, maxFreq, gain, curve,       \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct InfinityMatrixEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float zoomPhase; // Accumulated zoom: zoomPhase += zoomSpeed * dt
  float wavePhase; // Accumulated wave: wavePhase += waveSpeed * dt
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int zoomPhaseLoc;
  int zoomScaleLoc;
  int recursionDepthLoc;
  int fadeDepthLoc;
  int waveAmplitudeLoc;
  int waveFreqLoc;
  int wavePhaseLoc;
  int colorFreqMapLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} InfinityMatrixEffect;

// Returns true on success, false if shader fails to load
bool InfinityMatrixEffectInit(InfinityMatrixEffect *e,
                              const InfinityMatrixConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void InfinityMatrixEffectSetup(InfinityMatrixEffect *e,
                               const InfinityMatrixConfig *cfg, float deltaTime,
                               const Texture2D &fftTexture);

// Unloads shader and frees LUT
void InfinityMatrixEffectUninit(InfinityMatrixEffect *e);

// Registers modulatable params with the modulation engine
void InfinityMatrixRegisterParams(InfinityMatrixConfig *cfg);

InfinityMatrixEffect *GetInfinityMatrixEffect(PostEffect *pe);

#endif // INFINITY_MATRIX_H
