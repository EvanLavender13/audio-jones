// Fireworks effect module
// Analytical ballistic firework bursts with per-burst FFT and gradient LUT

#ifndef FIREWORKS_H
#define FIREWORKS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct FireworksConfig {
  bool enabled = false;

  // Burst
  int maxBursts = 6;       // Concurrent firework slots (1-8)
  int particles = 40;      // Sparks per burst (10-60)
  float spreadArea = 0.5f; // Horizontal range of launch positions (0.1-1.0)
  float yBias = 0.0f;      // Vertical offset of burst region (-0.5-0.5)

  // Timing
  float rocketTime = 1.1f;  // Rising rocket phase duration (0.3-2.0)
  float explodeTime = 0.9f; // Explosion phase duration (0.3-2.0)
  float pauseTime = 0.5f;   // Gap between episodes per slot (0.0-2.0)

  // Physics
  float gravity = 9.8f;     // Downward acceleration (0.0-20.0)
  float burstSpeed = 10.0f; // Initial outward velocity of sparks (5.0-20.0)
  float rocketSpeed = 8.0f; // Upward velocity of rising rocket (2.0-12.0)

  // Appearance
  float glowIntensity = 1.0f; // Particle peak brightness (0.1-3.0)
  float particleSize = 0.05f; // Glow numerator in scaled UV space (0.01-0.1)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT freq Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT freq Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT contrast curve (0.1-3.0)
  float baseBright = 0.15f; // Min brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define FIREWORKS_CONFIG_FIELDS                                                \
  enabled, maxBursts, particles, spreadArea, yBias, rocketTime, explodeTime,   \
      pauseTime, gravity, burstSpeed, rocketSpeed, glowIntensity,              \
      particleSize, baseFreq, maxFreq, gain, curve, baseBright, gradient,      \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct FireworksEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D target; // Single render target
  float time;             // Master time accumulator
  // Shader uniform locations
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int maxBurstsLoc;
  int particlesLoc;
  int spreadAreaLoc;
  int yBiasLoc;
  int rocketTimeLoc;
  int explodeTimeLoc;
  int pauseTimeLoc;
  int gravityLoc;
  int burstSpeedLoc;
  int rocketSpeedLoc;
  int glowIntensityLoc;
  int particleSizeLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} FireworksEffect;

// Loads shader, caches uniform locations, allocates render target
bool FireworksEffectInit(FireworksEffect *e, const FireworksConfig *cfg,
                         int width, int height);

// Binds scalar uniforms and accumulates time state
void FireworksEffectSetup(FireworksEffect *e, const FireworksConfig *cfg,
                          float deltaTime, int screenWidth, int screenHeight);

// Renders fireworks to target
void FireworksEffectRender(const FireworksEffect *e, const FireworksConfig *cfg,
                           float deltaTime, int screenWidth, int screenHeight,
                           const Texture2D &fftTexture);

// Reallocates render target at new dimensions
void FireworksEffectResize(FireworksEffect *e, int width, int height);

// Unloads shader, frees LUT and render target
void FireworksEffectUninit(FireworksEffect *e);

// Registers modulatable params with the modulation engine
void FireworksRegisterParams(FireworksConfig *cfg);

FireworksEffect *GetFireworksEffect(PostEffect *pe);

#endif // FIREWORKS_H
