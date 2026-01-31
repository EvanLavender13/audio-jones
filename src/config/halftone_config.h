#ifndef HALFTONE_CONFIG_H
#define HALFTONE_CONFIG_H

// Halftone: luminance-based dot pattern simulating print halftoning
struct HalftoneConfig {
  bool enabled = false;
  float dotScale = 8.0f;      // Grid cell size in pixels (2.0-20.0)
  float dotSize = 1.0f;       // Dot radius multiplier within cell (0.5-2.0)
  float rotationSpeed = 0.0f; // Rotation rate in radians/second
  float rotationAngle = 0.0f; // Static rotation offset in radians
};

#endif // HALFTONE_CONFIG_H
