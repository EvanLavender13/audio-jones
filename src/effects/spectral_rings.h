// Spectral rings generator
// Dense concentric rings with noise-texture coloring via gradient LUT,
// FFT-reactive brightness, elliptical deformation, and animated motion

#ifndef SPECTRAL_RINGS_H
#define SPECTRAL_RINGS_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SpectralRingsConfig {
  bool enabled = false;

  // Shape
  float eccentricity =
      0.3f; // Circle-to-ellipse stretch (0.0=circles, 0.3=reference, 0.0-1.0)
  float tiltAngle = -1.0f; // Stretch direction (radians, -PI-PI)

  // Center motion
  DualLissajousConfig lissajous = {
      .amplitude = 0.15f,
      .motionSpeed = 1.0f,
      .freqX1 = 0.07f,
      .freqY1 = 0.05f,
  };

  // Animation
  float pulseSpeed = 0.0f;      // Ring expansion rate (-2.0-2.0)
  float colorShiftSpeed = 0.0f; // Gradient scroll speed (-2.0-2.0)
  float rotationSpeed = 0.1f;   // Noise rotation speed (rads/sec, -PI-PI)

  // Noise
  float noiseScale = 0.25f; // Noise UV scale (0.05-2.0, 0.25 = 256px reference)

  // FFT mapping
  float baseFreq = 55.0f;   // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f; // High frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Quality
  int quality = 128; // Iteration count (8-256, higher=smoother/brighter)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend strength (0-5)
};

#define SPECTRAL_RINGS_CONFIG_FIELDS                                           \
  enabled, eccentricity, tiltAngle, lissajous, pulseSpeed, colorShiftSpeed,    \
      rotationSpeed, noiseScale, baseFreq, maxFreq, gain, curve, baseBright,   \
      quality, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpectralRingsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float pulseAccum;
  float colorShiftAccum;
  float rotationAccum;
  int resolutionLoc;
  int noiseTexLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int noiseScaleLoc;
  int qualityLoc;
  int pulseAccumLoc;
  int colorShiftAccumLoc;
  int rotationAccumLoc;
  int eccentricityLoc;
  int tiltAngleLoc;
  int centerLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} SpectralRingsEffect;

// Returns true on success, false if shader fails to load
bool SpectralRingsEffectInit(SpectralRingsEffect *e,
                             const SpectralRingsConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SpectralRingsEffectSetup(SpectralRingsEffect *e, SpectralRingsConfig *cfg,
                              float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void SpectralRingsEffectUninit(SpectralRingsEffect *e);

// Registers modulatable params with the modulation engine
void SpectralRingsRegisterParams(SpectralRingsConfig *cfg);

#endif // SPECTRAL_RINGS_H
