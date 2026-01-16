#ifndef LFO_H
#define LFO_H

#include "config/lfo_config.h"

struct LFOState {
    float phase;          // Current position in cycle (0.0 to 1.0)
    float currentOutput;  // Last computed output (-1.0 to 1.0)
    float heldValue;      // Held random value for sample & hold
    float prevHeldValue;  // Previous random value for smooth random interpolation
};

void LFOStateInit(LFOState* state);
float LFOProcess(LFOState* state, const LFOConfig* config, float deltaTime);

// Evaluate waveform shape at given phase (for UI preview, excludes random-based waveforms)
float LFOEvaluateWaveform(int waveform, float phase);

#endif // LFO_H
