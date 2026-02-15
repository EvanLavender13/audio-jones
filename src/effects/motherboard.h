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

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (A1) (27.5-440.0)
  int numOctaves = 5;       // Octave count (1-8)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Baseline brightness for silent traces (0.0-1.0)

  // Fold geometry
  int iterations = 12;      // Fold layers, 12 = one per semitone (4-16)
  float rangeX = 0.5f;      // Horizontal fold spacing (0.0-2.0)
  float rangeY = 0.2f;      // Vertical fold offset (0.0-1.0)
  float size = 0.1f;        // Trace y-threshold (0.0-1.0)
  float fallOff = 1.15f;    // Scale decay per iteration (1.0-3.0)
  float rotAngle = 0.7854f; // Rotation per fold, radians (0-PI), PI/4

  // Glow
  float glowIntensity = 0.01f;   // Trace glow numerator (0.001-0.1)
  float accentIntensity = 0.04f; // Fold-seam glow strength (0.0-0.2)

  // Animation
  float rotationSpeed = 0.0f; // Fold rotation rate, radians/second

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define MOTHERBOARD_CONFIG_FIELDS                                              \
  enabled, baseFreq, numOctaves, gain, curve, baseBright, iterations, rangeX,  \
      rangeY, size, fallOff, rotAngle, glowIntensity, accentIntensity,         \
      rotationSpeed, blendIntensity, gradient, blendMode

typedef struct ColorLUT ColorLUT;

typedef struct MotherboardEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;          // Accent glow animation accumulator
  float rotationAccum; // CPU-accumulated rotation angle
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int iterationsLoc;
  int rangeXLoc;
  int rangeYLoc;
  int sizeLoc;
  int fallOffLoc;
  int rotAngleLoc;
  int glowIntensityLoc;
  int accentIntensityLoc;
  int timeLoc;
  int rotationAccumLoc;
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
