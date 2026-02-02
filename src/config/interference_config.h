#ifndef INTERFERENCE_CONFIG_H
#define INTERFERENCE_CONFIG_H

#include "config/dual_lissajous_config.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct InterferenceConfig {
  bool enabled = false;

  // Sources
  int sourceCount = 3;           // Number of wave emitters (1-8)
  float baseRadius = 0.4f;       // Distance of sources from center (0.0-1.0)
  float patternAngle = 0.0f;     // Rotation of source arrangement (radians)
  DualLissajousConfig lissajous; // Source motion pattern

  // Wave properties
  float waveFreq = 30.0f; // Ripple density (5.0-100.0)
  float waveSpeed = 2.0f; // Animation speed (0.0-10.0)

  // Falloff
  int falloffType = 3;          // 0=None, 1=Inverse, 2=InvSquare, 3=Gaussian
  float falloffStrength = 1.0f; // Attenuation rate (0.0-5.0)

  // Boundaries (mirror sources at screen edges)
  bool boundaries = false;
  float reflectionGain = 0.5f; // Mirror source attenuation (0.0-1.0)

  // Visualization
  int visualMode = 0;      // 0=Raw, 1=Absolute, 2=Contour
  int contourCount = 8;    // Band count for contour mode (2-20)
  float visualGain = 1.5f; // Output intensity (0.5-5.0)

  // Color
  int colorMode = 0; // 0=Intensity, 1=Chromatic
  float chromaSpread =
      0.03f; // RGB wavelength spread for Chromatic mode (0.0-0.1)
  ColorConfig color = {.mode = COLOR_MODE_GRADIENT};

} InterferenceConfig;

#endif // INTERFERENCE_CONFIG_H
