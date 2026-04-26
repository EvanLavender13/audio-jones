// Dancing lines effect module
// Lissajous-driven line segment that snaps on a clock and leaves a fading
// trail of past poses with per-echo gradient color and FFT band brightness

#ifndef DANCING_LINES_EFFECT_H
#define DANCING_LINES_EFFECT_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct DancingLinesConfig {
  bool enabled = false;

  // Lissajous motion (shared dual-harmonic config)
  DualLissajousConfig lissajous = {
      .amplitude = 0.5f,
      .motionSpeed = 0.5f,
      .freqX1 = 2.0f,
      .freqY1 = 3.0f,
      .freqX2 = 0.0f,
      .freqY2 = 0.0f,
      .offsetX2 = 0.0f,
      .offsetY2 = 0.0f,
  };

  // Snap clock and trail
  float snapRate = 8.0f;        // Hz at which line snaps (1-60)
  int trailLength = 10;         // Number of echoes (1-32)
  float trailFade = 0.85f;      // Per-echo brightness multiplier (0.5-0.99)
  float colorPhaseStep = 0.08f; // t-step per echo (0.05-1.0)

  // Geometry
  float lineThickness =
      0.002f; // Inner smoothstep edge in R.y units (0.0005-0.02)
  float endpointOffset =
      1.5708f; // Parametric offset between the two endpoints (0.1-PI)

  // Glow
  float baseBright = 0.15f;   // Always-on per-echo floor (0-1)
  float glowIntensity = 1.0f; // Output multiplier (0-3)

  // FFT mapping
  float baseFreq = 55.0f;   // Newest echo band (27.5-440)
  float maxFreq = 14000.0f; // Oldest echo band (1000-16000)
  float gain = 2.0f;        // FFT magnitude scalar (0.1-10)
  float curve = 1.5f;       // FFT magnitude exponent (0.1-3)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DANCING_LINES_CONFIG_FIELDS                                            \
  enabled, lissajous, snapRate, trailLength, trailFade, colorPhaseStep,        \
      lineThickness, endpointOffset, baseBright, glowIntensity, baseFreq,      \
      maxFreq, gain, curve, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DancingLinesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float accumTime; // CPU-accumulated wall clock for snap
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int phaseLoc;
  int amplitudeLoc;
  int freqX1Loc;
  int freqY1Loc;
  int freqX2Loc;
  int freqY2Loc;
  int offsetX2Loc;
  int offsetY2Loc;
  int accumTimeLoc;
  int snapRateLoc;
  int trailLengthLoc;
  int trailFadeLoc;
  int colorPhaseStepLoc;
  int lineThicknessLoc;
  int endpointOffsetLoc;
  int baseBrightLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int gradientLUTLoc;
} DancingLinesEffect;

// Returns true on success, false if shader fails to load
bool DancingLinesEffectInit(DancingLinesEffect *e,
                            const DancingLinesConfig *cfg);

// Binds all uniforms including fftTexture, advances Lissajous phase and snap
// clock
void DancingLinesEffectSetup(DancingLinesEffect *e, DancingLinesConfig *cfg,
                             float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void DancingLinesEffectUninit(DancingLinesEffect *e);

// Registers modulatable params with the modulation engine
void DancingLinesRegisterParams(DancingLinesConfig *cfg);

#endif // DANCING_LINES_EFFECT_H
