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

  float baseFreq = 220.0f;     // Lowest visible frequency (Hz) — A3
  int numTurns = 8;            // Number of spiral rings (octaves visible)
  float spiralSpacing = 0.05f; // Distance between adjacent rings
  float lineWidth = 0.02f;     // Spiral line thickness
  float blur = 0.02f;          // Anti-aliasing softness (smoothstep width)
  float gain = 5.0f;           // FFT magnitude amplifier before contrast curve
  float curve = 2.0f; // Magnitude contrast exponent — pow(mag, curve)

  // Perspective tilt
  float tilt = 0.0f;      // Tilt amount (0 = flat, 1 = Cosmic tilt)
  float tiltAngle = 0.0f; // Tilt direction (radians)

  // Animation
  float rotationSpeed = 0.0f; // Spin rate (rad/s), positive = CCW
  float breathRate = 1.0f;    // Breathing oscillation frequency (Hz)
  float breathDepth =
      0.0f; // Radial expansion amplitude (fraction). 0 = disabled
  float shapeExponent = 1.0f; // Power-law curvature. 1.0 = Archimedean

  // Color (pitch-class coloring via LUT)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

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
