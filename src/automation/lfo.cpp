#include "lfo.h"
#include <math.h>
#include <stdlib.h>

static const float TAU = 6.283185307f;

static float GenerateWaveform(int waveform, float phase, const float *heldValue,
                              const float *prevHeldValue) {
  switch (waveform) {
  case LFO_WAVE_SINE:
    return sinf(phase * TAU);

  case LFO_WAVE_TRIANGLE:
    // Map phase to triangle: rises 0->1 in first half, falls 1->-1 in second
    // half
    if (phase < 0.5f) {
      return phase * 4.0f - 1.0f;
    } else {
      return 3.0f - phase * 4.0f;
    }

  case LFO_WAVE_SAWTOOTH:
    // Rising ramp from -1 to 1
    return phase * 2.0f - 1.0f;

  case LFO_WAVE_SQUARE:
    return (phase < 0.5f) ? 1.0f : -1.0f;

  case LFO_WAVE_SAMPLE_HOLD:
    // Return held value (updated on phase wrap in LFOProcess)
    return *heldValue;

  case LFO_WAVE_SMOOTH_RANDOM:
    // Linear interpolation from previous to current target
    return *prevHeldValue + (*heldValue - *prevHeldValue) * phase;

  default:
    return 0.0f;
  }
}

void LFOStateInit(LFOState *state) {
  state->phase = 0.0f;
  state->currentOutput = 0.0f;
  // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded visualizer, simple
  // randomness sufficient
  state->heldValue = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
  // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded visualizer, simple
  // randomness sufficient
  state->prevHeldValue = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

float LFOProcess(LFOState *state, const LFOConfig *config, float deltaTime) {
  if (!config->enabled) {
    state->currentOutput = 0.0f;
    return 0.0f;
  }

  // Advance phase
  state->phase += config->rate * deltaTime;

  // Wrap phase and update sample & hold on cycle boundary
  if (state->phase >= 1.0f) {
    state->phase -= floorf(state->phase);
    state->prevHeldValue = state->heldValue;
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded visualizer,
    // simple randomness sufficient
    state->heldValue = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
  }

  // Generate output based on waveform
  state->currentOutput = GenerateWaveform(
      config->waveform, state->phase, &state->heldValue, &state->prevHeldValue);

  return state->currentOutput;
}

// Deterministic pseudo-random for preview (consistent visual each frame)
static float PreviewRandom(int seed) {
  // Simple hash-based random for preview visualization
  unsigned int x = (unsigned int)(seed * 2654435761U);
  x ^= x >> 16;
  x *= 0x85ebca6bU;
  x ^= x >> 13;
  return ((float)(x & 0xFFFF) / 32768.0f) - 1.0f;
}

float LFOEvaluateWaveform(int waveform, float phase) {
  // For deterministic waveforms, evaluate directly
  switch (waveform) {
  case LFO_WAVE_SINE:
    return sinf(phase * TAU);

  case LFO_WAVE_TRIANGLE:
    if (phase < 0.5f) {
      return phase * 4.0f - 1.0f;
    } else {
      return 3.0f - phase * 4.0f;
    }

  case LFO_WAVE_SAWTOOTH:
    return phase * 2.0f - 1.0f;

  case LFO_WAVE_SQUARE:
    return (phase < 0.5f) ? 1.0f : -1.0f;

  case LFO_WAVE_SAMPLE_HOLD: {
    // Show 4 steps across the preview (4 cycles compressed)
    const int step = (int)(phase * 4.0f);
    return PreviewRandom(step);
  }

  case LFO_WAVE_SMOOTH_RANDOM: {
    // Show 4 segments across the preview (4 cycles compressed)
    const float scaledPhase = phase * 4.0f;
    const int segment = (int)scaledPhase;
    const float segmentPhase = scaledPhase - (float)segment;
    const float prevVal = PreviewRandom(segment);
    const float currVal = PreviewRandom(segment + 1);
    return prevVal + (currVal - prevVal) * segmentPhase;
  }

  default:
    return 0.0f;
  }
}
