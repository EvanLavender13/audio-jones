#ifndef BOKEH_CONFIG_H
#define BOKEH_CONFIG_H

// Bokeh: Simulates out-of-focus camera blur with golden-angle Vogel disc
// sampling Bright spots bloom into soft circular highlights weighted by
// brightness
struct BokehConfig {
  bool enabled = false;
  float radius = 0.02f; // Blur disc size in UV space (0.0-0.1)
  int iterations =
      64; // Sample count (16-150). Higher = better quality, slower.
  float brightnessPower =
      4.0f; // Brightness weighting exponent (1.0-8.0). Higher = more "pop".
};

#endif // BOKEH_CONFIG_H
