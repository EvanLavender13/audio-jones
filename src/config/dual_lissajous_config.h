#ifndef DUAL_LISSAJOUS_CONFIG_H
#define DUAL_LISSAJOUS_CONFIG_H

#include "config/constants.h"
#include <math.h>

// Dual-harmonic Lissajous configuration
// Two frequencies per axis create quasi-periodic motion that never exactly
// repeats when ratios are irrational.
struct DualLissajousConfig {
  // Modulatable params
  float amplitude = 0.2f;   // Motion amplitude (0.0-0.5)
  float motionSpeed = 1.0f; // Phase accumulation rate (0.0-5.0)

  // Shape params (not modulatable - cause discontinuities)
  float freqX1 = 0.05f;   // Primary X frequency (Hz)
  float freqY1 = 0.08f;   // Primary Y frequency (Hz)
  float freqX2 = 0.0f;    // Secondary X frequency (Hz, 0 = disabled)
  float freqY2 = 0.0f;    // Secondary Y frequency (Hz, 0 = disabled)
  float offsetX2 = 0.3f;  // Phase offset for secondary X (radians)
  float offsetY2 = 3.48f; // Phase offset for secondary Y (radians)

  // Internal state (not serialized)
  float phase = 0.0f; // Accumulated phase
};

#define DUAL_LISSAJOUS_CONFIG_FIELDS                                           \
  amplitude, motionSpeed, freqX1, freqY1, freqX2, freqY2, offsetX2, offsetY2

// Accumulates phase internally, then computes offset
// deltaTime: frame time in seconds
// perSourceOffset: additional phase offset for this source (e.g., i/count *
// TWO_PI) outX, outY: offset values to add to base position
inline void DualLissajousUpdate(DualLissajousConfig *cfg, float deltaTime,
                                float perSourceOffset, float *outX,
                                float *outY) {
  cfg->phase += deltaTime * cfg->motionSpeed;

  const float phaseX1 = cfg->phase * cfg->freqX1 + perSourceOffset;
  const float phaseY1 = cfg->phase * cfg->freqY1 + perSourceOffset;

  float x = sinf(phaseX1);
  float y = cosf(phaseY1);

  if (cfg->freqX2 > 0.0f) {
    const float phaseX2 =
        cfg->phase * cfg->freqX2 + cfg->offsetX2 + perSourceOffset;
    x += sinf(phaseX2);
  }
  if (cfg->freqY2 > 0.0f) {
    const float phaseY2 =
        cfg->phase * cfg->freqY2 + cfg->offsetY2 + perSourceOffset;
    y += cosf(phaseY2);
  }

  const float scaleX = (cfg->freqX2 > 0.0f) ? 0.5f : 1.0f;
  const float scaleY = (cfg->freqY2 > 0.0f) ? 0.5f : 1.0f;

  *outX = cfg->amplitude * x * scaleX;
  *outY = cfg->amplitude * y * scaleY;
}

// Compute N positions arranged in a circle with shared Lissajous motion.
// First iteration advances phase; all others use the accumulated phase.
// centerX/centerY: circle center (0,0 for centered UV, 0.5,0.5 for normalized)
// outPositions: interleaved x,y pairs (must hold count*2 floats)
inline void DualLissajousUpdateCircular(DualLissajousConfig *cfg,
                                        float deltaTime, float baseRadius,
                                        float centerX, float centerY, int count,
                                        float *outPositions) {
  for (int i = 0; i < count; i++) {
    const float angle = TWO_PI_F * (float)i / (float)count;
    const float baseX = centerX + baseRadius * cosf(angle);
    const float baseY = centerY + baseRadius * sinf(angle);
    const float perSourceOffset = (float)i / (float)count * TWO_PI_F;

    const float dt = (i == 0) ? deltaTime : 0.0f;
    float offsetX, offsetY;
    DualLissajousUpdate(cfg, dt, perSourceOffset, &offsetX, &offsetY);

    outPositions[i * 2 + 0] = baseX + offsetX;
    outPositions[i * 2 + 1] = baseY + offsetY;
  }
}

#endif // DUAL_LISSAJOUS_CONFIG_H
