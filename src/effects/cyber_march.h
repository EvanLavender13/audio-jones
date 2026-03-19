// Cyber March effect module
// Cyber March: raymarched folded-space fractal with FFT-reactive coloring

#ifndef CYBER_MARCH_H
#define CYBER_MARCH_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct CyberMarchConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 60;        // Raymarch iterations (30-100)
  float stepSize = 0.5f;      // March step multiplier (0.1-1.0)
  float fov = 0.3f;           // Ray direction Z component (0.2-1.0)
  float domainSize = 30.0f;   // Domain repetition half-period (10.0-60.0)
  int foldIterations = 8;     // Fold-scale-translate iterations (2-12)
  float foldScale = 1.4f;     // Base scale factor per fold (1.0-2.0)
  float morphAmount = 0.1f;   // Fold scale oscillation amplitude (0.0-0.5)
  float foldOffsetX = 15.0f;  // Fold translate X (1.0-200.0)
  float foldOffsetY = 120.0f; // Fold translate Y (1.0-200.0)
  float foldOffsetZ = 40.0f;  // Fold translate Z (1.0-200.0)
  float initialScale = 3.0f;  // Starting scale accumulator (1.0-10.0)
  float colorSpread = 0.1f;   // March distance to gradient spread (0.01-2.0)
  float tonemapGain = 0.001f; // Tonemap exposure (0.0001-0.003)

  // Speed (CPU-accumulated)
  float flySpeed = 1.0f;   // Forward camera speed (0.0-20.0)
  float morphSpeed = 0.1f; // Fold scale oscillation rate (0.0-2.0)

  // Camera
  DualLissajousConfig lissajous = {
      .amplitude = 0.5f, .motionSpeed = 1.0f, .freqX1 = 0.3f, .freqY1 = 0.2f};

  // Audio
  bool colorFreqMap = false; // Map FFT bands to gradient color instead of depth
  float baseFreq = 55.0f;    // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f;  // High frequency bound Hz (1000-16000)
  float gain = 2.0f;         // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;        // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f;  // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define CYBER_MARCH_CONFIG_FIELDS                                              \
  enabled, marchSteps, stepSize, fov, domainSize, foldIterations, foldScale,   \
      morphAmount, foldOffsetX, foldOffsetY, foldOffsetZ, initialScale,        \
      colorSpread, tonemapGain, flySpeed, morphSpeed, lissajous, colorFreqMap, \
      baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode,         \
      blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct CyberMarchEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;
  float morphPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int flyPhaseLoc;
  int morphPhaseLoc;
  int marchStepsLoc;
  int stepSizeLoc;
  int fovLoc;
  int domainSizeLoc;
  int foldIterationsLoc;
  int foldScaleLoc;
  int morphAmountLoc;
  int foldOffsetLoc;
  int initialScaleLoc;
  int colorSpreadLoc;
  int tonemapGainLoc;
  int yawLoc;
  int pitchLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int colorFreqMapLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} CyberMarchEffect;

// Returns true on success, false if shader fails to load
bool CyberMarchEffectInit(CyberMarchEffect *e, const CyberMarchConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: DualLissajousUpdate mutates internal phase state
void CyberMarchEffectSetup(CyberMarchEffect *e, CyberMarchConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void CyberMarchEffectUninit(CyberMarchEffect *e);

// Registers modulatable params with the modulation engine
void CyberMarchRegisterParams(CyberMarchConfig *cfg);

#endif // CYBER_MARCH_H
