// Muons effect module
// Raymarched turbulent ring trails through a volumetric noise field

#ifndef MUONS_H
#define MUONS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct MuonsConfig {
  bool enabled = false;

  // Raymarching
  int marchSteps =
      10; // Trail density — more steps reveal more filaments (4-40)
  int turbulenceOctaves =
      9; // Path complexity — fewer = smooth, more = chaotic (1-12)
  float turbulenceStrength = 1.0f; // FBM displacement amplitude (0.0-2.0)
  float ringThickness = 0.03f;     // Wire gauge of trails (0.005-0.1)
  float cameraDistance = 9.0f;     // Depth into volume (3.0-20.0)

  // Color
  float colorFreq = 33.0f; // Color cycles along ray depth (0.5-50.0)
  float colorSpeed = 0.5f; // LUT scroll rate over time (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Tonemap
  float brightness = 1.0f;  // Intensity multiplier before tonemap (0.1-5.0)
  float exposure = 3000.0f; // Tonemap divisor — lower = brighter (500-10000)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define MUONS_CONFIG_FIELDS                                                    \
  enabled, marchSteps, turbulenceOctaves, turbulenceStrength, ringThickness,   \
      cameraDistance, colorFreq, colorSpeed, brightness, exposure, gradient,   \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct MuonsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // CPU-accumulated animation phase
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceStrengthLoc;
  int ringThicknessLoc;
  int cameraDistanceLoc;
  int colorFreqLoc;
  int colorSpeedLoc;
  int brightnessLoc;
  int exposureLoc;
  int gradientLUTLoc;
} MuonsEffect;

// Returns true on success, false if shader fails to load
bool MuonsEffectInit(MuonsEffect *e, const MuonsConfig *cfg);

// Binds all uniforms, advances time accumulator, updates LUT texture
void MuonsEffectSetup(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime);

// Unloads shader and frees LUT
void MuonsEffectUninit(MuonsEffect *e);

// Returns default config
MuonsConfig MuonsConfigDefault(void);

// Registers modulatable params with the modulation engine
void MuonsRegisterParams(MuonsConfig *cfg);

#endif // MUONS_H
