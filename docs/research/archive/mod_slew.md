# Modulation Slew/Lag Processor

Bus-integrated slew rate limiter that smooths modulation signals over time. Prevents sudden jumps in modulated parameters by limiting how fast the signal can change per frame. Produces glide, portamento, and inertia effects on any modulation source.

## Classification

- **Category**: General (modulation engine architecture)
- **Pipeline Position**: Evaluated per frame as a bus processor (1-input bus type)

## References

- [Digital Slew Rate Limiter - dsp-weimich.com](https://www.dsp-weimich.com/digital-signal-processing/digital-slew-rate-limiter-filter-and-c-realization/) - Hard clamp implementation with asymmetric rise/fall rates
- [Slew Rate Limiters: Nonlinear and Proud of It! - Jason Sachs](https://www.embeddedrelated.com/showarticle/646.php) - Nonlinear vs. linear smoothing distinction, slew vs. lowpass
- [Single-Pole Low-Pass Filter - Jason Sachs](https://www.embeddedrelated.com/showarticle/779.php) - One-pole filter as lag processor, coefficient math
- [1-pole LPF for smooth parameter changes - musicdsp.org](https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html) - Sample-rate compensated smoothing formula
- [Exponential smoothing - lisyarus blog](https://lisyarus.github.io/blog/posts/exponential-smoothing.html) - Frame-rate independent `exp(-speed * dt)` derivation and correctness proof
- [Improved Lerp Smoothing - Game Developer](https://www.gamedeveloper.com/programming/improved-lerp-smoothing-) - `lerp(target, value, exp2(-rate * dt))` for variable timestep

## Reference Code

### One-pole lag (frame-rate independent)

```c
// Per-frame update: output approaches input exponentially
// speed = 1/lagTime (inverse seconds). Higher = faster response.
float alpha = 1.0f - expf(-speed * deltaTime);
output += alpha * (input - output);

// Equivalently:
output = lerp(output, input, 1.0f - expf(-speed * deltaTime));
```

This is correct under variable frame rate: applying it 60 times at dt=1/60 produces the same result as 30 times at dt=1/30.

### Hard slew clamp

```c
// Per-frame update: clamp maximum change per frame
float delta = input - output;
float maxChange = slewRate * deltaTime;  // units/second * seconds
if (delta > maxChange) delta = maxChange;
if (delta < -maxChange) delta = -maxChange;
output += delta;
```

### Coefficient from time constant

```c
// lagTime = seconds for output to reach ~63% of step input
// speed = 1.0 / lagTime
// For "time to reach 99%": speed = 4.605 / lagTime (since ln(100) ~= 4.605)

float speed = 1.0f / lagTimeSec;
float alpha = 1.0f - expf(-speed * deltaTime);
```

## Algorithm

### Two Slew Modes

The bus system supports two slew processor types:

| Operator | Behavior | Character |
|----------|----------|-----------|
| `BUS_OP_SLEW_EXP` | Exponential lag (one-pole lowpass) | Smooth, asymptotic approach. Never quite reaches target. Natural feel. |
| `BUS_OP_SLEW_LINEAR` | Hard slew rate clamp | Constant rate of change. Reaches target exactly. Mechanical feel. |

Exponential lag is the default and more commonly useful. Linear slew is available for cases where constant-rate motion is desired (e.g., sweeping a parameter at fixed speed regardless of jump size).

### Integration as Bus Processor

Like envelopes, slew processors are 1-input bus types. `inputB` is ignored.

```
BUS_OP_SLEW_EXP = 9,      // exponential lag
BUS_OP_SLEW_LINEAR = 10,  // hard slew clamp
```

### Per-Frame Update

**Exponential mode:**
1. Read input: `v = sources->values[bus.inputA]`
2. Compute alpha: `alpha = 1.0f - expf(-speed * deltaTime)` where `speed = 1.0f / lagTime`
3. Update: `state.output += alpha * (v - state.output)`
4. Write `state.output` to bus output slot

**Linear mode:**
1. Read input: `v = sources->values[bus.inputA]`
2. Compute max change: `maxDelta = slewRate * deltaTime`
3. Clamp: `delta = clamp(v - state.output, -maxDelta, maxDelta)`
4. Update: `state.output += delta`
5. Write `state.output` to bus output slot

### Asymmetric Option

Both modes support optional asymmetric rise/fall rates via separate `riseTime`/`fallTime` parameters. When asymmetric mode is disabled, both use `lagTime`.

**Exponential asymmetric:**
```c
float speed = (v > state.output) ? (1.0f / riseTime) : (1.0f / fallTime);
float alpha = 1.0f - expf(-speed * deltaTime);
state.output += alpha * (v - state.output);
```

This is the same conditional branch pattern as the envelope follower, making asymmetric slew and envelope following closely related. The difference: envelope follower takes `fabsf(input)` first (rectifies), slew passes the signal through with polarity intact.

### Runtime State

```c
struct BusSlewState {
    float output;  // current smoothed value
};
```

Minimal state -- just the last output value.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| lagTime | float | 0.01-5.0 | 0.2 | Smoothing time in seconds (exponential: 63% time constant; linear: time to traverse full range) |
| riseTime | float | 0.01-5.0 | 0.2 | Rise smoothing time when asymmetric (overrides lagTime for rising input) |
| fallTime | float | 0.01-5.0 | 0.2 | Fall smoothing time when asymmetric (overrides lagTime for falling input) |
| asymmetric | bool | - | false | Use separate rise/fall times instead of single lagTime |

## Modulation Candidates

- **lagTime**: Faster smoothing during high-energy sections, slower during ambient passages

### Interaction Patterns

**Competing forces with envelope follower**: Chain a slew bus after an envelope bus. The envelope creates sharp attack/slow release dynamics; the slew softens the attack edge. The two processors push in opposite directions on transients -- the envelope wants to jump, the slew wants to glide. The balance between envelope attack time and slew lag time controls whether transients feel punchy or fluid.

## Notes

- One `expf` call per active slew bus per frame -- negligible
- Exponential slew never exactly reaches target (asymptotic). At 5x the time constant, it's within 0.7% -- close enough for visual parameters
- Linear slew reaches target exactly but produces a visible speed discontinuity at arrival (constant motion then sudden stop)
- Slew preserves signal polarity (unlike envelope follower which rectifies). A bipolar LFO through exponential slew stays bipolar but with rounded transitions
- Initial `state.output` is 0.0 on load. First frame may produce a visible jump from 0 to the source's current value. Could initialize to source value on first frame if this is noticeable
