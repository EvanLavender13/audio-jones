// Isoflow effect module
// Isoflow: raymarched gyroid isosurface fly-through with FFT-reactive iso-lines

#ifndef ISOFLOW_H
#define ISOFLOW_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct IsoflowConfig {
  bool enabled = false;

  // Geometry
  float gyroidScale = 132.0f; // Primary gyroid cell size (20.0-200.0)
  float surfaceThickness =
      32.0f;                     // Surface wall thickness multiplier (1.0-64.0)
  int marchSteps = 96;           // Raymarch iterations (32-128)
  float turbulenceScale = 16.0f; // Turbulence warp wavelength (4.0-64.0)
  float turbulenceAmount = 16.0f; // Turbulence warp amplitude (0.0-32.0)
  float isoFreq = 0.7f;           // Iso-line ring frequency along z (0.1-2.0)
  float isoStrength = 1.0f;       // Iso-line effect blend (0.0-1.0)
  float baseOffset =
      100.0f; // Lateral offset to avoid gyroid origin (50.0-200.0)
  float tonemapGain = 0.00033f; // Brightness before tanh (0.0001-0.002)

  // Speed (CPU-accumulated)
  float flySpeed = 1.0f; // Z-axis fly-through speed (0.1-5.0)

  // Camera
  float camDrift = 0.3f; // UV-space camera drift from lissajous (0.0-1.0)
  DualLissajousConfig lissajous = {
      .amplitude = 0.8f, .motionSpeed = 1.0f, .freqX1 = 0.6f, .freqY1 = 0.3f};

  // Audio
  float baseFreq = 55.0f;   // FFT low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define ISOFLOW_CONFIG_FIELDS                                                  \
  enabled, gyroidScale, surfaceThickness, marchSteps, turbulenceScale,         \
      turbulenceAmount, isoFreq, isoStrength, baseOffset, tonemapGain,         \
      flySpeed, camDrift, lissajous, baseFreq, maxFreq, gain, curve,           \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct IsoflowEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int flyPhaseLoc;
  int marchStepsLoc;
  int gyroidScaleLoc;
  int surfaceThicknessLoc;
  int turbulenceScaleLoc;
  int turbulenceAmountLoc;
  int isoFreqLoc;
  int isoStrengthLoc;
  int baseOffsetLoc;
  int tonemapGainLoc;
  int camDriftLoc;
  int panLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} IsoflowEffect;

// Returns true on success, false if shader fails to load
bool IsoflowEffectInit(IsoflowEffect *e, const IsoflowConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: DualLissajousUpdate mutates internal phase state
void IsoflowEffectSetup(IsoflowEffect *e, IsoflowConfig *cfg, float deltaTime,
                        const Texture2D &fftTexture);

// Unloads shader and frees LUT
void IsoflowEffectUninit(IsoflowEffect *e);

// Registers modulatable params with the modulation engine
void IsoflowRegisterParams(IsoflowConfig *cfg);

IsoflowEffect *GetIsoflowEffect(PostEffect *pe);

#endif // ISOFLOW_H
