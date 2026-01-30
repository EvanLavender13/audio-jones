#ifndef INK_WASH_CONFIG_H
#define INK_WASH_CONFIG_H

// Ink Wash: Sobel edge darkening, FBM paper granulation, directional color
// bleed.
struct InkWashConfig {
  bool enabled = false;
  float strength = 1.0f;      // Edge darkening intensity (0.0-2.0)
  float granulation = 0.5f;   // Paper noise intensity (0.0-1.0)
  float bleedStrength = 0.5f; // Directional color bleed (0.0-1.0)
  float bleedRadius = 5.0f;   // How far colors spread at edges (1.0-10.0)
  float softness = 0.0f;      // Pre-blur radius before edge detection (0.0-5.0)
};

#endif // INK_WASH_CONFIG_H
