// Orrery effect module
// Hierarchical ring tree with FFT-driven brightness and connecting lines

#ifndef ORRERY_H
#define ORRERY_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct OrreryConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.15f; // Baseline brightness when audio silent (0.0-1.0)

  // Tree structure
  int branches = 3;             // Children per ring at each level (2-5)
  int depth = 3;                // Hierarchy levels, root = level 0 (2-4)
  float rootRadius = 0.5f;      // Root ring radius (0.1-0.8)
  float radiusDecay = 0.6f;     // Per-level shrinkage: 0=none, 1=max (0.0-0.9)
  float radiusVariation = 0.0f; // Per-ring random radius offset (0.0-1.0)

  // Animation
  float baseSpeed = 0.2f;      // Level 1 orbital speed, rad/s (-PI-PI)
  float speedScale = 2.0f;     // Speed multiplier per depth level (1.0-4.0)
  float speedVariation = 0.0f; // Per-ring random speed offset (0.0-1.0)

  // Stroke
  float strokeWidth = 0.005f; // Base ring thickness (0.001-0.02)
  float strokeTaper =
      0.5f; // Depth thinning: 0=uniform, 1=leaves thinnest (0.0-1.0)

  // Lines
  int lineMode = 0;            // 0=Seeded, 1=Siblings
  float lineSeed = 0.0f;       // Random seed for line pairs (0.0-100.0)
  int lineCount = 7;           // Number of connecting lines (0-20)
  float lineBrightness = 0.5f; // Line opacity, 0=off (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define ORRERY_CONFIG_FIELDS                                                   \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, branches, depth,        \
      rootRadius, radiusDecay, radiusVariation, baseSpeed, speedScale,         \
      speedVariation, strokeWidth, strokeTaper, lineMode, lineSeed, lineCount, \
      lineBrightness, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct OrreryEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float levelPhase[5];
  float variationPhase[5];
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int levelPhaseLoc;
  int variationPhaseLoc;
  int branchesLoc;
  int depthLoc;
  int rootRadiusLoc;
  int radiusDecayLoc;
  int radiusVariationLoc;
  int strokeWidthLoc;
  int strokeTaperLoc;
  int lineModeLoc;
  int lineSeedLoc;
  int lineCountLoc;
  int lineBrightnessLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} OrreryEffect;

// Returns true on success, false if shader fails to load
bool OrreryEffectInit(OrreryEffect *e, const OrreryConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void OrreryEffectSetup(OrreryEffect *e, const OrreryConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void OrreryEffectUninit(OrreryEffect *e);

// Registers modulatable params with the modulation engine
void OrreryRegisterParams(OrreryConfig *cfg);

OrreryEffect *GetOrreryEffect(PostEffect *pe);

#endif // ORRERY_H
