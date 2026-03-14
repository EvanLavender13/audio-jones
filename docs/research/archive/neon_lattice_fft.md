# Neon Lattice FFT Reactivity

Add FFT audio reactivity to the Neon Lattice raymarched generator so that each grid cell pulses with the energy of a frequency band determined by its gradient position.

## Classification

- **Category**: GENERATORS > Geometric (existing effect enhancement)
- **Pipeline Position**: Unchanged — generator renders to scratch texture, blend compositor composites onto chain

## Algorithm

Neon Lattice already computes a per-cell value `t = fract(f.x + f.y + axisOffset)` that samples the gradient LUT for color. The same `t` value drives FFT frequency lookup:

1. In `map()`, after each `geo()` call returns cell coordinates `f`, compute `t = fract(f.x + f.y + axisOffset)`
2. Convert `t` to a frequency: `freq = baseFreq * pow(maxFreq / baseFreq, t)`
3. Convert to FFT bin: `bin = freq / (sampleRate * 0.5)`
4. Sample: `energy = texture(fftTexture, vec2(bin, 0.5)).r`
5. Compute magnitude: `mag = pow(clamp(energy * gain, 0.0, 1.0), curve)`
6. Scale glow color: `color * (baseBright + mag)`

Single-bin sampling (no band averaging) — each cell is a point sample from the gradient, so a point sample from the FFT is consistent.

### Changes needed

**Shader (`neon_lattice.fs`):**
- Add uniforms: `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`
- In `map()`, after each `geo()` call, compute `t` from `f`, sample FFT, multiply glow by `(baseBright + mag)`
- The `getLight()` call already returns `color / (1 + |sdf * attenuation|^glowExponent)` — multiply result by `(baseBright + mag)` before accumulating into `col`

**Header (`neon_lattice.h`):**
- Add standard FFT config fields to `NeonLatticeConfig`: `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`
- Add to `NeonLatticeEffect`: `fftTextureLoc`, `sampleRateLoc`, `baseFreqLoc`, `maxFreqLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc`
- Add `NEON_LATTICE_CONFIG_FIELDS` entries for the new fields

**Source (`neon_lattice.cpp`):**
- Cache new uniform locations in init
- Bind FFT texture and new uniforms in setup
- Change setup signature to accept `Texture2D fftTexture`
- Add Audio section to colocated UI with standard slider order
- Register new params with modulation engine

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest mapped frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest mapped frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on magnitude |
| baseBright | float | 0.0-1.0 | 0.15 | Baseline brightness when silent |

## Modulation Candidates

- **gain**: Scales overall audio sensitivity
- **curve**: Low values compress dynamics (everything glows), high values gate to loud moments only
- **baseBright**: Controls silent-state visibility
- **baseFreq / maxFreq**: Shifts which part of the spectrum is mapped

### Interaction Patterns

- **Cascading threshold (curve × baseBright)**: High curve with low baseBright means cells only light up on strong hits — quiet passages go nearly dark, loud passages explode. Low curve with high baseBright keeps everything visible with subtle pulsing.

## Notes

- Single-bin sampling is adequate here because the cell hash produces pseudo-random `t` values — there's no spatial ordering that would benefit from band averaging
- The three axis offsets (+0.0, +0.33, +0.66) mean each axis emphasizes a different third of the spectrum, which should create nice spatial separation in the audio response
- No performance concern — one extra texture sample per `getLight()` call (3 per raymarch step max) is negligible
