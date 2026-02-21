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

  // Trail persistence
  float decayHalfLife =
      2.0f; // Trail persistence duration in seconds (0.1-10.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity multiplier (0.1-10.0)
  float curve = 1.0f;       // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.1f;  // Minimum brightness floor when silent (0.0-1.0)

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
      cameraDistance, decayHalfLife, baseFreq, maxFreq, gain, curve,           \
      baseBright, colorFreq, colorSpeed, brightness, exposure, gradient,       \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct MuonsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float time;
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
  int previousFrameLoc;
  int decayFactorLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} MuonsEffect;

// Returns true on success, false if shader fails to load
bool MuonsEffectInit(MuonsEffect *e, const MuonsConfig *cfg, int width,
                     int height);

// Binds all uniforms, advances time accumulator, updates LUT texture
void MuonsEffectSetup(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                      Texture2D fftTexture);

// Renders muons into ping-pong trail buffer with decay blending
void MuonsEffectRender(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                       int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void MuonsEffectResize(MuonsEffect *e, int width, int height);

// Unloads shader and frees LUT
void MuonsEffectUninit(MuonsEffect *e);

// Returns default config
MuonsConfig MuonsConfigDefault(void);

// Registers modulatable params with the modulation engine
void MuonsRegisterParams(MuonsConfig *cfg);

#endif // MUONS_H
