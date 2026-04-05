// Color Stretch - FFT-reactive recursive grid zoom with glyph subdivision and
// Lissajous focus drift

#ifndef COLOR_STRETCH_H
#define COLOR_STRETCH_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ColorStretchConfig {
  bool enabled = false;

  // Zoom
  float zoomSpeed = 0.5f; // Zoom rate (-2.0 to 2.0)
  float zoomScale = 0.1f; // Overall zoom level (0.01-1.0)
  int glyphSize = 3;      // Grid subdivision size (2-8)
  int recursionCount = 6; // Fractal recursion depth (2-12)
  float curvature = 0.0f; // Dome/tunnel time warp (0.0-2.0)
  float spinSpeed = 0.0f; // UV rotation rate rad/s (-ROTATION_SPEED_MAX to
                          // ROTATION_SPEED_MAX)
  DualLissajousConfig lissajous = {
      .amplitude = 0.3f, .freqX1 = 0.07f, .freqY1 = 0.05f};

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped pitch (27.5-440)
  float maxFreq = 14000.0f; // Highest mapped pitch (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10)
  float curve = 1.5f;       // Contrast exponent (0.1-3)
  float baseBright = 0.15f; // Floor brightness (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define COLOR_STRETCH_CONFIG_FIELDS                                            \
  enabled, zoomSpeed, zoomScale, glyphSize, recursionCount, curvature,         \
      spinSpeed, lissajous.amplitude, lissajous.motionSpeed, lissajous.freqX1, \
      lissajous.freqY1, lissajous.freqX2, lissajous.freqY2,                    \
      lissajous.offsetX2, lissajous.offsetY2, baseFreq, maxFreq, gain, curve,  \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ColorStretchEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float zoomPhase; // Accumulated zoom: zoomPhase += zoomSpeed * dt
  float spinPhase; // Accumulated spin: spinPhase += spinSpeed * dt
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int zoomPhaseLoc;
  int zoomSpeedLoc;
  int zoomScaleLoc;
  int glyphSizeLoc;
  int recursionCountLoc;
  int curvatureLoc;
  int spinPhaseLoc;
  int focusOffsetLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} ColorStretchEffect;

// Returns true on success, false if shader fails to load
bool ColorStretchEffectInit(ColorStretchEffect *e,
                            const ColorStretchConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: DualLissajousUpdate mutates internal phase state
void ColorStretchEffectSetup(ColorStretchEffect *e, ColorStretchConfig *cfg,
                             float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void ColorStretchEffectUninit(ColorStretchEffect *e);

// Registers modulatable params with the modulation engine
void ColorStretchRegisterParams(ColorStretchConfig *cfg);

#endif // COLOR_STRETCH_H
