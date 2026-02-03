#ifndef FFT_RADIAL_WARP_CONFIG_H
#define FFT_RADIAL_WARP_CONFIG_H

// FFT Radial Warp: audio-reactive radial displacement
// Maps FFT bins to screen radius - bass at center, treble at edges.
// Angular segments create bidirectional push/pull patterns.
struct FftRadialWarpConfig {
  bool enabled = false;
  float intensity = 0.1f; // Displacement strength (0.0 - 1.0)
  float freqStart = 0.0f; // FFT bin at center, 0 = bass (0.0 - 1.0)
  float freqEnd = 0.5f;   // FFT bin at maxRadius, 0.5 = mids (0.0 - 1.0)
  float maxRadius = 0.7f; // Screen radius mapping to freqEnd (0.1 - 1.0)
  float freqCurve = 1.0f; // pow(magnitude, curve) - >1 punchier (0.5 - 3.0)
  float bassBoost = 0.0f; // Extra strength at center/bass (0.0 - 2.0)
  int segments = 4;       // Angular divisions for push/pull (1 - 16)
  float pushPullBalance = 0.5f; // 0=all pull, 0.5=equal, 1=all push (0.0 - 1.0)
  float pushPullSmoothness =
      0.0f;                // Hard edges to gradual transitions (0.0 - 1.0)
  float phaseSpeed = 0.0f; // Auto-rotate speed (radians/second)
};

#endif // FFT_RADIAL_WARP_CONFIG_H
