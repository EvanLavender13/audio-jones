#ifndef CONSTELLATION_CONFIG_H
#define CONSTELLATION_CONFIG_H

#include "render/color_config.h"
#include <stdbool.h>

typedef struct ConstellationConfig {
  bool enabled = false;

  // Grid and animation
  float gridScale = 21.0f; // Point density: cells across screen (5.0-50.0)
  float animSpeed = 1.0f;  // Wander animation speed multiplier (0.0-5.0)
  float wanderAmp = 0.4f;  // How far points drift from cell center (0.0-0.5)

  // Radial wave overlay
  float radialFreq = 1.0f;  // Radial wave frequency (0.1-5.0)
  float radialAmp = 1.0f;   // Radial wave strength on positions (0.0-2.0)
  float radialSpeed = 0.5f; // Radial wave propagation speed (0.0-5.0)

  // Point rendering
  float glowScale = 100.0f;     // Inverse-squared glow falloff (50.0-500.0)
  float pointBrightness = 1.0f; // Point glow intensity (0.0-2.0)

  // Line rendering
  float lineThickness = 0.05f; // Width of connection lines (0.01-0.1)
  float maxLineLen = 1.5f;     // Lines longer than this fade out (0.5-2.0)
  float lineOpacity = 0.5f;    // Overall line brightness (0.0-1.0)

  // Color mode
  bool interpolateLineColor =
      false; // true: blend endpoint colors; false: sample LUT by length

  // Gradients
  ColorConfig pointGradient; // Color gradient for points (sampled by cell hash)
  ColorConfig lineGradient;  // Color gradient for lines (sampled by length or
                             // interpolated)

} ConstellationConfig;

#endif // CONSTELLATION_CONFIG_H
