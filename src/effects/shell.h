// Shell effect module
// Raymarched hollow sphere with outline contours from per-step view rotation

#ifndef SHELL_H
#define SHELL_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ShellConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 60; // Ray budget — more steps = denser contours (4-200)
  int turbulenceOctaves = 4;     // Distortion layers (2-12)
  float turbulenceGrowth = 2.0f; // Octave frequency multiplier (1.2-3.0)
  float sphereRadius = 3.0f;     // Hollow sphere size (0.5-10.0)
  float ringThickness = 0.1f;    // Step size multiplier (0.01-0.5)
  float cameraDistance = 9.0f;   // Depth into volume (3.0-20.0)
  float phaseX = 0.0f;           // Rotation axis X phase offset (-PI_F to PI_F)
  float phaseY = 2.0f;           // Rotation axis Y phase offset (-PI_F to PI_F)
  float phaseZ = 4.0f;           // Rotation axis Z phase offset (-PI_F to PI_F)
  float outlineSpread =
      0.1f; // Per-step rotation amount — 0 solid, higher wireframe (0.0-0.5)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity multiplier (0.1-10.0)
  float curve = 1.5f;       // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Color
  float colorSpeed = 0.5f; // LUT scroll rate (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  float colorStretch = 1.0f; // Spatial color frequency through depth (0.1-5.0)

  // Tonemap
  float brightness = 1.0f; // Intensity multiplier before tonemap (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define SHELL_CONFIG_FIELDS                                                    \
  enabled, marchSteps, turbulenceOctaves, turbulenceGrowth, sphereRadius,      \
      ringThickness, cameraDistance, phaseX, phaseY, phaseZ, outlineSpread,    \
      baseFreq, maxFreq, gain, curve, baseBright, colorSpeed, colorStretch,    \
      brightness, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ShellEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  Texture2D currentFFTTexture;
  float time;
  float colorPhase;
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceGrowthLoc;
  int sphereRadiusLoc;
  int ringThicknessLoc;
  int cameraDistanceLoc;
  int phaseLoc;
  int outlineSpreadLoc;
  int colorStretchLoc;
  int colorPhaseLoc;
  int brightnessLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ShellEffect;

// Returns true on success, false if shader fails to load
bool ShellEffectInit(ShellEffect *e, const ShellConfig *cfg);

// Binds all uniforms, advances time accumulator, updates LUT texture
void ShellEffectSetup(ShellEffect *e, const ShellConfig *cfg, float deltaTime,
                      Texture2D fftTexture);

// Unloads shader and frees LUT
void ShellEffectUninit(ShellEffect *e);

// Returns default config
ShellConfig ShellConfigDefault(void);

// Registers modulatable params with the modulation engine
void ShellRegisterParams(ShellConfig *cfg);

#endif // SHELL_H
