// Spectral rings generator
// Full-screen concentric rings colored by FFT-driven gradient LUT with
// elliptical deformation, noise variation, and animated pulse/rotation

#ifndef SPECTRAL_RINGS_H
#define SPECTRAL_RINGS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SpectralRingsConfig {
  bool enabled = false;

  // Ring layout
  float ringDensity = 24.0f; // Visible ring bands (4.0-64.0)
  float ringWidth = 0.5f;    // Bright band vs dark gap (0.05-1.0)
  int layers = 48;           // FFT sampling resolution across radius (8-128)

  // Animation
  float pulseSpeed =
      0.0f; // Ring expansion/contraction rate (rads/sec, -2.0-2.0)
  float colorShiftSpeed = 0.1f; // LUT scroll speed (rads/sec, -2.0-2.0)
  float rotationSpeed = 0.1f; // Ring structure rotation rate (rads/sec, -PI-PI)

  // Eccentricity
  float eccentricity = 0.0f; // Circle-to-ellipse deformation (0.0-0.8)
  float skewAngle = 0.0f;    // Ellipse major axis angle (radians, -PI-PI)

  // Noise
  float noiseAmount = 0.15f; // Color variation within/between rings (0.0-0.5)
  float noiseScale = 5.0f;   // Noise spatial frequency (1.0-20.0)

  // FFT mapping
  float baseFreq = 55.0f;   // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f; // High frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend strength (0-5)
};

#define SPECTRAL_RINGS_CONFIG_FIELDS                                           \
  enabled, ringDensity, ringWidth, layers, pulseSpeed, colorShiftSpeed,        \
      rotationSpeed, eccentricity, skewAngle, noiseAmount, noiseScale,         \
      baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode,         \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpectralRingsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float pulseAccum;
  float colorShiftAccum;
  float rotationAccum;
  float time;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeLoc;
  int ringDensityLoc;
  int ringWidthLoc;
  int layersLoc;
  int pulseAccumLoc;
  int colorShiftAccumLoc;
  int rotationAccumLoc;
  int eccentricityLoc;
  int skewAngleLoc;
  int noiseAmountLoc;
  int noiseScaleLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SpectralRingsEffect;

// Returns true on success, false if shader fails to load
bool SpectralRingsEffectInit(SpectralRingsEffect *e,
                             const SpectralRingsConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SpectralRingsEffectSetup(SpectralRingsEffect *e,
                              const SpectralRingsConfig *cfg, float deltaTime,
                              Texture2D fftTexture);

// Unloads shader and frees LUT
void SpectralRingsEffectUninit(SpectralRingsEffect *e);

// Returns default config
SpectralRingsConfig SpectralRingsConfigDefault(void);

// Registers modulatable params with the modulation engine
void SpectralRingsRegisterParams(SpectralRingsConfig *cfg);

#endif // SPECTRAL_RINGS_H
