#ifndef DUAL_LISSAJOUS_CONFIG_H
#define DUAL_LISSAJOUS_CONFIG_H

#include <math.h>

// Dual-harmonic Lissajous configuration
// Two frequencies per axis create quasi-periodic motion that never exactly
// repeats when ratios are irrational.
struct DualLissajousConfig {
  float amplitude = 0.2f; // Motion amplitude (0.0-0.5)
  float freqX1 = 0.05f;   // Primary X frequency (Hz)
  float freqY1 = 0.08f;   // Primary Y frequency (Hz)
  float freqX2 = 0.0f;    // Secondary X frequency (Hz, 0 = disabled)
  float freqY2 = 0.0f;    // Secondary Y frequency (Hz, 0 = disabled)
  float offsetX2 = 0.3f;  // Phase offset for secondary X (radians)
  float offsetY2 = 3.48f; // Phase offset for secondary Y (radians)
};

// Compute dual-harmonic Lissajous offset from base position
// phase: accumulated time in radians (typically time * TWO_PI)
// perSourceOffset: additional phase offset per source
// outX, outY: offset values to add to base position
inline void DualLissajousCompute(const DualLissajousConfig *cfg, float phase,
                                 float perSourceOffset, float *outX,
                                 float *outY) {
  const float phaseX1 = phase * cfg->freqX1 + perSourceOffset;
  const float phaseY1 = phase * cfg->freqY1 + perSourceOffset;

  float x = sinf(phaseX1);
  float y = cosf(phaseY1);

  // Add secondary harmonics if enabled (0 = disabled)
  if (cfg->freqX2 > 0.0f) {
    const float phaseX2 = phase * cfg->freqX2 + cfg->offsetX2 + perSourceOffset;
    x += sinf(phaseX2);
  }
  if (cfg->freqY2 > 0.0f) {
    const float phaseY2 = phase * cfg->freqY2 + cfg->offsetY2 + perSourceOffset;
    y += cosf(phaseY2);
  }

  // Normalize: single harmonic range [-1,1], dual range [-2,2]
  const float scaleX = (cfg->freqX2 > 0.0f) ? 0.5f : 1.0f;
  const float scaleY = (cfg->freqY2 > 0.0f) ? 0.5f : 1.0f;

  *outX = cfg->amplitude * x * scaleX;
  *outY = cfg->amplitude * y * scaleY;
}

#endif // DUAL_LISSAJOUS_CONFIG_H
