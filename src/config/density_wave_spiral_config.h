#ifndef DENSITY_WAVE_SPIRAL_CONFIG_H
#define DENSITY_WAVE_SPIRAL_CONFIG_H

#include <stdbool.h>

typedef struct DensityWaveSpiralConfig {
  bool enabled = false;
  float centerX = 0.0f; // Galaxy center offset X (-0.5 to 0.5)
  float centerY = 0.0f; // Galaxy center offset Y (-0.5 to 0.5)
  float aspectX = 0.5f; // Ellipse eccentricity X (0.1-1.0)
  float aspectY =
      0.3f; // Ellipse eccentricity Y (0.1-1.0), smaller = barred spiral
  float tightness =
      -1.5708f; // Arm winding in radians (-PI to PI), negative = trailing arms
  float rotationSpeed =
      0.5f; // Differential rotation rate (radians/second), CPU accumulated
  float globalRotationSpeed =
      0.0f; // Whole-spiral rotation rate (radians/second), CPU accumulated
  float thickness = 0.3f; // Ring edge softness (0.05-0.5)
  int ringCount = 30;     // Number of concentric rings (10-50)
  float falloff = 1.0f;   // Brightness decay exponent (0.5-2.0)
} DensityWaveSpiralConfig;

#endif // DENSITY_WAVE_SPIRAL_CONFIG_H
