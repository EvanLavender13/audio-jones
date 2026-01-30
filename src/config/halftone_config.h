#ifndef HALFTONE_CONFIG_H
#define HALFTONE_CONFIG_H

// Halftone: CMYK dot-matrix transform simulating print halftoning
// Converts to CMYK, samples each channel on a rotated grid, renders
// anti-aliased dots sized by ink density
struct HalftoneConfig {
  bool enabled = false;
  float dotScale = 8.0f;      // Grid cell size in pixels (2.0-20.0)
  float dotSize = 1.0f;       // Dot radius multiplier within cell (0.5-2.0)
  float rotationSpeed = 0.0f; // Rotation rate in radians/frame
  float rotationAngle = 0.0f; // Static rotation offset in radians
  float threshold = 0.888f;   // Smoothstep center for dot edge (0.5-1.0)
  float softness = 0.288f; // Smoothstep width for edge antialiasing (0.1-0.5)
};

#endif // HALFTONE_CONFIG_H
