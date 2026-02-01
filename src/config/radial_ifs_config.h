// src/config/radial_ifs_config.h
#ifndef RADIAL_IFS_CONFIG_H
#define RADIAL_IFS_CONFIG_H

// Radial IFS: Iterated polar wedge folding for snowflake/flower fractals
// Each iteration: polar fold -> translate -> scale -> twist
struct RadialIfsConfig {
  bool enabled = false;
  int segments = 6;           // Wedge count per fold (3-12)
  int iterations = 4;         // Recursion depth (1-8)
  float scale = 1.8f;         // Expansion per iteration (1.2-2.5)
  float offset = 0.5f;        // Translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  float smoothing = 0.0f;     // Blend width at wedge seams (0.0-0.5)
};

#endif // RADIAL_IFS_CONFIG_H
