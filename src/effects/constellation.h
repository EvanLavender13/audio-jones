// Constellation effect module
// Procedural star field with wandering points and distance-based connection
// lines

#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ConstellationConfig {
  bool enabled = false;

  // Grid and animation
  float gridScale = 21.0f; // Point density: cells across screen (5.0-50.0)
  float animSpeed = 1.0f;  // Wander animation speed multiplier (0.0-5.0)
  float wanderAmp = 0.4f;  // How far points drift from cell center (0.0-0.5)

  // Wave overlay
  float waveFreq = 1.0f;  // Ripple count across grid (0.1-5.0)
  float waveAmp = 2.0f;   // Coordination strength (0.0-4.0)
  float waveSpeed = 0.5f; // Ripple propagation speed (0.0-5.0)

  // Triangle fill
  bool fillEnabled = false;
  float fillOpacity = 0.3f;   // Triangle fill brightness (0.0-1.0)
  float fillThreshold = 2.5f; // Max perimeter for visible triangles (1.0-4.0)

  // Wave center
  float waveCenterX = 0.5f; // Wave origin X in UV space (-2.0 to 3.0)
  float waveCenterY = 0.5f; // Wave origin Y in UV space (-2.0 to 3.0)

  // Point rendering
  float pointSize =
      1.0f; // Glow size multiplier (0.3-3.0), higher = bigger glow
  float pointBrightness = 1.0f; // Point glow intensity (0.0-2.0)

  // Line rendering
  float lineThickness = 0.05f; // Width of connection lines (0.01-0.1)
  float maxLineLen = 1.5f;     // Lines longer than this fade out (0.5-2.0)
  float lineOpacity = 0.5f;    // Overall line brightness (0.0-1.0)

  // Color mode
  bool interpolateLineColor =
      false; // true: blend endpoint colors; false: sample LUT by length

  // Gradients (default to gradient mode with cyan-magenta)
  ColorConfig pointGradient = {.mode = COLOR_MODE_GRADIENT};
  ColorConfig lineGradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct ConstellationEffect {
  Shader shader;
  ColorLUT *pointLUT;
  ColorLUT *lineLUT;
  float animPhase;
  float wavePhase;
  int resolutionLoc;
  int gridScaleLoc;
  int wanderAmpLoc;
  int waveFreqLoc;
  int waveAmpLoc;
  int pointSizeLoc;
  int pointBrightnessLoc;
  int lineThicknessLoc;
  int maxLineLenLoc;
  int lineOpacityLoc;
  int interpolateLineColorLoc;
  int animPhaseLoc;
  int wavePhaseLoc;
  int pointLUTLoc;
  int lineLUTLoc;
  int fillEnabledLoc;
  int fillOpacityLoc;
  int fillThresholdLoc;
  int waveCenterLoc;
} ConstellationEffect;

// Returns true on success, false if shader fails to load
bool ConstellationEffectInit(ConstellationEffect *e,
                             const ConstellationConfig *cfg);

// Binds all uniforms, updates LUT textures, and advances time accumulators
void ConstellationEffectSetup(ConstellationEffect *e,
                              const ConstellationConfig *cfg, float deltaTime);

// Unloads shader and frees LUTs
void ConstellationEffectUninit(ConstellationEffect *e);

// Returns default config
ConstellationConfig ConstellationConfigDefault(void);

// Registers modulatable params with the modulation engine
void ConstellationRegisterParams(ConstellationConfig *cfg);

#endif // CONSTELLATION_H
