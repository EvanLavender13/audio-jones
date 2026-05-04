// Jellyfish effect module
// FFT-driven swarm of drifting bell silhouettes with pulsing rims, wavy
// tentacles, marine snow, and caustic backdrop

#ifndef JELLYFISH_H
#define JELLYFISH_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct JellyfishConfig {
  bool enabled = false;

  // FFT mapping (per-jellyfish log-spaced band)
  float baseFreq = 55.0f;   // Lowest mapped pitch in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest mapped pitch in Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on FFT magnitudes (0.1-3.0)
  float baseBright = 0.15f; // Base brightness when band is silent (0.0-1.0)

  // Swarm
  int jellyCount = 8; // Number of jellyfish (1-16)

  // Bell shape and pulse
  float sizeBase = 0.052f;     // Mean bell radius (0.02-0.10)
  float sizeVariance = 0.026f; // Per-jellyfish size jitter (0.0-0.06)
  float pulseDepth = 0.14f;    // Bell pulse modulation depth (0.0-0.4)
  float pulseSpeed = 1.0f;     // Bell pulse rate (0.1-3.0)

  // Drift path
  float driftAmp = 1.0f;   // Scales seeded jellyPath amplitudes (0.0-3.0)
  float driftSpeed = 1.0f; // Scales seeded jellyPath frequencies (0.0-3.0)

  // Tentacles
  float tentacleWave = 0.007f; // Tentacle lateral wave amplitude (0.0-0.025)

  // Glow stack
  float bellGlow = 0.40f;     // Inner bell halo strength (0.0-1.0)
  float rimGlow = 0.45f;      // Bell rim glow strength (0.0-1.0)
  float tentacleGlow = 0.55f; // Tentacle glow strength (0.0-1.0)

  // Marine snow
  float snowDensity = 0.5f;     // Snow particle density (0.0-1.0)
  float snowBrightness = 0.14f; // Snow particle brightness (0.0-0.4)

  // Caustics
  float causticIntensity = 0.26f; // Caustic light pattern strength (0.0-0.6)

  // Backdrop
  float backdropDepth = 1.0f; // Depth gradient strength multiplier (0.0-2.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define JELLYFISH_CONFIG_FIELDS                                                \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, jellyCount, sizeBase,   \
      sizeVariance, pulseDepth, pulseSpeed, driftAmp, driftSpeed,              \
      tentacleWave, bellGlow, rimGlow, tentacleGlow, snowDensity,              \
      snowBrightness, causticIntensity, backdropDepth, gradient, blendMode,    \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct JellyfishEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;

  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;

  int jellyCountLoc;
  int sizeBaseLoc;
  int sizeVarianceLoc;
  int pulseDepthLoc;
  int pulseSpeedLoc;
  int driftAmpLoc;
  int driftSpeedLoc;
  int tentacleWaveLoc;
  int bellGlowLoc;
  int rimGlowLoc;
  int tentacleGlowLoc;
  int snowDensityLoc;
  int snowBrightnessLoc;
  int causticIntensityLoc;
  int backdropDepthLoc;
} JellyfishEffect;

// Returns true on success, false if shader fails to load
bool JellyfishEffectInit(JellyfishEffect *e, const JellyfishConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void JellyfishEffectSetup(JellyfishEffect *e, const JellyfishConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void JellyfishEffectUninit(JellyfishEffect *e);

// Registers modulatable params with the modulation engine
void JellyfishRegisterParams(JellyfishConfig *cfg);

JellyfishEffect *GetJellyfishEffect(struct PostEffect *pe);

#endif // JELLYFISH_H
