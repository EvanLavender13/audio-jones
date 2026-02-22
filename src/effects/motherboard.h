// Motherboard effect module
// Iterative fold-and-glow circuit trace pattern driven by FFT semitone energy

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct MotherboardConfig {
  bool enabled = false;

  // Geometry
  int iterations = 12; // Fold depth; each iteration = one frequency band (4-16)
  float zoom = 2.0f;   // Scale factor before tiling (0.5-4.0)
  float clampLo = 0.15f;     // Inversion lower bound (0.01-1.0)
  float clampHi = 2.0f;      // Inversion upper bound (0.5-5.0)
  float foldConstant = 1.0f; // Post-inversion translation (0.5-2.0)
  float rotAngle = 0.0f;     // Per-iteration fold rotation, radians (-PI..PI)

  // Animation
  float panSpeed = 0.3f;      // Drift speed through fractal space (-2.0..2.0)
  float flowSpeed = 0.3f;     // Data streaming speed (0.0-2.0)
  float flowIntensity = 0.3f; // Streaming visibility (0.0-1.0)
  float rotationSpeed = 0.0f; // Pattern rotation rate, radians/second

  // Rendering
  float glowIntensity =
      0.033f; // Trace glow width: exp sharpness = 1/glowIntensity (0.001-0.1)
  float accentIntensity = 0.033f; // Junction glow width: exp sharpness =
                                  // 1/accentIntensity (0.0-0.1)

  // Audio
  float baseFreq = 55.0f;   // Lowest frequency band Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest frequency band Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define MOTHERBOARD_CONFIG_FIELDS                                              \
  enabled, iterations, zoom, clampLo, clampHi, foldConstant, rotAngle,         \
      panSpeed, flowSpeed, flowIntensity, rotationSpeed, glowIntensity,        \
      accentIntensity, baseFreq, maxFreq, gain, curve, baseBright, gradient,   \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct MotherboardEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float panAccum;      // CPU-accumulated pan offset
  float flowAccum;     // CPU-accumulated flow phase
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc;
  int iterationsLoc, zoomLoc, clampLoLoc, clampHiLoc, foldConstantLoc,
      rotAngleLoc;
  int panAccumLoc, flowAccumLoc, flowIntensityLoc, rotationAccumLoc;
  int glowIntensityLoc, accentIntensityLoc;
  int gradientLUTLoc;
} MotherboardEffect;

// Returns true on success, false if shader fails to load
bool MotherboardEffectInit(MotherboardEffect *e, const MotherboardConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void MotherboardEffectSetup(MotherboardEffect *e, const MotherboardConfig *cfg,
                            float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void MotherboardEffectUninit(MotherboardEffect *e);

// Returns default config
MotherboardConfig MotherboardConfigDefault(void);

// Registers modulatable params with the modulation engine
void MotherboardRegisterParams(MotherboardConfig *cfg);

#endif // MOTHERBOARD_H
