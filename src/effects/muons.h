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
  int mode = 0;           // Distance function mode (0-6)
  int turbulenceMode = 0; // Turbulence waveform (0-6)
  int marchSteps =
      10; // Trail density - more steps reveal more filaments (4-200)
  int turbulenceOctaves =
      9; // Path complexity - fewer = smooth, more = chaotic (1-12)
  float turbulenceStrength = 1.0f; // FBM displacement amplitude (0.0-2.0)
  float ringThickness = 0.03f;     // Wire gauge of trails (0.005-0.1)
  float cameraDistance = 9.0f;     // Depth into volume (3.0-20.0)
  float phaseX = 0.717f; // Rotation axis X phase offset (-PI_F to PI_F)
  float phaseY = 1.0f;   // Rotation axis Y phase offset (-PI_F to PI_F)
  float phaseZ = 0.0f;   // Rotation axis Z phase offset (-PI_F to PI_F)
  float drift =
      0.0f; // Per-axis speed divergence - 0 = vanilla cycling (0.0-0.5)
  float axisFeedback = 1.0f; // Turbulence-to-axis coupling - 1 = filaments, <1
                             // = coherent swirls (0.0-2.0)

  int colorMode = 0; // 0 = Winner-takes-all, 1 = Additive volume (0-1)

  // Trail persistence
  float decayHalfLife =
      2.0f; // Trail persistence duration in seconds (0.1-10.0)
  float trailBlur =
      1.0f; // Trail blur amount - 0 sharp, 1 full gaussian (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity multiplier (0.1-10.0)
  float curve = 1.0f;       // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor when silent (0.0-1.0)

  // Color
  float colorSpeed = 0.5f; // LUT scroll rate over time (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  float colorStretch =
      1.67f; // Spatial color frequency - higher = tighter bands (0.1-5.0)

  // Tonemap
  float brightness = 1.0f; // Intensity multiplier before tonemap (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define MUONS_CONFIG_FIELDS                                                    \
  enabled, mode, turbulenceMode, marchSteps, turbulenceOctaves,                \
      turbulenceStrength, ringThickness, cameraDistance, phaseX, phaseY,       \
      phaseZ, drift, axisFeedback, colorMode, decayHalfLife, trailBlur,        \
      baseFreq, maxFreq, gain, curve, baseBright, colorSpeed, colorStretch,    \
      brightness, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct MuonsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  int modeLoc;
  int turbulenceModeLoc;
  Texture2D currentFFTTexture;
  float time;
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceStrengthLoc;
  int ringThicknessLoc;
  int cameraDistanceLoc;
  int phaseLoc;
  int driftLoc;
  int axisFeedbackLoc;
  int colorModeLoc;
  float colorPhase;
  int colorPhaseLoc;
  int colorStretchLoc;
  int brightnessLoc;
  int gradientLUTLoc;
  int previousFrameLoc;
  int decayFactorLoc;
  int trailBlurLoc;
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
                      const Texture2D &fftTexture);

// Renders muons into ping-pong trail buffer with decay blending
void MuonsEffectRender(MuonsEffect *e, const MuonsConfig *cfg, float deltaTime,
                       int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void MuonsEffectResize(MuonsEffect *e, int width, int height);

// Unloads shader and frees LUT
void MuonsEffectUninit(MuonsEffect *e);

// Registers modulatable params with the modulation engine
void MuonsRegisterParams(MuonsConfig *cfg);

#endif // MUONS_H
