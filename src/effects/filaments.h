// Filaments effect module
// Tangled radial line segments driven by FFT semitone energy — rotating
// endpoint geometry, per-segment FFT warp, triangle-wave noise, additive glow

#ifndef FILAMENTS_H
#define FILAMENTS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct FilamentsConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;  // Lowest visible frequency in Hz
  float numOctaves = 5.0f; // Octave count (x12 = total filaments)
  float gain = 2.0f;       // FFT magnitude amplifier
  float curve = 0.7f;      // Contrast exponent on magnitude

  // Filament geometry (nimitz rotating-endpoint pattern)
  float radius = 0.8f;    // Endpoint distance from center (0.1-2.0)
  float spread = 0.0f;    // Angular fan per-filament index
  float stepAngle = 0.0f; // Cumulative rotation step between filaments

  // Glow
  float glowIntensity = 0.008f; // Peak filament brightness (0.001-0.05)
  float falloffExponent = 1.2f; // Distance falloff sharpness (0.8-2.0)
  float baseBright = 0.15f;     // Dim ember level for quiet semitones (0.0-1.0)

  // Triangle-noise displacement
  float noiseStrength = 0.4f; // Noise h-offset magnitude (0.0-1.0)
  float noiseSpeed = 4.5f;    // Noise rotation rate (0.0-10.0)

  // Animation
  float rotationSpeed = 1.5f; // Radial spin rate (rad/s)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct FilamentsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation angle
  float noiseTime;     // CPU-accumulated noise animation phase
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int radiusLoc;
  int spreadLoc;
  int stepAngleLoc;
  int glowIntensityLoc;
  int falloffExponentLoc;
  int baseBrightLoc;
  int noiseStrengthLoc;
  int noiseTimeLoc;
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
