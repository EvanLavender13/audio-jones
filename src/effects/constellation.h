// Constellation effect module
// Procedural star field with wandering points and distance-based connection
// lines

#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include "raylib.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ConstellationConfig {
  bool enabled = false;

  // Grid and animation
  float gridScale = 21.0f; // Point density: cells across screen (5.0-50.0)
  float animSpeed = 1.0f;  // Wander animation speed multiplier (0.0-5.0)
  float wanderAmp = 0.4f;  // How far points drift from cell center (0.0-0.5)

  // Radial wave overlay
  float radialFreq = 1.0f;  // Ripple count across grid (0.1-5.0)
  float radialAmp = 2.0f;   // Coordination strength (0.0-4.0)
  float radialSpeed = 0.5f; // Ripple propagation speed (0.0-5.0)

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
};

typedef struct ColorLUT ColorLUT;

typedef struct ConstellationEffect {
  Shader shader;
  ColorLUT *pointLUT;
  ColorLUT *lineLUT;
  float animPhase;
  float radialPhase;
  int resolutionLoc;
  int gridScaleLoc;
  int wanderAmpLoc;
  int radialFreqLoc;
  int radialAmpLoc;
  int pointSizeLoc;
  int pointBrightnessLoc;
  int lineThicknessLoc;
  int maxLineLenLoc;
  int lineOpacityLoc;
  int interpolateLineColorLoc;
  int animPhaseLoc;
  int radialPhaseLoc;
  int pointLUTLoc;
  int lineLUTLoc;
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

#endif // CONSTELLATION_H
