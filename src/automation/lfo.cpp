#include "lfo.h"
#include <math.h>
#include <stdlib.h>

static const float TAU = 6.283185307f;

static float GenerateWaveform(int waveform, float phase, const float* heldValue)
{
    switch (waveform) {
        case LFO_WAVE_SINE:
            return sinf(phase * TAU);

        case LFO_WAVE_TRIANGLE:
            // Map phase to triangle: rises 0->1 in first half, falls 1->-1 in second half
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

        default:
            return 0.0f;
    }
}

void LFOStateInit(LFOState* state)
{
    state->phase = 0.0f;
    state->currentOutput = 0.0f;
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded visualizer, simple randomness sufficient
    state->heldValue = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

float LFOProcess(LFOState* state, const LFOConfig* config, float deltaTime)
{
    if (!config->enabled) {
        state->currentOutput = 0.0f;
        return 0.0f;
    }

    // Advance phase
    state->phase += config->rate * deltaTime;

    // Wrap phase and update sample & hold on cycle boundary
    if (state->phase >= 1.0f) {
        state->phase -= floorf(state->phase);
        // NOLINTNEXTLINE(concurrency-mt-unsafe) - single-threaded visualizer, simple randomness sufficient
        state->heldValue = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }

    // Generate output based on waveform
    state->currentOutput = GenerateWaveform(config->waveform, state->phase,
                                             &state->heldValue);

    return state->currentOutput;
}
