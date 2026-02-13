// Slashes effect module
// FFT-driven diagonal bar field — semitone-mapped bars with envelope decay,
// random scatter, thickness variation, and gradient-colored additive glow

#ifndef SLASHES_H
#define SLASHES_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SlashesConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f; // Lowest mapped frequency in Hz
  int numOctaves = 5;     // Octave count; total bars = octaves * 12
  float gain = 2.0f;      // FFT magnitude amplification
  float curve = 0.7f;     // Magnitude contrast shaping

  // Tick animation
  float tickRate = 4.0f;      // Re-roll rate (ticks/second)
  float envelopeSharp = 4.0f; // Envelope decay sharpness

  // Bar geometry
  float maxBarLength = 0.7f;       // Maximum bar half-length at full magnitude
  float barThickness = 0.005f;     // Bar half-thickness baseline
  float thicknessVariation = 0.5f; // Random thickness spread per bar
  float scatter = 0.5f;            // Position offset range from center
  float glowSoftness = 0.01f;      // Edge anti-aliasing width
  float baseBright = 0.15f;        // Minimum brightness for visible notes
  float rotationDepth = 0.0f; // 3D foreshortening (0=flat, 1=full 3D scatter)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct SlashesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float tickAccum; // CPU-accumulated tick counter
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int tickAccumLoc;
  int envelopeSharpLoc;
  int maxBarLengthLoc;
  int barThicknessLoc;
  int thicknessVariationLoc;
  int scatterLoc;
  int glowSoftnessLoc;
  int baseBrightLoc;
  int rotationDepthLoc;
  int gradientLUTLoc;
} SlashesEffect;

// Returns true on success, false if shader fails to load
bool SlashesEffectInit(SlashesEffect *e, const SlashesConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SlashesEffectSetup(SlashesEffect *e, const SlashesConfig *cfg,
                        float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void SlashesEffectUninit(SlashesEffect *e);

// Returns default config
SlashesConfig SlashesConfigDefault(void);

// Registers modulatable params with the modulation engine
void SlashesRegisterParams(SlashesConfig *cfg);

#endif // SLASHES_H
