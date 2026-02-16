// Iris rings effect module
// Concentric ring arcs driven by FFT energy with per-ring differential
// rotation, arc gating capped at half circle, and perspective tilt

#ifndef IRIS_RINGS_H
#define IRIS_RINGS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct IrisRingsConfig {
  bool enabled = false;

  // Ring layout
  int layers = 24;            // Number of concentric rings (4-96)
  float ringScale = 0.3f;     // Total radius of outermost ring (0.05-0.8)
  float rotationSpeed = 0.2f; // Global rotation rate rad/s (-PI to PI)

  // Perspective
  float tilt = 0.0f;      // Perspective tilt amount (0-3)
  float tiltAngle = 0.0f; // Tilt direction in radians (-PI to PI)

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped frequency Hz (27.5-440)
  float maxFreq = 14000.0f; // Highest mapped frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.0f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.05f; // Minimum ring brightness (0-1)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend strength (0-5)
};

#define IRIS_RINGS_CONFIG_FIELDS                                               \
  enabled, layers, ringScale, rotationSpeed, tilt, tiltAngle, baseFreq,        \
      maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct IrisRingsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int layersLoc;
  int ringScaleLoc;
  int rotationAccumLoc;
  int tiltLoc;
  int tiltAngleLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} IrisRingsEffect;

// Returns true on success, false if shader fails to load
bool IrisRingsEffectInit(IrisRingsEffect *e, const IrisRingsConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void IrisRingsEffectSetup(IrisRingsEffect *e, const IrisRingsConfig *cfg,
                          float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void IrisRingsEffectUninit(IrisRingsEffect *e);

// Returns default config
IrisRingsConfig IrisRingsConfigDefault(void);

// Registers modulatable params with the modulation engine
void IrisRingsRegisterParams(IrisRingsConfig *cfg);

#endif // IRIS_RINGS_H
