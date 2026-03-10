// Triskelion generator — fractal tiled grid with iterated space-folding and
// concentric circle interference

#ifndef TRISKELION_H
#define TRISKELION_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct TriskelionConfig {
  bool enabled = false;

  // Geometry
  int foldMode = 1;         // Tiling type: 0=square, 1=hex
  int layers = 16;          // Iteration depth (4-32)
  float circleFreq = 5.0f;  // Concentric circle ring density (1.0-20.0)
  float colorCycles = 3.0f; // Radial color gradient ring count (0.1-10.0)

  // Animation
  float rotationSpeed = 0.25f; // Fold matrix rotation rate (rad/s, -PI..PI)
  float scaleSpeed = 0.15f;    // Scale oscillation rate (rad/s, -PI..PI)
  float scaleAmount = 0.1f;    // Scale deviation from 1.0 (0.01-0.3)
  float colorSpeed = 0.15f;    // Color cycling rate (rad/s, -PI..PI)
  float circleSpeed = 0.35f; // Circle interference phase rate (rad/s, -PI..PI)

  // Audio
  float baseFreq = 55.0f;   // FFT low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response power curve (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend strength (0-5)
};

#define TRISKELION_CONFIG_FIELDS                                               \
  enabled, foldMode, layers, circleFreq, colorCycles, rotationSpeed,           \
      scaleSpeed, scaleAmount, colorSpeed, circleSpeed, baseFreq, maxFreq,     \
      gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct TriskelionEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAngle; // accumulated rotation
  float scaleAngle;    // accumulated scale oscillation
  float colorPhase;    // accumulated color cycling
  float circlePhase;   // accumulated circle interference phase
  int resolutionLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int foldModeLoc;
  int layersLoc;
  int circleFreqLoc;
  int colorCyclesLoc;
  int rotationAngleLoc;
  int scaleAngleLoc;
  int scaleAmountLoc;
  int colorPhaseLoc;
  int circlePhaseLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} TriskelionEffect;

// Returns true on success, false if shader fails to load
bool TriskelionEffectInit(TriskelionEffect *e, const TriskelionConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void TriskelionEffectSetup(TriskelionEffect *e, TriskelionConfig *cfg,
                           float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void TriskelionEffectUninit(TriskelionEffect *e);

// Returns default config
TriskelionConfig TriskelionConfigDefault(void);

// Registers modulatable params with the modulation engine
void TriskelionRegisterParams(TriskelionConfig *cfg);

#endif // TRISKELION_H
