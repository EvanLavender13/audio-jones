// Fireworks effect module
// Burst particles with gravity, drag, and trail persistence via ping-pong decay

#ifndef FIREWORKS_H
#define FIREWORKS_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct FireworksConfig {
  bool enabled = false;

  // Burst timing
  float burstRate = 1.5f;  // Bursts per second (0.0-5.0)
  int maxBursts = 3;       // Concurrent burst slots (1-8)
  int particles = 60;      // Particles per burst (16-120)
  float spreadArea = 0.5f; // Spawn distance from center (0.1-1.0)
  float yBias = 0.2f;      // Vertical offset of burst centers (-0.5-0.5)

  // Physics
  float burstRadius = 0.6f; // Max expansion distance (0.1-1.5)
  float gravity = 0.8f;     // Downward acceleration (0.0-2.0)
  float dragRate = 2.0f;    // Exponential deceleration (0.5-5.0)

  // Appearance
  float glowIntensity = 1.0f;  // Particle peak brightness (0.1-3.0)
  float particleSize = 0.008f; // Base glow radius (0.002-0.03)
  float glowSharpness = 1.7f;  // Glow falloff power (1.0-3.0)
  float sparkleSpeed = 20.0f;  // Sparkle oscillation freq (5.0-40.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT freq Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT freq Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.0f;       // FFT contrast curve (0.1-3.0)
  float baseBright = 0.1f;  // Min brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define FIREWORKS_CONFIG_FIELDS                                                \
  enabled, burstRate, maxBursts, particles, spreadArea, yBias, burstRadius,    \
      gravity, dragRate, glowIntensity, particleSize, glowSharpness,           \
      sparkleSpeed, baseFreq, maxFreq, gain, curve, baseBright, gradient,      \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct FireworksEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2]; // Trail persistence pair
  int readIdx;                 // Which pingPong to read from (0 or 1)
  float time;                  // Master time accumulator
  // Shader uniform locations
  int resolutionLoc;
  int previousFrameLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int burstRateLoc;
  int maxBurstsLoc;
  int particlesLoc;
  int spreadAreaLoc;
  int yBiasLoc;
  int burstRadiusLoc;
  int gravityLoc;
  int dragRateLoc;
  int glowIntensityLoc;
  int particleSizeLoc;
  int glowSharpnessLoc;
  int sparkleSpeedLoc;
  int decayFactorLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} FireworksEffect;

// Loads shader, caches uniform locations, allocates ping-pong textures
bool FireworksEffectInit(FireworksEffect *e, const FireworksConfig *cfg,
                         int width, int height);

// Binds scalar uniforms and accumulates time state
void FireworksEffectSetup(FireworksEffect *e, const FireworksConfig *cfg,
                          float deltaTime, int screenWidth, int screenHeight);

// Executes ping-pong render pass: spawns bursts + fades previous trails
void FireworksEffectRender(FireworksEffect *e, const FireworksConfig *cfg,
                           float deltaTime, int screenWidth, int screenHeight,
                           Texture2D fftTexture);

// Unloads ping-pong textures, reallocates at new dimensions
void FireworksEffectResize(FireworksEffect *e, int width, int height);

// Unloads shader, frees LUT and ping-pong textures
void FireworksEffectUninit(FireworksEffect *e);

// Returns default config
FireworksConfig FireworksConfigDefault(void);

// Registers modulatable params with the modulation engine
void FireworksRegisterParams(FireworksConfig *cfg);

#endif // FIREWORKS_H
