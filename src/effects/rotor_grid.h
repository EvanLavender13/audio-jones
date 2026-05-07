// Rotor Grid effect module - radial cell-grid generator with three coloring
// modes (smooth, wedge, random)

#ifndef ROTOR_GRID_H
#define ROTOR_GRID_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

enum RotorGridMode {
  ROTOR_GRID_MODE_SMOOTH = 0,
  ROTOR_GRID_MODE_WEDGE = 1,
  ROTOR_GRID_MODE_RANDOM = 2,
};

struct RotorGridConfig {
  bool enabled = false;

  // Coloring mode
  int mode = ROTOR_GRID_MODE_SMOOTH;

  // FFT mapping
  float baseFreq = 55.0f;   // (27.5-440.0)
  float maxFreq = 14000.0f; // (1000-16000)
  float gain = 2.0f;        // (0.1-10.0)
  float curve = 1.5f;       // (0.1-3.0)
  float baseBright = 0.15f; // (0.0-1.0)

  // Geometry
  float ringSpacing = 0.1f; // (0.05-0.5) radial scale; smaller = more rings
  int baseDivisions = 4;    // (1-8) base angular subdivisions
  float ringFrequency =
      3.14f;                // (1.0-6.283) ring spacing in cos(ringFrequency*l)
  float radialDrift = 0.0f; // (-PI..PI) ring phase shift for breathing

  // Animation
  float spinSpeed = 5.0f; // (-ROTATION_SPEED_MAX..ROTATION_SPEED_MAX) rad/s
  float differentialTwist =
      0.0f;               // (-2.0..2.0) outer-vs-inner rotation differential
  float driftRate = 0.0f; // (0.0..1.0) RANDOM mode hash drift rate

  // Mode-specific
  float wedgeWidth = 0.4f; // (0.0..PI) WEDGE mode angular half-width

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define ROTOR_GRID_CONFIG_FIELDS                                               \
  enabled, mode, baseFreq, maxFreq, gain, curve, baseBright, ringSpacing,      \
      baseDivisions, ringFrequency, radialDrift, spinSpeed, differentialTwist, \
      driftRate, wedgeWidth, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct RotorGridEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float spinPhase;  // CPU-accumulated rotation phase
  float driftPhase; // CPU-accumulated random-mode drift phase

  int resolutionLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int modeLoc;
  int ringSpacingLoc;
  int baseDivisionsLoc;
  int ringFrequencyLoc;
  int radialDriftLoc;
  int spinPhaseLoc;
  int differentialTwistLoc;
  int driftPhaseLoc;
  int wedgeWidthLoc;
} RotorGridEffect;

// Returns true on success, false if shader fails to load
bool RotorGridEffectInit(RotorGridEffect *e, const RotorGridConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void RotorGridEffectSetup(RotorGridEffect *e, const RotorGridConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void RotorGridEffectUninit(RotorGridEffect *e);

// Registers modulatable params with the modulation engine
void RotorGridRegisterParams(RotorGridConfig *cfg);

RotorGridEffect *GetRotorGridEffect(PostEffect *pe);

#endif // ROTOR_GRID_H
