# LFO Phase Offset

Per-LFO phase offset that shifts the waveform's starting position in its cycle. Enables quadrature relationships (90-degree offset between two LFOs at the same rate), staggered animation timing, and wider stereo-like modulation patterns without requiring phase sync or coupling.

## Classification

- **Category**: General (modulation engine, LFO subsystem)
- **Pipeline Position**: Applied during LFO waveform evaluation, before output reaches ModSources

## References

No external references needed -- phase offset is a fundamental oscillator parameter. The implementation is a single addition in the existing `GenerateWaveform` call.

## Algorithm

### Change Summary

Add a `phaseOffset` field to `LFOConfig`. During waveform evaluation, add the offset to the current phase (wrapping to [0, 1]) before passing to `GenerateWaveform`.

### Modified Processing in LFOProcess

Current code evaluates:
```c
state->currentOutput = GenerateWaveform(
    config->waveform, state->phase, &state->heldValue, &state->prevHeldValue);
```

With phase offset:
```c
float evalPhase = state->phase + config->phaseOffset;
evalPhase -= floorf(evalPhase);  // wrap to [0, 1]
state->currentOutput = GenerateWaveform(
    config->waveform, evalPhase, &state->heldValue, &state->prevHeldValue);
```

The internal `state->phase` accumulator is NOT modified -- the offset is applied only at evaluation time. This means:
- Phase wrapping and random value generation still trigger at the same absolute times
- Changing the offset mid-run shifts the waveform instantly without resetting the cycle
- Two LFOs at the same rate with different offsets stay at a constant phase relationship (they drift only if rates differ, which is the expected free-running behavior)

### Storage Convention

`phaseOffset` is stored in radians internally (0 to TWO_PI), displayed in degrees (0 to 360) in the UI via `SliderAngleDeg()`. The conversion to [0, 1] phase space for evaluation:

```c
float evalPhase = state->phase + config->phaseOffset / TAU;
```

Alternatively, store as normalized [0, 1] directly and display as degrees via a custom slider. Either works -- radians is consistent with the codebase's other angular fields.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| phaseOffset | float | 0.0-6.283 | 0.0 | Phase offset in radians (displayed as 0-360 deg) |

## Modulation Candidates

- **phaseOffset**: Modulating phase offset creates frequency modulation effects (FM). Slow modulation of offset produces vibrato-like wavering. Fast modulation produces chaotic signals. Registering this as a modulatable parameter is optional -- it may be more confusing than useful for most users.

## Notes

- Zero implementation risk -- single line change in `LFOProcess`, one field addition to `LFOConfig`
- No impact on sample-and-hold or smooth-random waveforms: those use `state->phase` for trigger timing (unchanged) and the offset shifts only the evaluation point. For S&H this means the held value changes at the same time but the "read" position shifts -- effectively no visible change since S&H output is constant between triggers
- The LFO preview waveform in the UI (`LFOEvaluateWaveform`) should also apply the offset so the preview matches the actual output
- Phase offset is distinct from phase sync: two LFOs at slightly different rates will still drift apart. The offset just sets the initial relationship
- Add to `LFO_CONFIG_FIELDS` macro for serialization
- Register as `"lfo<N>.phaseOffset"` with bounds [0, TWO_PI] alongside the existing `"lfo<N>.rate"` registration
