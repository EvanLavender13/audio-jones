// Light Medley effect module
// Light Medley: fly-through raymarcher rendering a warped crystalline lattice
// with FFT-reactive glow

#ifndef LIGHT_MEDLEY_H
#define LIGHT_MEDLEY_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct LightMedleyConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped pitch in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest mapped pitch in Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on FFT magnitudes (0.1-3.0)
  float baseBright = 0.15f; // Minimum glow when no audio present (0.0-1.0)

  // Geometry
  float swirlAmount = 3.0f; // Coordinate displacement amplitude (0.0-10.0)
  float swirlRate = 0.3f;   // Spatial frequency of swirl displacement (0.1-2.0)
  float swirlTimeRate = 0.1f; // How fast swirl pattern evolves (0.01-0.5)
  float twistRate = 0.1f;     // XY rotation per unit Z depth (-PI..PI)
  float flySpeed = 1.0f;      // Forward motion speed through lattice (0.1-3.0)
  float slabFreq = 1.0f;      // Frequency of cosine slab planes (0.5-4.0)
  float latticeScale = 1.0f;  // Cell density of octahedral lattice (0.5-4.0)

  // Camera
  DualLissajousConfig lissajous = {
      .amplitude = 0.8f, .motionSpeed = 1.0f, .freqX1 = 0.6f, .freqY1 = 0.3f};

  // Output
  float glowIntensity = 1.0f; // Glow brightness scale (0.1-5.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define LIGHT_MEDLEY_CONFIG_FIELDS                                             \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, swirlAmount, swirlRate, \
      swirlTimeRate, twistRate, flySpeed, slabFreq, latticeScale, lissajous,   \
      glowIntensity, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct LightMedleyEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float swirlPhase;
  float flyPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int flyPhaseLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int swirlAmountLoc;
  int swirlRateLoc;
  int swirlPhaseLoc;
  int twistRateLoc;
  int slabFreqLoc;
  int latticeScaleLoc;
  int panLoc;
  int glowIntensityLoc;
  int gradientLUTLoc;
} LightMedleyEffect;

// Returns true on success, false if shader fails to load
bool LightMedleyEffectInit(LightMedleyEffect *e, const LightMedleyConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: DualLissajousUpdate mutates internal phase state
void LightMedleyEffectSetup(LightMedleyEffect *e, LightMedleyConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void LightMedleyEffectUninit(LightMedleyEffect *e);

// Registers modulatable params with the modulation engine
void LightMedleyRegisterParams(LightMedleyConfig *cfg);

#endif // LIGHT_MEDLEY_H
