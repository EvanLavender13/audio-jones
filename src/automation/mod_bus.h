#ifndef MOD_BUS_H
#define MOD_BUS_H

#include "config/mod_bus_config.h"

struct ModSources;

typedef enum {
  BUS_ENV_IDLE,
  BUS_ENV_ATTACK,
  BUS_ENV_HOLD,
  BUS_ENV_DECAY
} BusEnvPhase;

typedef struct ModBusState {
  float output;    // Current processed output (all modes)
  float prevInput; // Previous frame input (trigger threshold detection)
  int envPhase;    // BusEnvPhase for triggered envelope state machine
  float holdTimer; // Seconds remaining in hold phase
} ModBusState;

// Zero all fields
void ModBusStateInit(ModBusState *state);

// Evaluate all 8 buses, writing results to sources->values[MOD_SOURCE_BUS1 + i]
void ModBusEvaluate(ModBusState states[], const ModBusConfig configs[],
                    ModSources *sources, float deltaTime);

#endif // MOD_BUS_H
