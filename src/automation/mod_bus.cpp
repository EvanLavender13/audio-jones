#include "mod_bus.h"
#include "automation/mod_sources.h"
#include <math.h>

static const float TARGET_RATIO = 0.01f;

void ModBusStateInit(ModBusState *state) {
  state->output = 0.0f;
  state->prevInput = 0.0f;
  state->envPhase = BUS_ENV_IDLE;
  state->holdTimer = 0.0f;
}

static float ProcessCombinator(int op, float a, float b) {
  switch (op) {
  case BUS_OP_ADD:
    return fminf(fmaxf(a + b, -1.0f), 1.0f);
  case BUS_OP_MULTIPLY:
    return a * b;
  case BUS_OP_MIN:
    return fminf(a, b);
  case BUS_OP_MAX:
    return fmaxf(a, b);
  case BUS_OP_GATE:
    return a > 0.0f ? b : 0.0f;
  case BUS_OP_CROSSFADE:
    return a + (b - a) * 0.5f;
  case BUS_OP_DIFFERENCE:
    return fabsf(a - b);
  default:
    return 0.0f;
  }
}

static void ProcessEnvFollow(ModBusState *state, const ModBusConfig *cfg,
                             float inputVal, float deltaTime) {
  const float v = fabsf(inputVal);
  float coef;
  if (v > state->output) {
    coef = (cfg->attack > 0.0f) ? powf(0.01f, deltaTime / cfg->attack) : 0.0f;
  } else {
    coef = (cfg->release > 0.0f) ? powf(0.01f, deltaTime / cfg->release) : 0.0f;
  }
  state->output = coef * (state->output - v) + v;
  state->output = fminf(fmaxf(state->output, 0.0f), 1.0f);
}

static void ProcessEnvTrigger(ModBusState *state, const ModBusConfig *cfg,
                              float inputVal, float deltaTime) {
  const float v = fabsf(inputVal);
  const float TR = TARGET_RATIO;

  // Threshold crossing detection
  if (v > cfg->threshold && state->prevInput <= cfg->threshold) {
    if (state->envPhase == BUS_ENV_IDLE || state->envPhase == BUS_ENV_DECAY) {
      // Retrigger from current output (no discontinuity)
      state->envPhase = BUS_ENV_ATTACK;
      state->holdTimer = cfg->hold;
    }
  }

  switch (state->envPhase) {
  case BUS_ENV_IDLE:
    state->output = 0.0f;
    break;

  case BUS_ENV_ATTACK: {
    if (cfg->attack <= 0.0f) {
      state->output = 1.0f;
    } else {
      const float coef =
          expf(-logf((1.0f + TR) / TR) * deltaTime / cfg->attack);
      const float base = (1.0f + TR) * (1.0f - coef);
      state->output = base + state->output * coef;
    }
    if (state->output >= 1.0f) {
      state->output = 1.0f;
      if (cfg->hold > 0.0f) {
        state->envPhase = BUS_ENV_HOLD;
      } else {
        state->envPhase = BUS_ENV_DECAY;
      }
    }
    break;
  }

  case BUS_ENV_HOLD:
    state->output = 1.0f;
    state->holdTimer -= deltaTime;
    if (state->holdTimer <= 0.0f) {
      state->envPhase = BUS_ENV_DECAY;
    }
    break;

  case BUS_ENV_DECAY: {
    if (cfg->release <= 0.0f) {
      state->output = 0.0f;
      state->envPhase = BUS_ENV_IDLE;
    } else {
      const float coef =
          expf(-logf((1.0f + TR) / TR) * deltaTime / cfg->release);
      const float base = -TR * (1.0f - coef);
      state->output = base + state->output * coef;
      if (state->output <= 0.001f) {
        state->output = 0.0f;
        state->envPhase = BUS_ENV_IDLE;
      }
    }
    break;
  }
  default:
    break;
  }

  state->prevInput = v;
}

static void ProcessSlewExp(ModBusState *state, const ModBusConfig *cfg, float v,
                           float deltaTime) {
  float speed;
  if (cfg->asymmetric) {
    if (v > state->output) {
      speed = (cfg->riseTime > 0.0f) ? (1.0f / cfg->riseTime) : 1e6f;
    } else {
      speed = (cfg->fallTime > 0.0f) ? (1.0f / cfg->fallTime) : 1e6f;
    }
  } else {
    speed = (cfg->lagTime > 0.0f) ? (1.0f / cfg->lagTime) : 1e6f;
  }
  const float alpha = 1.0f - expf(-speed * deltaTime);
  state->output += alpha * (v - state->output);
}

static void ProcessSlewLinear(ModBusState *state, const ModBusConfig *cfg,
                              float v, float deltaTime) {
  float rate;
  if (cfg->asymmetric) {
    if (v > state->output) {
      rate = (cfg->riseTime > 0.0f) ? (2.0f / cfg->riseTime) : 1e6f;
    } else {
      rate = (cfg->fallTime > 0.0f) ? (2.0f / cfg->fallTime) : 1e6f;
    }
  } else {
    rate = (cfg->lagTime > 0.0f) ? (2.0f / cfg->lagTime) : 1e6f;
  }
  const float maxDelta = rate * deltaTime;
  float delta = v - state->output;
  if (delta > maxDelta) {
    delta = maxDelta;
  }
  if (delta < -maxDelta) {
    delta = -maxDelta;
  }
  state->output += delta;
}

void ModBusEvaluate(ModBusState states[], const ModBusConfig configs[],
                    ModSources *sources, float deltaTime) {
  for (int i = 0; i < NUM_MOD_BUSES; i++) {
    const ModBusConfig *cfg = &configs[i];
    ModBusState *state = &states[i];

    if (!cfg->enabled) {
      sources->values[MOD_SOURCE_BUS1 + i] = 0.0f;
      continue;
    }

    const float a = sources->values[cfg->inputA];
    const float b = sources->values[cfg->inputB];

    if (BusOpIsSingleInput(cfg->op)) {
      switch (cfg->op) {
      case BUS_OP_ENV_FOLLOW:
        ProcessEnvFollow(state, cfg, a, deltaTime);
        break;
      case BUS_OP_ENV_TRIGGER:
        ProcessEnvTrigger(state, cfg, a, deltaTime);
        break;
      case BUS_OP_SLEW_EXP:
        ProcessSlewExp(state, cfg, a, deltaTime);
        break;
      case BUS_OP_SLEW_LINEAR:
        ProcessSlewLinear(state, cfg, a, deltaTime);
        break;
      default:
        state->output = 0.0f;
        break;
      }
    } else {
      state->output = ProcessCombinator(cfg->op, a, b);
    }

    sources->values[MOD_SOURCE_BUS1 + i] = state->output;
  }
}
