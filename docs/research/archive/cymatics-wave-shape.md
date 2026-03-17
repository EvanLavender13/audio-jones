# Cymatics Wave Shape Selection

Add selectable wave shapes (sine, triangle, square, sawtooth) to Ripple Tank's parametric mode and a new parametric mode for Faraday Waves.

## Classification

- **Category**: GENERATORS > Cymatics (enhancement to existing effects)
- **Pipeline Position**: No change — both effects keep their current pipeline position
- **Compute Model**: No change — both use existing ping-pong render textures

## References

- Existing LFO waveform implementation in `src/automation/lfo.cpp` — sine, triangle, sawtooth, square math
- Ripple Tank shader `shaders/ripple_tank.fs` — parametric sine on line 75
- Faraday shader `shaders/faraday.fs` — cosine lattice on line 34, temporal oscillation on line 76

## Reference Code

### LFO waveform generation (CPU, from `src/automation/lfo.cpp`)

```c
// phase is [0, 1), output is [-1, 1]
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
```

### GLSL port of waveform shapes

The LFO shapes operate on a normalized phase `[0,1)`. In the shaders, the input is an unbounded argument (like `dist * waveFreq - time + phase`). Convert to normalized phase with `fract(x / TAU)` where `TAU = 6.283185307`.

```glsl
const float TAU = 6.283185307;

// wave(x, shape): evaluate periodic waveform at arbitrary x
// shape: 0=sine, 1=triangle, 2=sawtooth, 3=square
float wave(float x, int shape) {
    if (shape == 0) return sin(x);                                   // Sine
    float p = fract(x / TAU);                                        // Normalize to [0,1)
    if (shape == 1) return (p < 0.5) ? p * 4.0 - 1.0 : 3.0 - p * 4.0; // Triangle
    if (shape == 2) return p * 2.0 - 1.0;                            // Sawtooth
    return (p < 0.5) ? 1.0 : -1.0;                                  // Square
}
```

### Ripple Tank: current parametric wave (line 75 of `ripple_tank.fs`)

```glsl
return sin(dist * waveFreq - time + phase) * atten;
```

Replace with:

```glsl
return wave(dist * waveFreq - time + phase, waveShape) * atten;
```

Same substitution in the chromatic overload (line 90):

```glsl
return wave(dist * freq - time + phase, waveShape) * atten;
```

### Faraday: current lattice wave (line 34 of `faraday.fs`)

```glsl
h += cos(dot(dir, p) * k);
```

In parametric mode, replace with:

```glsl
h += wave(dot(dir, p) * k, waveShape);
```

In audio mode (current behavior), keep `cos()` — it's the physics.

### Faraday: current temporal oscillation (line 76 of `faraday.fs`)

```glsl
layerHeight *= cos(time * freq * 0.5);
```

In parametric mode, this uses a fixed `waveFreq` instead of FFT-derived `freq`:

```glsl
layerHeight *= wave(time * waveFreq * 0.5, waveShape);
```

In audio mode, keep `cos(time * freq * 0.5)`.

## Algorithm

### Shared GLSL wave function

Both shaders include the same `wave(float x, int shape)` function defined above. The `sin()` path (shape==0) skips the `fract()` normalization for efficiency since `sin()` is already periodic.

### Ripple Tank changes

1. Add `uniform int waveShape;` (0=sine, 1=triangle, 2=sawtooth, 3=square)
2. Replace `sin(...)` with `wave(..., waveShape)` in both `waveFromSource` overloads
3. `waveShape` only has effect when `waveSource == 1` (parametric mode) — in audio mode the waveform comes from the ring buffer, not a function
4. Config: add `int waveShape = 0;` field to `RippleTankConfig`

### Faraday changes

1. Add `uniform int waveSource;` (0=audio, 1=parametric) — same concept as Ripple Tank
2. Add `uniform int waveShape;` (0=sine, 1=triangle, 2=sawtooth, 3=square)
3. Add `uniform float waveFreq;` — spatial frequency for parametric mode
4. In `main()`, branch on `waveSource`:
   - **Audio mode (0)**: current behavior unchanged — FFT energy drives layer amplitudes, `cos()` lattice, `cos()` temporal
   - **Parametric mode (1)**: skip FFT sampling, use uniform amplitude, `wave(..., waveShape)` for lattice, `wave(time * waveFreq * 0.5, waveShape)` for temporal
5. Config: add `int waveSource = 0;`, `int waveShape = 0;`, `float waveFreq = 30.0f;`, `float waveSpeed = 2.0f;` to `FaradayConfig`
6. Faraday parametric mode uses `time` uniform (already exists) for animation. `waveSpeed` accumulates into `time` on CPU like Ripple Tank does.

### UI layout

Both effects show `waveShape` combo only when `waveSource == 1`. Faraday's parametric mode also shows `waveFreq` and `waveSpeed` sliders (hidden in audio mode), mirroring Ripple Tank's existing pattern.

## Parameters

### Ripple Tank (new field)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| waveShape | int  | 0-3   | 0       | Parametric wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square |

### Faraday (new fields)

| Parameter | Type  | Range     | Default | Effect |
|-----------|-------|-----------|---------|--------|
| waveSource | int  | 0-1       | 0       | 0=audio (FFT-driven), 1=parametric |
| waveShape  | int  | 0-3       | 0       | Lattice wave shape: 0=sine, 1=triangle, 2=sawtooth, 3=square |
| waveFreq   | float | 5.0-100.0 | 30.0   | Parametric spatial frequency |
| waveSpeed  | float | 0.0-10.0  | 2.0    | Parametric animation speed |

## Modulation Candidates

- **waveFreq** (both): changes interference spacing — wider ripples when low, tight moiré when high
- **waveSpeed** (both): controls animation rate — freeze at 0, rapid pulse at max
- **visualGain** (existing): interacts with wave shape — square waves clip harder than sine, so gain changes are more dramatic with non-sine shapes
- **spatialScale** (Faraday): combined with non-sine wave shapes, changes the visual density of sharp edges

### Interaction Patterns

- **Wave shape × visual gain (competing forces)**: Square and triangle waves have more harmonic content than sine. At low gain they look similar, but as gain increases the sharp transitions saturate differently — square waves hit hard clipping while sine stays smooth. The gain slider has more visual range with non-sine shapes.
- **Wave shape × waveCount (resonance)**: Non-sine lattice superposition in Faraday creates moiré interference at the harmonic frequencies that sine doesn't have. Triangle with waveCount=3 produces secondary hexagonal subpatterns that only appear because the triangle wave's harmonics create additional interference nodes.

## Notes

- The `wave()` function uses branching on `shape` — this is fine for a per-fragment function with only 4 branches. No performance concern.
- Square waves produce exact ±1 values which may cause hard visual edges. The existing `tanh()` compression in both shaders handles this gracefully.
- Sawtooth waves break the bilateral symmetry of the interference patterns — the result is directional/asymmetric, which is a feature, not a bug.
- The wave shape enum should NOT reuse `LFOWaveform` — it's a shader-side int uniform, and sample & hold / smooth random don't apply to spatial waves. Define as a simple 0-3 int.
