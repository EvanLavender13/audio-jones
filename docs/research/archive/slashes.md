# Slashes

Chaotic thin rectangular bars scattered across the screen, one per semitone. Each tick, every bar re-rolls to a new random position and rotation via PCG hash. FFT magnitude gates visibility and drives width — loud notes snap open as bright slashes, quiet notes vanish. A sharp-attack/fast-decay envelope gives each tick a punchy, staccato flash. During sparse passages only a few bars appear; during dense chords the screen fills with overlapping geometry.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, blended onto scene alongside other generators
- **Chosen Approach**: Full — snap-shut envelope is core to the punchy feel; extra parameters (tick rate, scatter, thickness variation) are just uniforms with no structural complexity. Single-pass fragment shader.

## References

- [Inigo Quilez - 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) — sdBox SDF, smoothstep anti-aliasing
- [Hash Functions for GPU Rendering (Jarzynski & Olano)](https://jcgt.org/published/0009/03/02/paper.pdf) — PCG3D hash: deterministic, fast, high quality
- [GitHub - riccardoscalco/glsl-pcg-prng](https://github.com/riccardoscalco/glsl-pcg-prng) — GLSL PCG3D implementation
- [Suricrasia Online - Useful Shader Functions](https://suricrasia.online/blog/shader-functions/) — erot (Rodrigues rotation), compact GLSL patterns

## Algorithm

### SDF Box

Standard 2D box signed distance (IQ):

```glsl
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}
```

`b.x` = bar half-width (driven by FFT + envelope), `b.y` = bar half-thickness.

### PCG3D Hash

Deterministic pseudo-random from 3D integer coordinates (Jarzynski & Olano):

```glsl
uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    return v;
}
```

Seeded with `(semitoneIndex, tickIndex, tickIndex)` so each note gets a new random position/rotation every tick.

### Per-Bar Logic (pseudocode)

For each semitone `i` in `0..numOctaves*12`:

1. **Hash**: `rnd = pcg3d(i, floor(tickAccum), floor(tickAccum))` normalized to 0-1
2. **FFT lookup**: `freq = baseFreq * pow(2, i/12)`, `bin = freq / nyquist`, `mag = fftTexture(bin) * gain ^ curve`
3. **Envelope**: `env = exp(-pow(fract(tickAccum), envelopeSharp))` — peaks at tick boundary, decays within tick
4. **Bar geometry**:
   - Rotate UV by `rnd.x * TAU` (random angle)
   - Offset by `rnd.yz * scatter` (random position)
   - Width = `maxBarWidth * env * mag`
   - Thickness = `barThickness + rnd.x * thicknessVariation * barThickness`
5. **SDF distance**: `d = sdBox(transformedUV, vec2(width, thickness))`
6. **Glow**: `smoothstep(glowSoftness, 0.0, d)` — soft-edged bar mask
7. **Color**: gradient LUT sampled at `fract(i / 12.0)` (pitch class)
8. **Accumulate**: additive blend `result += color * glow * (baseBright + mag)`

Final output through `tanh()` tone mapping (same as filaments).

### Tick System

`tickAccum` accumulates on CPU: `tickAccum += tickRate * deltaTime`. Passed as uniform. `floor(tickAccum)` gives discrete tick index for hash re-rolling. `fract(tickAccum)` gives position within tick for envelope.

### Envelope Curve

`exp(-pow(fract(tickAccum), envelopeSharp))`:
- At tick start (`fract = 0`): envelope = 1.0 (full open)
- Sharp = 4: snappy VJ-style flash, collapses within ~25% of tick
- Sharp = 1: gentle linear-ish fade
- Sharp = 10: near-instantaneous flash, almost a strobe

### Depth Illusion (optional erot)

Instead of pure 2D rotation, rotate the bar's UV through a 3D axis using Rodrigues' formula, then project back to 2D. Bars tilted away from viewer appear foreshortened — thinner and slightly compressed. Controlled by `rotationDepth` parameter (0 = pure 2D, 1 = full 3D scatter).

```glsl
vec3 erot(vec3 p, vec3 ax, float t) {
    return mix(dot(ax, p) * ax, p, cos(t)) + cross(ax, p) * sin(t);
}
```

Rotation axis randomized per bar from hash. Applied only when `rotationDepth > 0`.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20-2000 | 55.0 | Lowest mapped frequency (Hz) |
| numOctaves | int | 1-8 | 4 | Octave count; total bars = octaves * 12 |
| gain | float | 0.1-20.0 | 5.0 | FFT magnitude amplification |
| curve | float | 0.1-5.0 | 1.0 | Magnitude contrast shaping |
| tickRate | float | 0.5-20.0 | 4.0 | Re-roll rate (ticks/second) |
| envelopeSharp | float | 1.0-10.0 | 4.0 | Envelope decay sharpness |
| maxBarWidth | float | 0.1-1.5 | 0.7 | Maximum bar half-width at full magnitude |
| barThickness | float | 0.001-0.05 | 0.005 | Bar half-thickness baseline |
| thicknessVariation | float | 0.0-1.0 | 0.5 | Random thickness spread per bar |
| scatter | float | 0.0-1.0 | 0.5 | Position offset range from center |
| glowSoftness | float | 0.001-0.05 | 0.01 | Edge anti-aliasing width |
| baseBright | float | 0.0-0.5 | 0.05 | Minimum brightness for visible notes |
| rotationDepth | float | 0.0-1.0 | 0.0 | 3D foreshortening amount (0 = flat) |

## Modulation Candidates

- **tickRate**: changes re-roll speed — slow = lingering bars, fast = strobing chaos
- **envelopeSharp**: softer = sustained glow, sharper = percussive flash
- **scatter**: low = clustered center burst, high = full-screen scatter
- **maxBarWidth**: controls how dramatic each flash reads
- **barThickness**: hairlines vs bold strokes
- **rotationDepth**: flat graphic vs perspective depth

## Notes

- Loop count = `numOctaves * 12`. At 4 octaves (48 iterations) with one SDF eval each, performance is comfortable. 8 octaves (96) may need profiling on integrated GPUs.
- The original shader uses `min(d, dd)` to composite bars (closest-wins). This gives sharp non-overlapping silhouettes. Additive accumulation (like filaments) gives a different look — overlapping bars brighten. Both are valid; additive matches the existing generator family better.
- `tanh()` tone mapping prevents blowout from additive accumulation during dense passages.
- Spatial jitter via `pcg3d` with `floatBitsToUint` in the original shader — AudioJones targets GLSL 330 which supports this.
