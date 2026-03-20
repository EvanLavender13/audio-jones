// Protean Clouds effect module
// Protean Clouds: raymarched volumetric clouds with morphing noise and
// FFT-reactive density

#ifndef PROTEAN_CLOUDS_H
#define PROTEAN_CLOUDS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ProteanCloudsConfig {
  bool enabled = false;

  // Volume
  float morph = 0.5f;       // Cloud morphology (0.0-1.0)
  float turbulence = 0.15f; // Displacement amplitude in noise loop (0.05-0.5)
  float densityThreshold = 0.3f; // Minimum density for visible cloud (0.0-1.0)
  int marchSteps = 80;           // Raymarch iterations (40-130)

  // Color
  float colorBlend = 0.3f;   // LUT index: 0=density, 1=depth (0.0-1.0)
  float fogIntensity = 0.5f; // Atmospheric fog amount (0.0-1.0)

  // Motion
  float speed = 3.0f; // Flight speed (0.5-6.0)

  // Audio
  float baseFreq = 55.0f;   // FFT low frequency Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define PROTEAN_CLOUDS_CONFIG_FIELDS                                           \
  enabled, morph, turbulence, densityThreshold, marchSteps, colorBlend,        \
      fogIntensity, speed, baseFreq, maxFreq, gain, curve, baseBright,         \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ProteanCloudsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;     // Raw animation clock accumulator (1x rate)
  float flyPhase; // Flight z-position accumulator (speed * deltaTime)
  int resolutionLoc;
  int timeLoc;
  int flyPhaseLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int morphLoc;
  int colorBlendLoc;
  int fogIntensityLoc;
  int turbulenceLoc;
  int densityThresholdLoc;
  int marchStepsLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ProteanCloudsEffect;

// Returns true on success, false if shader fails to load
bool ProteanCloudsEffectInit(ProteanCloudsEffect *e,
                             const ProteanCloudsConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void ProteanCloudsEffectSetup(ProteanCloudsEffect *e,
                              const ProteanCloudsConfig *cfg, float deltaTime,
                              const Texture2D &fftTexture);

// Unloads shader and frees LUT
void ProteanCloudsEffectUninit(ProteanCloudsEffect *e);

// Registers modulatable params with the modulation engine
void ProteanCloudsRegisterParams(ProteanCloudsConfig *cfg);

#endif // PROTEAN_CLOUDS_H
