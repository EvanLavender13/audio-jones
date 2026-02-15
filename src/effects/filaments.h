// Filaments effect module
// Tangled radial line segments driven by FFT energy — rotating
// endpoint geometry, per-segment FFT brightness, additive glow

#ifndef FILAMENTS_H
#define FILAMENTS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct FilamentsConfig {
  bool enabled = false;

  // Geometry
  int filaments = 60; // Number of filaments (4-256)

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier
  float curve = 0.7f;       // Contrast exponent on magnitude

  // Filament geometry (nimitz rotating-endpoint pattern)
  float radius = 0.8f;    // Endpoint distance from center (0.1-2.0)
  float spread = 0.0f;    // Angular fan per-filament index
  float stepAngle = 0.0f; // Cumulative rotation step between filaments

  // Glow
  float glowIntensity = 2.0f; // Brightness multiplier (0.5-10.0)
  float baseBright = 0.15f;   // Dim ember level for quiet semitones (0.0-1.0)

  // Animation
  float rotationSpeed = 1.5f; // Radial spin rate (rad/s)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define FILAMENTS_CONFIG_FIELDS                                                \
  enabled, filaments, baseFreq, maxFreq, gain, curve, radius, spread,          \
      stepAngle, glowIntensity, baseBright, rotationSpeed, gradient,           \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct FilamentsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int filamentsLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int radiusLoc;
  int spreadLoc;
  int stepAngleLoc;
  int glowIntensityLoc;
  int baseBrightLoc;
  int rotationAccumLoc;
  int gradientLUTLoc;
} FilamentsEffect;

// Returns true on success, false if shader fails to load
bool FilamentsEffectInit(FilamentsEffect *e, const FilamentsConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void FilamentsEffectSetup(FilamentsEffect *e, const FilamentsConfig *cfg,
                          float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void FilamentsEffectUninit(FilamentsEffect *e);

// Returns default config
FilamentsConfig FilamentsConfigDefault(void);

// Registers modulatable params with the modulation engine
void FilamentsRegisterParams(FilamentsConfig *cfg);

#endif // FILAMENTS_H
