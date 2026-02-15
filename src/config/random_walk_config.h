#ifndef RANDOM_WALK_CONFIG_H
#define RANDOM_WALK_CONFIG_H

#include <math.h>
#include <stdint.h>

// Random walk motion via deterministic hash-based steps with smoothing.
// Produces wandering offset positions that vary per drawable via seed.

typedef enum {
  WALK_BOUNDARY_CLAMP = 0,
  WALK_BOUNDARY_WRAP = 1,
  WALK_BOUNDARY_DRIFT = 2,
} WalkBoundaryMode;

struct RandomWalkConfig {
  // Modulatable
  float stepSize = 0.02f;  // Distance per discrete step (0.001-0.1)
  float smoothness = 0.5f; // 0=jerky snaps, 1=smooth glide (0.0-1.0)

  // Non-modulatable (cause discontinuities)
  float tickRate = 20.0f; // Discrete steps per second (1.0-60.0)
  WalkBoundaryMode boundaryMode = WALK_BOUNDARY_DRIFT;
  float driftStrength = 0.3f; // Pull toward center in Drift mode (0.0-2.0)
  int seed = 0;               // 0 = auto from drawableId
};

#define RANDOM_WALK_CONFIG_FIELDS                                              \
  stepSize, smoothness, tickRate, boundaryMode, driftStrength, seed

struct RandomWalkState {
  float walkX = 0.0f; // Current discrete position (offset, clamped +/-0.48)
  float walkY = 0.0f;
  float prevX = 0.0f; // Previous discrete position (for interpolation)
  float prevY = 0.0f;
  float timeAccum = 0.0f;   // Fractional time within current tick
  uint32_t tickCounter = 0; // Step counter for hash input
};

// Splitmix-style integer hash for deterministic pseudo-random steps
inline uint32_t RandomWalkHash(uint32_t x) {
  x ^= x >> 16;
  x *= 0x45d9f3b;
  x ^= x >> 16;
  x *= 0x45d9f3b;
  x ^= x >> 16;
  return x;
}

// Convert hash to float in [0, 1]
inline float RandomWalkHashFloat(uint32_t x) {
  return (float)(RandomWalkHash(x) & 0xFFFFFF) / (float)0xFFFFFF;
}

// Accumulates time, advances discrete steps via hash, interpolates output.
// drawableId: used as seed source when cfg->seed == 0
// outX, outY: offset values to add to base position
inline void RandomWalkUpdate(RandomWalkConfig *cfg, RandomWalkState *state,
                             uint32_t drawableId, float deltaTime, float *outX,
                             float *outY) {
  const uint32_t baseSeed =
      (cfg->seed == 0) ? RandomWalkHash(drawableId) : (uint32_t)cfg->seed;

  state->timeAccum += deltaTime * cfg->tickRate;

  while (state->timeAccum >= 1.0f) {
    // Hash two values for dx and dy
    const float hashX = RandomWalkHashFloat(baseSeed + state->tickCounter * 2);
    const float hashY =
        RandomWalkHashFloat(baseSeed + state->tickCounter * 2 + 1);

    const float dx = (hashX - 0.5f) * 2.0f * cfg->stepSize;
    const float dy = (hashY - 0.5f) * 2.0f * cfg->stepSize;

    // Store previous position for interpolation
    state->prevX = state->walkX;
    state->prevY = state->walkY;

    // Advance position
    state->walkX += dx;
    state->walkY += dy;

    // Apply boundary mode
    switch (cfg->boundaryMode) {
    case WALK_BOUNDARY_CLAMP:
      if (state->walkX < -0.48f)
        state->walkX = -0.48f;
      if (state->walkX > 0.48f)
        state->walkX = 0.48f;
      if (state->walkY < -0.48f)
        state->walkY = -0.48f;
      if (state->walkY > 0.48f)
        state->walkY = 0.48f;
      break;

    case WALK_BOUNDARY_WRAP: {
      // Wrap to [-0.48, 0.48] range
      const float range = 0.96f; // 0.48 * 2
      const float preX = state->walkX;
      const float preY = state->walkY;
      state->walkX = fmodf(state->walkX + 0.48f, range);
      if (state->walkX < 0.0f)
        state->walkX += range;
      state->walkX -= 0.48f;
      state->walkY = fmodf(state->walkY + 0.48f, range);
      if (state->walkY < 0.0f)
        state->walkY += range;
      state->walkY -= 0.48f;
      // Snap prev only on axes that actually wrapped to avoid cross-screen lerp
      if (fabsf(state->walkX - preX) > cfg->stepSize * 2.0f)
        state->prevX = state->walkX;
      if (fabsf(state->walkY - preY) > cfg->stepSize * 2.0f)
        state->prevY = state->walkY;
      break;
    }

    case WALK_BOUNDARY_DRIFT: {
      const float driftRate = cfg->driftStrength * (1.0f / cfg->tickRate);
      state->walkX += (0.0f - state->walkX) * driftRate;
      state->walkY += (0.0f - state->walkY) * driftRate;
      break;
    }
    }

    state->tickCounter++;
    state->timeAccum -= 1.0f;
  }

  // Interpolate between previous and current position
  const float frac = state->timeAccum;
  const float smoothX = state->prevX + (state->walkX - state->prevX) * frac;
  const float smoothY = state->prevY + (state->walkY - state->prevY) * frac;

  // Blend between snappy (walkPos) and smooth (interpolated) based on
  // smoothness
  *outX = state->walkX + (smoothX - state->walkX) * cfg->smoothness;
  *outY = state->walkY + (smoothY - state->walkY) * cfg->smoothness;
}

// Zeroes all state fields. Called on motion mode switch and preset load.
inline void RandomWalkReset(RandomWalkState *state) {
  state->walkX = 0.0f;
  state->walkY = 0.0f;
  state->prevX = 0.0f;
  state->prevY = 0.0f;
  state->timeAccum = 0.0f;
  state->tickCounter = 0;
}

#endif // RANDOM_WALK_CONFIG_H
