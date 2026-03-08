// Vortex effect module
// Raymarched turbulent sphere with volumetric distortion and trail persistence

#ifndef VORTEX_H
#define VORTEX_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct VortexConfig {
  bool enabled = false;

  // Raymarching
  int marchSteps = 50;           // Ray budget (4-200)
  int turbulenceOctaves = 8;     // Distortion layers (2-12)
  float turbulenceGrowth = 0.8f; // Octave frequency growth (0.5-0.95)
  float sphereRadius = 0.5f;     // Seed shape size (0.1-3.0)
  float surfaceDetail = 40.0f;   // Step precision near surface (5.0-100.0)
  float cameraDistance = 6.0f;   // Depth into volume (3.0-20.0)
  float rotationSpeed = 0.3f;    // Y-axis spin rate radians/sec (-PI..PI)

  // Trail persistence
  float decayHalfLife = 2.0f; // Trail persistence seconds (0.1-10.0)
  float trailBlur = 1.0f;     // Trail blur amount (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity multiplier (0.1-10.0)
  float curve = 1.5f;       // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor when silent (0.0-1.0)

  // Color
  float colorSpeed = 0.5f; // LUT scroll rate over time (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  float colorStretch = 1.0f; // Spatial color frequency through depth (0.1-5.0)

  // Tonemap
  float brightness = 1.0f; // Intensity multiplier before tonemap (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define VORTEX_CONFIG_FIELDS                                                   \
  enabled, marchSteps, turbulenceOctaves, turbulenceGrowth, sphereRadius,      \
      surfaceDetail, cameraDistance, rotationSpeed, decayHalfLife, trailBlur,  \
      baseFreq, maxFreq, gain, curve, baseBright, colorSpeed, gradient,        \
      colorStretch, brightness, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct VortexEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float time;
  float colorPhase;
  float rotationAngle;
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceGrowthLoc;
  int sphereRadiusLoc;
  int surfaceDetailLoc;
  int cameraDistanceLoc;
  int rotationAngleLoc;
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
} VortexEffect;

// Returns true on success, false if shader fails to load
bool VortexEffectInit(VortexEffect *e, const VortexConfig *cfg, int width,
                      int height);

// Binds all uniforms, advances time accumulator, updates LUT texture
void VortexEffectSetup(VortexEffect *e, const VortexConfig *cfg,
                       float deltaTime, Texture2D fftTexture);

// Renders vortex into ping-pong trail buffer with decay blending
void VortexEffectRender(VortexEffect *e, const VortexConfig *cfg,
                        float deltaTime, int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void VortexEffectResize(VortexEffect *e, int width, int height);

// Unloads shader and frees LUT
void VortexEffectUninit(VortexEffect *e);

// Returns default config
VortexConfig VortexConfigDefault(void);

// Registers modulatable params with the modulation engine
void VortexRegisterParams(VortexConfig *cfg);

#endif // VORTEX_H
