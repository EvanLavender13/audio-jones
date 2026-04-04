#ifndef MOD_BUS_CONFIG_H
#define MOD_BUS_CONFIG_H

#include <stdbool.h>

#define NUM_MOD_BUSES 8

typedef enum {
  BUS_OP_ADD = 0,     // clamp(A + B, -1, 1)
  BUS_OP_MULTIPLY,    // A * B
  BUS_OP_MIN,         // min(A, B)
  BUS_OP_MAX,         // max(A, B)
  BUS_OP_GATE,        // A > 0 ? B : 0
  BUS_OP_CROSSFADE,   // lerp(A, B, 0.5)
  BUS_OP_DIFFERENCE,  // abs(A - B)
  BUS_OP_ENV_FOLLOW,  // Continuous envelope follower (1-input)
  BUS_OP_ENV_TRIGGER, // Triggered one-shot envelope (1-input)
  BUS_OP_SLEW_EXP,    // Exponential lag (1-input)
  BUS_OP_SLEW_LINEAR, // Linear slew rate clamp (1-input)
  BUS_OP_COUNT
} BusOp;

static inline bool BusOpIsSingleInput(int op) {
  return op >= BUS_OP_ENV_FOLLOW;
}
static inline bool BusOpIsEnvelope(int op) {
  return op == BUS_OP_ENV_FOLLOW || op == BUS_OP_ENV_TRIGGER;
}
static inline bool BusOpIsSlew(int op) {
  return op == BUS_OP_SLEW_EXP || op == BUS_OP_SLEW_LINEAR;
}

struct ModBusConfig {
  bool enabled = false;
  char name[32] = {};
  int inputA = 0; // ModSource index (MOD_SOURCE_BASS)
  int inputB = 4; // ModSource index (MOD_SOURCE_LFO1)
  int op = BUS_OP_MULTIPLY;

  // Envelope params (ENV_FOLLOW, ENV_TRIGGER)
  float attack = 0.01f;   // 0.001-2.0 s
  float release = 0.3f;   // 0.01-5.0 s
  float hold = 0.0f;      // 0.0-2.0 s (ENV_TRIGGER only)
  float threshold = 0.3f; // 0.0-1.0 (ENV_TRIGGER only)

  // Slew params (SLEW_EXP, SLEW_LINEAR)
  float lagTime = 0.2f;  // 0.01-5.0 s
  float riseTime = 0.2f; // 0.01-5.0 s
  float fallTime = 0.2f; // 0.01-5.0 s
  bool asymmetric = false;
};

#endif // MOD_BUS_CONFIG_H
