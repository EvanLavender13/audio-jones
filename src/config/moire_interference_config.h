#ifndef MOIRE_INTERFERENCE_CONFIG_H
#define MOIRE_INTERFERENCE_CONFIG_H

// Moiré Interference: Multi-sample UV transform with rotated/scaled copies
// Small rotation/scale differences produce large-scale wave interference
// patterns
struct MoireInterferenceConfig {
  bool enabled = false;
  float rotationAngle = 0.087f;  // Angle between layers (radians, ~5°)
  float scaleDiff = 1.02f;       // Scale ratio between layers
  int layers = 2;                // Number of overlaid samples (2-4)
  int blendMode = 0;             // 0=multiply, 1=min, 2=average, 3=difference
  float centerX = 0.5f;          // Rotation/scale center X
  float centerY = 0.5f;          // Rotation/scale center Y
  float animationSpeed = 0.017f; // Rotation rate (radians/second, ~1°/s)
};

#endif // MOIRE_INTERFERENCE_CONFIG_H
