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
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Ceiling frequency (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent on magnitude (0.1-3.0)

  // Ring layout
  int rings = 24;         // Visual density (4-96)
  float ringScale = 2.5f; // Ring spread factor (higher = tighter packing)
  float tilt = 0.0f;      // Perspective tilt amount (0 = flat, 1 = Cosmic tilt)
  float tiltAngle = 0.0f; // Tilt direction (radians)

  // Arc appearance
  float arcWidth = 0.6f;      // cos() clamp ceiling (arc visibility, 0.0-1.0)
  float glowIntensity = 0.2f; // Glow numerator (brightness at ring center)
  float glowFalloff = 40.0f;  // Denominator epsilon scale
  float baseBright = 0.15f;   // Baseline brightness for inactive arcs (0.0-1.0)

  // Animation
  float rotationSpeed = 1.0f; // Rotation rate (radians/second), CPU-accumulated

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SPECTRAL_ARCS_CONFIG_FIELDS                                            \
  enabled, baseFreq, maxFreq, gain, curve, rings, ringScale, tilt, tiltAngle,  \
      arcWidth, glowIntensity, glowFalloff, baseBright, rotationSpeed,         \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpectralArcsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int ringsLoc;
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
