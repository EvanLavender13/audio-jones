// Star trail effect module
// Orbiting stars with persistent luminous trails and per-star FFT reactivity

#ifndef STAR_TRAIL_H
#define STAR_TRAIL_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct StarTrailConfig {
  bool enabled = false;

  // Stars
  int starCount = 500;       // Number of orbiting stars (50-1000)
  int freqMapMode = 0;       // FFT mapping: 0=radius, 1=hash, 2=angle (0-2)
  float spreadRadius = 1.0f; // How far stars spread from center (0.2-2.0)

  // Orbit
  float orbitSpeed = 0.4f;     // Base angular velocity multiplier (-2.0-2.0)
  float speedWaviness = 10.0f; // Radial speed ripple frequency (0.0-20.0)
  float speedVariation = 0.1f; // Radial speed ripple amplitude (0.0-0.5)

  // Appearance
  float glowIntensity = 0.9f; // Peak brightness of star sprites (0.1-3.0)
  float decayHalfLife = 2.0f; // Trail persistence in seconds (0.1-10.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Min brightness floor when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define STAR_TRAIL_CONFIG_FIELDS                                               \
  enabled, starCount, freqMapMode, spreadRadius, orbitSpeed, speedWaviness,    \
      speedVariation, glowIntensity, decayHalfLife, baseFreq, maxFreq, gain,   \
      curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct StarTrailEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float time;          // Accumulated orbit phase (orbitSpeed * sum(dt))
  float variationTime; // Accumulated variation phase (speedVariation * sum(dt))
  // Uniform locations
  int resolutionLoc;
  int timeLoc;
  int variationTimeLoc;
  int previousFrameLoc;
  int starCountLoc;
  int freqMapModeLoc;
  int spreadRadiusLoc;
  int speedWavinessLoc;
  int glowIntensityLoc;
  int decayFactorLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} StarTrailEffect;

// Loads shader, caches uniform locations, allocates ping-pong targets
bool StarTrailEffectInit(StarTrailEffect *e, const StarTrailConfig *cfg,
                         int width, int height);

// Binds uniforms and accumulates orbit/variation time
void StarTrailEffectSetup(StarTrailEffect *e, const StarTrailConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);

// Renders stars and trails to ping-pong targets
void StarTrailEffectRender(StarTrailEffect *e, const StarTrailConfig *cfg,
                           float deltaTime, int screenWidth, int screenHeight);

// Reallocates ping-pong targets at new dimensions
void StarTrailEffectResize(StarTrailEffect *e, int width, int height);

// Unloads shader, frees LUT and ping-pong targets
void StarTrailEffectUninit(StarTrailEffect *e);

// Registers modulatable params with the modulation engine
void StarTrailRegisterParams(StarTrailConfig *cfg);

#endif // STAR_TRAIL_H
