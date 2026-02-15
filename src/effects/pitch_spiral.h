// Pitch spiral effect module
// Maps FFT bins onto a logarithmic spiral — one full turn per octave —
// with pitch-class coloring via gradient LUT

#ifndef PITCH_SPIRAL_H
#define PITCH_SPIRAL_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PitchSpiralConfig {
  bool enabled = false;

  int numOctaves = 5;          // Octave count (1-8)
  float baseFreq = 55.0f;      // Lowest visible frequency (Hz) (27.5-440.0)
  int numTurns = 8;            // Number of spiral rings (octaves visible)
  float spiralSpacing = 0.05f; // Distance between adjacent rings
  float lineWidth = 0.02f;     // Spiral line thickness
  float blur = 0.02f;          // Anti-aliasing softness (smoothstep width)
  float gain = 2.0f;           // FFT magnitude amplifier (0.1-10.0)
  float curve = 0.7f;          // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Baseline brightness for inactive arcs (0.0-1.0)

  // Perspective tilt
  float tilt = 0.0f;      // Tilt amount (0 = flat, 1 = Cosmic tilt)
  float tiltAngle = 0.0f; // Tilt direction (radians)

  // Animation
  float rotationSpeed = 0.0f; // Spin rate (rad/s), positive = CCW
  float breathSpeed = 1.0f;   // Breathing oscillation rate (rad/s)
  float breathDepth =
      0.0f; // Radial expansion amplitude (fraction). 0 = disabled
  float shapeExponent = 1.0f; // Power-law curvature. 1.0 = Archimedean

  // Color (pitch-class coloring via LUT)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define PITCH_SPIRAL_CONFIG_FIELDS                                             \
  enabled, numOctaves, baseFreq, numTurns, spiralSpacing, lineWidth, blur,     \
      gain, curve, baseBright, tilt, tiltAngle, gradient, blendMode,           \
      blendIntensity, rotationSpeed, breathSpeed, breathDepth, shapeExponent

typedef struct ColorLUT ColorLUT;

typedef struct PitchSpiralEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum; // CPU-accumulated rotation phase
  float breathAccum;   // CPU-accumulated breathing phase
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numTurnsLoc;
  int spiralSpacingLoc;
  int lineWidthLoc;
  int blurLoc;
  int gainLoc;
  int curveLoc;
  int numOctavesLoc;
  int baseBrightLoc;
  int tiltLoc;
  int tiltAngleLoc;
  int gradientLUTLoc;
  int rotationAccumLoc;
  int breathAccumLoc;
  int breathDepthLoc;
  int shapeExponentLoc;
} PitchSpiralEffect;

// Returns true on success, false if shader fails to load
bool PitchSpiralEffectInit(PitchSpiralEffect *e, const PitchSpiralConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void PitchSpiralEffectSetup(PitchSpiralEffect *e, const PitchSpiralConfig *cfg,
                            float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void PitchSpiralEffectUninit(PitchSpiralEffect *e);

// Returns default config
PitchSpiralConfig PitchSpiralConfigDefault(void);

// Registers modulatable params with the modulation engine
void PitchSpiralRegisterParams(PitchSpiralConfig *cfg);

#endif // PITCH_SPIRAL_H
