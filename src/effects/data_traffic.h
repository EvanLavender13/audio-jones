// Data traffic effect module
// Scrolling lane grid of colored cells with FFT-driven brightness and
// randomized widths

#ifndef DATA_TRAFFIC_H
#define DATA_TRAFFIC_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct DataTrafficConfig {
  bool enabled = false;

  // Geometry
  int lanes = 20;          // Number of horizontal lanes (4-60)
  float cellWidth = 0.08f; // Base cell width before random variation (0.01-0.3)
  float spacing = 3.0f;    // Cell spacing multiplier (1.5-6.0)
  float gapSize = 0.08f;   // Dark gap between lanes (0.02-0.3)
  float scrollAngle = 0.0f; // Lane direction angle in radians (-PI..PI)

  // Animation
  float scrollSpeed = 0.8f;    // Global scroll speed multiplier (0.0-3.0)
  float widthVariation = 0.6f; // Cell width randomization amount (0.0-1.0)
  float colorMix =
      0.5f; // Fraction of cells colored/reactive vs grayscale (0.0-1.0)
  float jitter = 0.3f;      // Gentle positional jitter amplitude (0.0-1.0)
  float changeRate = 0.15f; // How often widths/speeds re-randomize (0.05-0.5)
  float sparkIntensity =
      0.7f; // Brightness of sparks between close cells (0.0-2.0)

  // Behaviors
  float breathProb = 0.0f;    // Fraction of lanes that breathe (0.0-1.0)
  float breathRate = 0.25f;   // Breathing oscillation speed (0.05-2.0)
  float glowIntensity = 0.0f; // Proximity glow brightness (0.0-1.0)
  float glowRadius = 2.5f;    // Glow reach multiplier on cellWidth (0.5-5.0)

  // Audio
  float baseFreq = 55.0f;   // FFT low frequency bound Hz (27.5-440.0)
  float maxFreq = 14000.0f; // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplification (0.1-10.0)
  float curve = 1.0f;       // FFT magnitude contrast curve (0.1-3.0)
  float baseBright = 0.05f; // Minimum brightness for reactive cells (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DATA_TRAFFIC_CONFIG_FIELDS                                             \
  enabled, lanes, cellWidth, spacing, gapSize, scrollAngle, scrollSpeed,       \
      widthVariation, colorMix, jitter, changeRate, sparkIntensity,            \
      breathProb, breathRate, glowIntensity, glowRadius, baseFreq, maxFreq,    \
      gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DataTrafficEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // Animation time accumulator
  int resolutionLoc;
  int timeLoc;
  int lanesLoc;
  int cellWidthLoc;
  int spacingLoc;
  int gapSizeLoc;
  int scrollAngleLoc;
  int scrollSpeedLoc;
  int widthVariationLoc;
  int colorMixLoc;
  int jitterLoc;
  int changeRateLoc;
  int sparkIntensityLoc;
  int breathProbLoc;
  float breathPhase; // Accumulated breath oscillation phase
  int breathPhaseLoc;
  int glowIntensityLoc;
  int glowRadiusLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DataTrafficEffect;

// Returns true on success, false if shader fails to load
bool DataTrafficEffectInit(DataTrafficEffect *e, const DataTrafficConfig *cfg);

// Binds all uniforms, advances time accumulator, updates LUT texture
void DataTrafficEffectSetup(DataTrafficEffect *e, const DataTrafficConfig *cfg,
                            float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void DataTrafficEffectUninit(DataTrafficEffect *e);

// Returns default config
DataTrafficConfig DataTrafficConfigDefault(void);

// Registers modulatable params with the modulation engine
void DataTrafficRegisterParams(DataTrafficConfig *cfg);

#endif // DATA_TRAFFIC_H
