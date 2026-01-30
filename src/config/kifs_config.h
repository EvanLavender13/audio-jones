#ifndef KIFS_CONFIG_H
#define KIFS_CONFIG_H

// KIFS (Kaleidoscopic IFS): Fractal folding via Cartesian abs() reflection
// Each iteration: abs(p) fold → optional octant fold → scale → translate →
// rotate
struct KifsConfig {
  bool enabled = false;
  int iterations = 4;         // Fold/scale/translate cycles (1-6)
  float scale = 2.0f;         // Expansion factor per iteration (1.5-2.5)
  float offsetX = 1.0f;       // X translation after fold (0.0-2.0)
  float offsetY = 1.0f;       // Y translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  bool octantFold = false; // Enable 8-way octant symmetry (swap x/y when x < y)
  bool polarFold =
      false; // Enable polar coordinate pre-fold for radial symmetry
  int polarFoldSegments = 6; // Wedge count for polar fold (2-12)
  float polarFoldSmoothing =
      0.0f; // Blend width at polar fold seams (0.0-0.5, 0 = hard edge)
};

#endif // KIFS_CONFIG_H
