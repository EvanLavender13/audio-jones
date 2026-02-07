// Spectral arcs effect module
// Cosmic-style tilted concentric ring arcs driven by FFT semitone energy —
// perspective tilt, cos() multi-arc clipping, per-ring rotation,
// inverse-distance glow

#ifndef SPECTRAL_ARCS_H
#define SPECTRAL_ARCS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SpectralArcsConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 220.0f; // Lowest visible frequency in Hz (A3)
  int numOctaves = 8;      // Octave count (x12 = total rings)
  float gain = 5.0f;       // FFT magnitude amplifier
  float curve = 2.0f;      // Contrast exponent on magnitude

  // Ring layout
  float ringScale = 2.5f; // Ring spread factor (higher = tighter packing)
  float tilt = 0.0f;      // Perspective tilt amount (0 = flat, 1 = Cosmic tilt)
  float tiltAngle = 0.0f; // Tilt direction (radians)

  // Arc appearance
  float arcWidth = 0.6f;      // cos() clamp ceiling (arc visibility, 0.0-1.0)
  float glowIntensity = 0.2f; // Glow numerator (brightness at ring center)
  float glowFalloff = 40.0f;  // Denominator epsilon scale
  float baseBright = 0.1f;    // Baseline brightness for inactive arcs (0.0-1.0)

  // Animation
  float rotationSpeed = 1.0f; // Rotation rate (radians/second), CPU-accumulated

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct SpectralArcsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int ringScaleLoc;
  int tiltLoc;
  int tiltAngleLoc;
  int arcWidthLoc;
  int glowIntensityLoc;
  int glowFalloffLoc;
  int baseBrightLoc;
  int rotationAccumLoc;
  int gradientLUTLoc;
} SpectralArcsEffect;

// Returns true on success, false if shader fails to load
bool SpectralArcsEffectInit(SpectralArcsEffect *e,
                            const SpectralArcsConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SpectralArcsEffectSetup(SpectralArcsEffect *e,
                             const SpectralArcsConfig *cfg, float deltaTime,
                             Texture2D fftTexture);

// Unloads shader and frees LUT
void SpectralArcsEffectUninit(SpectralArcsEffect *e);

// Returns default config
SpectralArcsConfig SpectralArcsConfigDefault(void);

// Registers modulatable params with the modulation engine
void SpectralArcsRegisterParams(SpectralArcsConfig *cfg);

#endif // SPECTRAL_ARCS_H
