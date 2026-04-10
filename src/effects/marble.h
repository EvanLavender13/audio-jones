// Marble effect module
// FFT-driven inversive fractal raymarcher with orbit traps and gradient output

#ifndef MARBLE_H
#define MARBLE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct MarbleConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped pitch (27.5-440)
  float maxFreq = 14000.0f; // Highest mapped pitch (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10)
  float curve = 1.5f;       // Contrast exponent (0.1-3)
  float baseBright = 0.15f; // Floor brightness (0-1)

  // Geometry
  int fractalIters = 10;         // Fractal detail iterations (4-12)
  int marchSteps = 64;           // Raymarch sample count (32-128)
  float stepSize = 0.02f;        // Base ray step size (0.005-0.1)
  float foldScale = 0.7f;        // Inversive fold strength (0.3-1.2)
  float foldOffset = -0.7f;      // Attractor shift (-1.5-0.0)
  float trapSensitivity = 19.0f; // Orbit trap decay rate (5.0-40.0)
  float sphereRadius = 2.0f;     // Bounding sphere radius (1.0-3.0)

  // Animation
  float orbitSpeed = 0.1f;    // Camera orbit rate rad/s (-2.0-2.0)
  float perturbAmp = 0.15f;   // Fold animation strength (0.0-0.5)
  float perturbSpeed = 0.15f; // Fold animation rate rad/s (0.01-1.0)
  float zoom = 1.0f;          // Camera distance multiplier (0.5-3.0)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define MARBLE_CONFIG_FIELDS                                                   \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, fractalIters,           \
      marchSteps, stepSize, foldScale, foldOffset, trapSensitivity,            \
      sphereRadius, orbitSpeed, perturbAmp, perturbSpeed, zoom, gradient,      \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct MarbleEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float orbitPhase;   // CPU-accumulated orbit angle
  float perturbPhase; // CPU-accumulated perturb phase
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int orbitPhaseLoc;
  int perturbPhaseLoc;
  int fractalItersLoc;
  int marchStepsLoc;
  int stepSizeLoc;
  int foldScaleLoc;
  int foldOffsetLoc;
  int trapSensitivityLoc;
  int sphereRadiusLoc;
  int perturbAmpLoc;
  int zoomLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} MarbleEffect;

// Returns true on success, false if shader fails to load
bool MarbleEffectInit(MarbleEffect *e, const MarbleConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void MarbleEffectSetup(MarbleEffect *e, const MarbleConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void MarbleEffectUninit(MarbleEffect *e);

// Registers modulatable params with the modulation engine
void MarbleRegisterParams(MarbleConfig *cfg);

#endif // MARBLE_H
