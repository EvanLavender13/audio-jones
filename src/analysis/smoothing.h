#ifndef SMOOTHING_H
#define SMOOTHING_H

#include <math.h>

// Running average decay factor (0.999 = ~1 second time constant at 60Hz)
#define AVG_DECAY 0.999f
#define AVG_ATTACK 0.001f

// Attack/release time constants (seconds)
#define ENVELOPE_ATTACK_TIME 0.010f  // 10ms - captures transients
#define ENVELOPE_RELEASE_TIME 0.150f // 150ms - prevents jitter

// Minimum denominator to avoid division by zero
#define MIN_DENOM 1e-6f

// Apply attack/release envelope follower (inline for performance)
// Uses faster attack, slower release for natural-feeling audio response
static inline void ApplyEnvelope(float *smoothed, float raw, float dt,
                                 float attackTime, float releaseTime) {
  const float tau = (raw > *smoothed) ? attackTime : releaseTime;
  const float alpha = 1.0f - expf(-dt / tau);
  *smoothed += alpha * (raw - *smoothed);
}

// Convenience wrapper using default attack/release times
static inline void ApplyEnvelopeDefault(float *smoothed, float raw, float dt) {
  ApplyEnvelope(smoothed, raw, dt, ENVELOPE_ATTACK_TIME, ENVELOPE_RELEASE_TIME);
}

// Update exponential moving average
static inline void UpdateRunningAvg(float *avg, float raw) {
  *avg = *avg * AVG_DECAY + raw * AVG_ATTACK;
}

#endif // SMOOTHING_H
