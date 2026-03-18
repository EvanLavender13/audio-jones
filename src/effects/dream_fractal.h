// Dream Fractal effect module
// FFT-driven Menger sponge raymarcher with carved spheres, orbital camera,
// turbulence coloring, and gradient output

#ifndef DREAM_FRACTAL_H
#define DREAM_FRACTAL_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct DreamFractalConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped pitch (27.5-440)
  float maxFreq = 14000.0f; // Highest mapped pitch (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10)
  float curve = 1.5f;       // Contrast exponent (0.1-3)
  float baseBright = 0.15f; // Floor brightness (0-1)

  // Camera
  float orbitSpeed = 0.2f;  // Camera orbit rate rad/s (-2.0-2.0)
  float driftSpeed = 0.05f; // Forward/backward movement speed (-0.5-0.5)

  // Fractal geometry
  int marchSteps = 70;       // Raymarch iterations (30-120)
  int fractalIters = 8;      // Detail levels (3-12)
  float sphereRadius = 0.9f; // Carved sphere size (0.3-1.5)
  float scaleFactor = 3.0f;  // Per-iteration scale ratio (2.0-5.0)

  // Color
  float colorScale = 10.0f;         // Turbulence spatial frequency (1.0-30.0)
  float turbulenceIntensity = 1.2f; // Domain warp strength (0.1-3.0)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DREAM_FRACTAL_CONFIG_FIELDS                                            \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, orbitSpeed, driftSpeed, \
      marchSteps, fractalIters, sphereRadius, scaleFactor, colorScale,         \
      turbulenceIntensity, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DreamFractalEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float orbitPhase; // Accumulated orbit angle
  float driftPhase; // Accumulated forward drift
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int orbitPhaseLoc;
  int driftPhaseLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int marchStepsLoc;
  int fractalItersLoc;
  int sphereRadiusLoc;
  int scaleFactorLoc;
  int colorScaleLoc;
  int turbulenceIntensityLoc;
  int gradientLUTLoc;
} DreamFractalEffect;

// Returns true on success, false if shader fails to load
bool DreamFractalEffectInit(DreamFractalEffect *e,
                            const DreamFractalConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void DreamFractalEffectSetup(DreamFractalEffect *e,
                             const DreamFractalConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture);

// Unloads shader and frees LUT
void DreamFractalEffectUninit(DreamFractalEffect *e);

// Registers modulatable params with the modulation engine
void DreamFractalRegisterParams(DreamFractalConfig *cfg);

#endif // DREAM_FRACTAL_H
