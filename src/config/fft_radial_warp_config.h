#ifndef FFT_RADIAL_WARP_CONFIG_H
#define FFT_RADIAL_WARP_CONFIG_H

// FFT Radial Warp: audio-reactive radial displacement
// Maps FFT bins to screen radius - bass at center, treble at edges.
// Angular segments create bidirectional push/pull patterns.
struct FftRadialWarpConfig {
  bool enabled = false;
  float intensity = 0.1f;     // Displacement strength (0.0 - 0.5)
  float freqStart = 0.0f;     // FFT bin at center, 0 = bass (0.0 - 1.0)
  float freqEnd = 0.5f;       // FFT bin at maxRadius, 0.5 = mids (0.0 - 1.0)
  float maxRadius = 0.7f;     // Screen radius mapping to freqEnd (0.1 - 1.0)
  int segments = 4;           // Angular divisions for push/pull (1 - 16)
  float pushPullPhase = 0.0f; // Rotates push/pull pattern (radians)
};

#endif // FFT_RADIAL_WARP_CONFIG_H
