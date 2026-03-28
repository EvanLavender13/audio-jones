// Viscera effect module
// Organic domain-warped noise with bump-mapped lighting and radial pulsation

#ifndef VISCERA_H
#define VISCERA_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct VisceraConfig {
  bool enabled = false;

  // Geometry
  float scale = 9.0f; // Pattern zoom - lower = larger structures (1.0-20.0)
  float twistAngle = -1.28f;  // Rotation per iteration in radians
                              // (-ROTATION_OFFSET_MAX..ROTATION_OFFSET_MAX)
  int iterations = 16;        // Octave count (4-24)
  float freqGrowth = 1.2f;    // Frequency multiplier per octave (1.05-1.5)
  float warpIntensity = 1.0f; // Domain warp feedback strength (0.0-2.0)

  // Animation
  float animSpeed = 4.0f; // Phase accumulation rate (0.1-8.0)

  // Pulsation
  float pulseAmplitude = 0.8f; // Radial pulsation strength (0.0-2.0)
  float pulseFreq = 4.0f;      // Pulsation time frequency (0.5-8.0)
  float pulseRadial = 6.0f;    // Radial ring density (1.0-12.0)

  // Lighting
  float bumpStrength =
      20.0f; // Surface relief intensity - higher = deeper bumps (2.0-100.0)
  float specPower = 32.0f;    // Specular shininess exponent (4.0-128.0)
  float specIntensity = 0.8f; // Specular highlight brightness (0.0-2.0)

  // Height
  float heightScale = 1.2f; // Height-to-gradient mapping scale (0.5-3.0)

  // Audio
  float baseFreq = 55.0f;   // FFT low frequency bound Hz (27.5-440.0)
  float maxFreq = 14000.0f; // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT gain (0.1-10.0)
  float curve = 1.5f;       // FFT contrast curve (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define VISCERA_CONFIG_FIELDS                                                  \
  enabled, scale, twistAngle, iterations, freqGrowth, warpIntensity,           \
      animSpeed, pulseAmplitude, pulseFreq, pulseRadial, bumpStrength,         \
      specPower, specIntensity, heightScale, baseFreq, maxFreq, gain, curve,   \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct VisceraEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  Texture2D currentFFTTexture;
  float time; // CPU-accumulated animation phase
  int resolutionLoc;
  int timeLoc;
  int scaleLoc;
  int twistAngleLoc;
  int iterationsLoc;
  int freqGrowthLoc;
  int warpIntensityLoc;
  int pulseAmplitudeLoc;
  int pulseFreqLoc;
  int pulseRadialLoc;
  int bumpStrengthLoc;
  int specPowerLoc;
  int specIntensityLoc;
  int heightScaleLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} VisceraEffect;

// Returns true on success, false if shader fails to load
bool VisceraEffectInit(VisceraEffect *e, const VisceraConfig *cfg);

// Binds all uniforms, advances time accumulator, updates LUT texture
void VisceraEffectSetup(VisceraEffect *e, const VisceraConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void VisceraEffectUninit(VisceraEffect *e);

// Registers modulatable params with the modulation engine
void VisceraRegisterParams(VisceraConfig *cfg);

#endif // VISCERA_H
