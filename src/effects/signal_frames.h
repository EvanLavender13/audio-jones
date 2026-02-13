// Signal frames effect module
// FFT-driven concentric rounded-rectangle outlines with per-octave sizing,
// orbital motion, sweep glow, and gradient coloring

#ifndef SIGNAL_FRAMES_H
#define SIGNAL_FRAMES_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SignalFramesConfig {
  bool enabled = false;

  // FFT mapping
  float numOctaves = 5.0f;  // Octave count (1.0-8.0)
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Baseline brightness for inactive frames (0.0-1.0)

  // Animation
  float rotationSpeed = 0.5f; // Rotation rate (radians/second), CPU-accumulated
  float orbitRadius = 0.3f;   // Orbital offset from center (0.0-1.5)
  float orbitSpeed = 0.4f;    // Orbital revolution rate (0.0-3.0)

  // Frame geometry
  float sizeMin = 0.05f;          // Smallest frame half-extent (0.01-0.5)
  float sizeMax = 0.6f;           // Largest frame half-extent (0.1-1.5)
  float aspectRatio = 1.5f;       // Width-to-height ratio (0.2-5.0)
  float outlineThickness = 0.01f; // Stroke width in UV space (0.002-0.05)

  // Glow
  float glowWidth = 0.005f;     // Glow falloff distance (0.001-0.05)
  float glowIntensity = 2.0f;   // Glow brightness multiplier (0.5-10.0)
  float sweepSpeed = 0.5f;      // Sweep rotation rate (0.0-3.0)
  float sweepIntensity = 0.02f; // Sweep brightness boost (0.0-0.1)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct SignalFramesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation phase
  float sweepAccum;    // CPU-accumulated sweep phase
  float orbitAccum;    // CPU-accumulated orbit phase
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int numOctavesLoc;
  int baseFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int rotationAccumLoc;
  int orbitRadiusLoc;
  int orbitAccumLoc;
  int sizeMinLoc;
  int sizeMaxLoc;
  int aspectRatioLoc;
  int outlineThicknessLoc;
  int glowWidthLoc;
  int glowIntensityLoc;
  int sweepAccumLoc;
  int sweepIntensityLoc;
  int gradientLUTLoc;
} SignalFramesEffect;

// Returns true on success, false if shader fails to load
bool SignalFramesEffectInit(SignalFramesEffect *e,
                            const SignalFramesConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void SignalFramesEffectSetup(SignalFramesEffect *e,
                             const SignalFramesConfig *cfg, float deltaTime,
                             Texture2D fftTexture);

// Unloads shader and frees LUT
void SignalFramesEffectUninit(SignalFramesEffect *e);

// Returns default config
SignalFramesConfig SignalFramesConfigDefault(void);

// Registers modulatable params with the modulation engine
void SignalFramesRegisterParams(SignalFramesConfig *cfg);

#endif // SIGNAL_FRAMES_H
