# Modulation Envelope Processors

Bus-integrated envelope processors that shape modulation signals with attack/release dynamics. Two modes: a continuous envelope follower (smooths any source with separate rise/fall rates) and a triggered one-shot envelope (fires an attack-decay shape when a source crosses a threshold). Lives as a bus processor type alongside combinators.

## Classification

- **Category**: General (modulation engine architecture)
- **Pipeline Position**: Evaluated per frame as a bus processor (1-input bus type)

## References

- [Envelope Follower with Different Attack and Release - musicdsp.org](https://www.musicdsp.org/en/latest/Analysis/136-envelope-follower-with-different-attack-and-release.html) - Canonical one-pole envelope follower with separate attack/release coefficients
- [Envelope Generators: ADSR Code - EarLevel Engineering](https://www.earlevel.com/main/2013/06/03/envelope-generators-adsr-code/) - State machine ADSR with exponential segments, targetRatio curve control
- [Dynamics Processing Part 1 - flyingSand](https://christianfloisand.wordpress.com/2014/06/09/dynamics-processing-compressorlimiter-part-1/) - `g = exp(-1/(t*sr))` coefficient derivation, diode-bypass analogy
- [A One-Pole Filter - EarLevel Engineering](https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/) - `b1 = exp(-2*PI*Fc)` derivation for one-pole lowpass
- [Output Limiter using Envelope Follower - musicdsp.org](https://www.musicdsp.org/en/latest/Filters/265-output-limiter-using-envelope-follower-in-c.html) - Confirms `pow(0.01, ...)` formula in C++

## Reference Code

### Envelope Follower (musicdsp.org entry 136)

```c
// One-pole envelope follower with asymmetric attack/release
// Coefficients from time in milliseconds:
//   coef = pow(0.01, 1.0 / (timeMs * 0.001 * sampleRate))
// Where 0.01 means "time to reach 99% of target"

float envelope = 0.0f;

// Per-sample (or per-frame at frame rate):
float v = fabsf(input);
if (v > envelope)
    envelope = attackCoef * (envelope - v) + v;   // rising
else
    envelope = releaseCoef * (envelope - v) + v;  // falling
```

Algebraically equivalent to `envelope += (1 - coef) * (v - envelope)` where a smaller coef = faster response.

### Frame-rate-independent coefficient conversion

```c
// Convert time in seconds to one-pole coefficient at given frame rate
// tau = time for output to reach 99% of step input
float TimeToCoef(float timeSec, float frameRate) {
    if (timeSec <= 0.0f) return 0.0f;  // instant
    return powf(0.01f, 1.0f / (timeSec * frameRate));
}
```

### Triggered one-shot (EarLevel ADSR, adapted)

```c
// State machine: IDLE -> ATTACK -> HOLD -> DECAY -> IDLE
// Trigger on threshold crossing: input > threshold AND prev <= threshold

// Attack phase: exponential rise toward 1.0
attackCoef = expf(-logf((1.0f + targetRatio) / targetRatio) / attackFrames);
attackBase = (1.0f + targetRatio) * (1.0f - attackCoef);
// Per-frame: output = attackBase + output * attackCoef
// Transition to HOLD when output >= 1.0

// Decay phase: exponential fall toward 0.0
decayCoef = expf(-logf((1.0f + targetRatio) / targetRatio) / decayFrames);
decayBase = -targetRatio * (1.0f - decayCoef);
// Per-frame: output = decayBase + output * decayCoef
// Transition to IDLE when output <= 0.001

// targetRatio controls curve shape:
//   small (0.001) = steep exponential
//   large (100.0) = near-linear
```

## Algorithm

### Integration as Bus Processor

Envelope processors are a bus type with 1 input (not 2). The bus operator field determines the mode:

```
BUS_OP_ENV_FOLLOW = 7,    // continuous envelope follower
BUS_OP_ENV_TRIGGER = 8,   // triggered one-shot
```

When a bus uses an envelope operator, `inputB` is ignored. The bus config gains additional fields for envelope parameters (attack, release, hold, threshold).

### Envelope Follower Mode

Per-frame update, adapted from musicdsp.org reference:

1. Read absolute value of input source: `v = fabsf(sources->values[bus.inputA])`
2. Compute coefficients from attack/release times: `coef = powf(0.01f, 1.0f / (timeSec * frameRate))`
3. Apply asymmetric smoothing:
   - If `v > state.envelope`: `state.envelope = attackCoef * (state.envelope - v) + v`
   - Else: `state.envelope = releaseCoef * (state.envelope - v) + v`
4. Write `state.envelope` to bus output slot (clamped to [0, 1])

### Triggered One-Shot Mode

State machine per bus, adapted from EarLevel ADSR:

1. **IDLE**: Output = 0. Check trigger condition: `input > threshold AND prevInput <= threshold`
2. **ATTACK**: Exponential rise. `output = attackBase + output * attackCoef`. Transition to HOLD when `output >= 1.0`
3. **HOLD**: Output = 1.0 for `holdFrames` frames. Transition to DECAY when hold expires
4. **DECAY**: Exponential fall. `output = decayBase + output * decayCoef`. Transition to IDLE when `output <= 0.001`

Retrigger behavior: if a new trigger occurs during DECAY, jump back to ATTACK from current output value (no discontinuity).

### Runtime State

Each envelope bus needs persistent state between frames:

```c
struct BusEnvState {
    float envelope;      // current follower output (follower mode)
    float output;        // current envelope output (trigger mode)
    float prevInput;     // for threshold crossing detection
    int state;           // IDLE/ATTACK/HOLD/DECAY
    int holdCounter;     // frames remaining in hold phase
};
```

This state is NOT serialized in presets -- it resets on load.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| attack | float | 0.001-2.0 | 0.01 | Rise time in seconds (both modes) |
| release | float | 0.01-5.0 | 0.3 | Fall time in seconds (follower: decay rate; trigger: decay segment) |
| hold | float | 0.0-2.0 | 0.0 | Hold time at peak in seconds (trigger mode only, 0 = no hold) |
| threshold | float | 0.0-1.0 | 0.3 | Trigger threshold (trigger mode only) |

These fields extend the bus config struct when the bus operator is an envelope type.

## Modulation Candidates

- **attack**: Shorter attack during energetic sections makes envelope snappier
- **release**: Longer release during quiet sections creates lingering trails
- **threshold**: Lower threshold catches quieter transients

### Interaction Patterns

**Cascading threshold with audio energy**: Route bass energy to the threshold parameter. During loud sections, threshold rises and only the strongest transients fire the envelope. During quiet sections, threshold drops and subtle hits trigger responses. The envelope self-adjusts its sensitivity to match the music's dynamics.

## Notes

- Coefficient recomputation every frame is negligible (two `powf` calls per active envelope bus)
- The 0.01 threshold in coefficient conversion means "time to reach 99% of target" -- perceptually complete
- `targetRatio` for triggered mode is fixed at 0.01 (steep exponential). Could become a parameter if linear shapes are needed
- Envelope buses output unipolar [0, 1] regardless of input polarity (absolute value taken)
- Hold phase is optional (0 = skip directly to decay) -- useful for sharp transient response without plateau
