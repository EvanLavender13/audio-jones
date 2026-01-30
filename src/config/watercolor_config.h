#ifndef WATERCOLOR_CONFIG_H
#define WATERCOLOR_CONFIG_H

// Watercolor: Gradient-flow stroke tracing with paper granulation.
// Traces perpendicular to gradient (outlines) and along gradient (color wash).
struct WatercolorConfig {
  bool enabled = false;
  int samples = 24;        // Trace iterations per pixel (8-32)
  float strokeStep = 1.0f; // Outline trace step length (0.4-2.0)
  float washStrength =
      0.7f;                // Wash color blend (0.0=outline only, 1.0=full wash)
  float paperScale = 8.0f; // Paper texture frequency (1.0-20.0)
  float paperStrength = 0.4f; // Paper texture intensity (0.0-1.0)
};

#endif // WATERCOLOR_CONFIG_H
