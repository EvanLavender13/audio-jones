// Nebula effect module
// FFT-driven procedural nebula clouds with fractal layers, semitone-reactive
// stars, sinusoidal drift, and gradient coloring

#ifndef NEBULA_H
#define NEBULA_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct NebulaConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest mapped pitch in Hz (27.5-440.0)
  int numOctaves = 5;       // Semitone range for star mapping (1-8)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent on FFT magnitudes (0.1-3.0)
  float baseBright = 0.15f; // Star glow when semitone is silent (0.0-1.0)

  // Nebula layers
  float driftSpeed = 1.0f; // Time accumulation rate (0.01-5.0)
  float frontScale = 4.0f; // UV divisor for foreground layer (1.0-8.0)
  float midScale = 3.0f;   // UV divisor for mid layer (1.0-10.0)
  float backScale = 4.0f;  // UV divisor for background layer (2.0-12.0)
  int frontIter = 26;      // Fractal iterations front (6-40)
  int midIter = 20;        // Fractal iterations mid (6-40)
  int backIter = 18;       // Fractal iterations back (6-40)

  // Stars
  float starDensity =
      400.0f; // Grid resolution for star placement (100.0-800.0)
  float starSharpness = 35.0f; // Hash power exponent (10.0-60.0)
  float glowWidth = 0.25f;     // Gaussian sigma in cell space (0.05-0.3)
  float glowIntensity = 2.0f;  // Star glow brightness multiplier (0.5-10.0)

  // Noise
  int noiseType = 0;   // 0 = kaliset, 1 = domain-warped FBM
  int fbmFrontOct = 5; // FBM octaves front (2-8)
  int fbmMidOct = 4;   // FBM octaves mid (2-8)
  int fbmBackOct = 3;  // FBM octaves back (2-8)

  // Dust lanes
  float dustScale = 3.5f; // FBM frequency for dust lanes (1.0-8.0)
  float dustStrength =
      0.4f;              // Opacity of dark absorption (0.0-1.0) — 0 disables
  float dustEdge = 0.1f; // Smoothstep width for lane boundaries (0.05-0.3)

  // Diffraction spikes
  float spikeIntensity = 0.5f;  // Diffraction cross brightness (0.0-2.0)
  float spikeSharpness = 20.0f; // Exponent for spike thinness (5.0-40.0)

  // Output
  float brightness = 1.0f; // Overall multiplier (0.5-3.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define NEBULA_CONFIG_FIELDS                                                   \
  enabled, baseFreq, numOctaves, gain, curve, baseBright, driftSpeed,          \
      frontScale, midScale, backScale, frontIter, midIter, backIter,           \
      starDensity, starSharpness, glowWidth, glowIntensity, noiseType,         \
      fbmFrontOct, fbmMidOct, fbmBackOct, dustScale, dustStrength, dustEdge,   \
      spikeIntensity, spikeSharpness, brightness, gradient, blendMode,         \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct NebulaEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // Master time accumulator for drift
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;

  int frontScaleLoc;
  int midScaleLoc;
  int backScaleLoc;
  int frontIterLoc;
  int midIterLoc;
  int backIterLoc;
  int starDensityLoc;
  int starSharpnessLoc;
  int glowWidthLoc;
  int glowIntensityLoc;
  int noiseTypeLoc;
  int dustScaleLoc;
  int dustStrengthLoc;
  int dustEdgeLoc;
  int spikeIntensityLoc;
  int spikeSharpnessLoc;
  int brightnessLoc;
  int gradientLUTLoc;
} NebulaEffect;

// Returns true on success, false if shader fails to load
bool NebulaEffectInit(NebulaEffect *e, const NebulaConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void NebulaEffectSetup(NebulaEffect *e, const NebulaConfig *cfg,
                       float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void NebulaEffectUninit(NebulaEffect *e);

// Returns default config
NebulaConfig NebulaConfigDefault(void);

// Registers modulatable params with the modulation engine
void NebulaRegisterParams(NebulaConfig *cfg);

#endif // NEBULA_H
