// Motherboard: Kali-family circuit fractals with three modes from the Circuits
// series. Mode 0: dot-product inversion, compound orbit trap, pow rendering.
// Mode 1: product inversion, stepping flow, exp rendering.
// Mode 2: 90deg fold rotation, box junctions, exp+smoothstep rendering.

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct MotherboardConfig {
  bool enabled = false;

  // Mode
  int mode = 0; // Fractal variant: 0=Circuits, 1=Circuits II, 2=Circuits III

  // Geometry
  int iterations = 10; // Fold depth; each iteration = one frequency band (4-16)
  float zoom = 2.0f;   // Scale factor (0.1-4.0)
  float clampLo = 0.15f;     // Inversion lower bound (0.0-1.0)
  float clampHi = 2.0f;      // Inversion upper bound (0.5-5.0)
  float foldConstant = 1.0f; // Post-inversion translation (0.5-2.0)
  float traceWidth = 0.005f; // Trace glow thickness (0.001-0.05)

  // Animation
  float panSpeed = 0.05f;     // Drift speed through fractal space (-2.0..2.0)
  float flowSpeed = 0.3f;     // Data streaming speed (0.0-2.0)
  float flowIntensity = 0.3f; // Streaming visibility (0.0-1.0)
  float rotationSpeed = 0.0f; // Pattern rotation rate, radians/second

  // Audio
  float baseFreq = 55.0f;   // Lowest frequency band Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest frequency band Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum trace brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define MOTHERBOARD_CONFIG_FIELDS                                              \
  enabled, mode, iterations, zoom, clampLo, clampHi, foldConstant, traceWidth, \
      panSpeed, flowSpeed, flowIntensity, rotationSpeed, baseFreq, maxFreq,    \
      gain, curve, baseBright, gradient, blendMode, blendIntensity

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
  int modeLoc;
  int baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc;
  int iterationsLoc, zoomLoc, clampLoLoc, clampHiLoc, foldConstantLoc,
      traceWidthLoc;
  int panAccumLoc, flowAccumLoc, flowIntensityLoc, rotationAccumLoc;
  int gradientLUTLoc;
} MotherboardEffect;

// Returns true on success, false if shader fails to load
bool MotherboardEffectInit(MotherboardEffect *e, const MotherboardConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void MotherboardEffectSetup(MotherboardEffect *e, const MotherboardConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void MotherboardEffectUninit(MotherboardEffect *e);

// Registers modulatable params with the modulation engine
void MotherboardRegisterParams(MotherboardConfig *cfg);

MotherboardEffect *GetMotherboardEffect(PostEffect *pe);

#endif // MOTHERBOARD_H
