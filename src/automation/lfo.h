#ifndef LFO_H
#define LFO_H

#include "config/lfo_config.h"

typedef struct {
    float phase;          // Current position in cycle (0.0 to 1.0)
    float currentOutput;  // Last computed output (-1.0 to 1.0)
    float heldValue;      // Held random value for sample & hold
    float prevHeldValue;  // Previous random value for smooth random interpolation
} LFOState;

void LFOStateInit(LFOState* state);
float LFOProcess(LFOState* state, const LFOConfig* config, float deltaTime);

#endif // LFO_H
