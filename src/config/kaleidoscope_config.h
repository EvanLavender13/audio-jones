#ifndef KALEIDOSCOPE_CONFIG_H
#define KALEIDOSCOPE_CONFIG_H

// Kaleidoscope (Polar): Wedge-based radial mirroring with optional smooth edges
struct KaleidoscopeConfig {
  bool enabled = false;
  int segments = 6;           // Mirror segments / wedge count (1-12)
  float rotationSpeed = 0.0f; // Rotation rate (radians/second)
  float twistAngle = 0.0f;    // Radial twist offset (radians)
  float smoothing = 0.0f; // Blend width at wedge seams (0.0-0.5, 0 = hard edge)

  // Deprecated: kept for backward compatibility until Phase 3/4/6 migrate usage
  float polarIntensity = 0.0f;
  float kifsIntensity = 0.0f;
  float hexFoldIntensity = 0.0f;
  int kifsIterations = 4;
  float kifsScale = 2.0f;
  float kifsOffsetX = 1.0f;
  float kifsOffsetY = 1.0f;
  float hexScale = 8.0f;
};

#endif // KALEIDOSCOPE_CONFIG_H
