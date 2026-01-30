#ifndef PENCIL_SKETCH_CONFIG_H
#define PENCIL_SKETCH_CONFIG_H

// Pencil Sketch: Directional gradient-aligned stroke accumulation with paper
// texture Maps luminance gradients to hatching strokes along multiple angles
struct PencilSketchConfig {
  bool enabled = false;
  int angleCount = 3;            // Number of hatching directions (2-6)
  int sampleCount = 16;          // Samples per direction/stroke length (8-24)
  float strokeFalloff = 1.0f;    // Distance fade rate (0.0-1.0)
  float gradientEps = 0.4f;      // Edge sensitivity epsilon (0.2-1.0)
  float paperStrength = 0.5f;    // Paper texture visibility (0.0-1.0)
  float vignetteStrength = 1.0f; // Edge darkening (0.0-1.0)
  float wobbleSpeed = 1.0f;      // Animation rate, 0 = static (0.0-2.0)
  float wobbleAmount = 4.0f;     // Pixel displacement magnitude (0.0-8.0)
};

#endif // PENCIL_SKETCH_CONFIG_H
