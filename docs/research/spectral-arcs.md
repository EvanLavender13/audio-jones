# Spectral Arcs

Concentric ring arcs driven by FFT frequency data — one arc per semitone across 8 octaves (~96 rings). Each arc's angular sweep and brightness scale with its semitone's energy, and every arc rotates at a unique speed so adjacent rings shimmer and drift apart. Pitch-class coloring via gradient LUT. Visually inspired by XorDev's "Cosmic" shader.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage — renders to `generatorScratch`, blended via `BlendCompositor`
- **Chosen Approach**: Balanced — SDF ring distance with smoothstep angular masking. Amplitude drives both arc sweep width and glow brightness. Same loop cost as the minimal Cosmic port but delivers the core concept (louder = brighter + wider).

## References

- [Inigo Quilez — 2D SDF Functions](https://iquilezles.org/articles/distfunctions2d/) — sdArc, ring SDF, and distance-based glow techniques
- [Inspirnathan — Glow Shader in Shadertoy](https://inspirnathan.com/posts/65-glow-shader-in-shadertoy/) — Inverse-distance glow formula: `intensity / (d + epsilon)`
- [dlbeer — A Better FFT Visualization](https://dlbeer.co.nz/articles/fftvis.html) — Logarithmic bin mapping for perceptual frequency scaling
- XorDev "Cosmic" (Shadertoy) — Per-ring rotation via `sin(i*i)`, angular clipping via `cos(atan(...))`, inverse-distance ring glow. User-provided source code.

## Algorithm

### Coordinate setup

Convert pixel to centered UV with aspect correction. Compute `radius` and `angle` once outside the loop.

### Ring layout

96 rings (12 semitones x `numOctaves`) spaced linearly between `innerRadius` and `outerRadius`:

```
ringSpacing = (outerRadius - innerRadius) / totalRings
r_i = innerRadius + i * ringSpacing
```

### FFT-to-semitone mapping

Same technique as `pitch_spiral.fs`. Convert semitone index to frequency, then to normalized FFT bin:

```
freq_i = baseFreq * pow(2.0, i / 12.0)
bin_i  = freq_i / (sampleRate * 0.5)
mag    = texture(fftTexture, vec2(bin_i, 0.5)).r
mag    = pow(clamp(mag * gain, 0, 1), curve)
```

### Per-arc rotation

Each arc gets a unique rotation offset that evolves over time. Borrowed from the Cosmic shader's `iTime * sin(i*i) + i*i` pattern:

```
rot_i = time * rotationSpeed * sin(i*i + seed) + i*i
```

`sin(i*i)` produces a pseudo-random speed distribution in [-1, 1]. The `i*i` static offset staggers starting angles. `seed` parameter shifts the entire pattern.

### Angular masking

Compute the pixel's angular position on ring `i`, wrapped to [-PI, PI]:

```
a = mod(angle + rot_i + PI, TAU) - PI
```

Arc extent scales with magnitude: `extent = mag * maxSweep`. The mask uses smoothstep:

```
arcMask = smoothstep(extent + blur, extent - blur, abs(a))
```

At zero magnitude the arc vanishes. At full magnitude it sweeps up to `maxSweep` radians.

### Glow

Inverse-distance falloff from the ring radius, clamped to prevent blowout:

```
glow = clamp(glowIntensity / (abs(radius - r_i) * glowFalloff + epsilon), 0, maxBright)
```

`glowFalloff` controls how fast brightness drops away from the ring. `epsilon` prevents division by zero.

### Color

Gradient LUT indexed by pitch class (fractional position within the octave):

```
pitchClass = fract(i / 12.0)
color = texture(gradientLUT, vec2(pitchClass, 0.5))
```

### Accumulation

Each ring's contribution is additive:

```
result += color * mag * arcMask * glow
```

The loop runs all 96 rings per pixel. `radius`, `angle`, and `atan` compute once outside the loop.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20-880 Hz | 220.0 | Lowest visible frequency (A3 default) |
| numOctaves | int | 1-10 | 8 | Number of octave groups (x12 = total rings) |
| innerRadius | float | 0.05-0.4 | 0.10 | Radius of innermost ring |
| outerRadius | float | 0.3-0.8 | 0.60 | Radius of outermost ring |
| ringWidth | float | 0.001-0.02 | 0.005 | Visual thickness reference (affects glow perceived width) |
| glowIntensity | float | 0.001-0.1 | 0.01 | Glow brightness at ring center |
| glowFalloff | float | 10-500 | 100.0 | How fast glow drops with distance from ring |
| gain | float | 0.1-20 | 5.0 | FFT magnitude amplifier |
| curve | float | 0.5-4.0 | 2.0 | Contrast exponent on magnitude |
| maxSweep | float | 0.1-6.28 (TAU) | 3.14 (PI) | Maximum arc angular extent at full amplitude |
| rotationSpeed | float | 0-5.0 | 1.0 | Global multiplier on per-arc rotation rates |
| seed | float | 0-100 | 0.0 | Shifts the rotation pattern |
| blur | float | 0.001-0.1 | 0.02 | Arc edge softness (smoothstep width) |
| gradient | ColorConfig | — | gradient mode | Pitch-class coloring via LUT |
| blendMode | EffectBlendMode | — | SCREEN | Blend compositing mode |
| blendIntensity | float | 0-1 | 1.0 | Blend opacity |

## Modulation Candidates

- **gain**: Pumps overall arc visibility with audio energy
- **glowIntensity**: Breathing glow size
- **rotationSpeed**: Speeds up / slows down the spinning
- **maxSweep**: Widens/narrows all arcs simultaneously
- **innerRadius / outerRadius**: Expands or contracts the ring field
- **blur**: Sharpens or softens arc edges
- **seed**: Slow drift creates evolving rotation patterns

## Notes

- 96 iterations per pixel is within budget — the Cosmic shader runs 30, and each iteration here is comparably cheap (one texture lookup, a few multiplies, smoothstep). Profile if `numOctaves` > 8.
- Low-frequency semitones (below ~80 Hz) map to FFT bins with poor resolution. The pitch spiral has the same limitation. `gain` and `curve` compensate by amplifying weak signals.
- Ring spacing at defaults: `0.5 / 96 ≈ 0.005`. Adjacent ring glow overlaps, creating a continuous luminous field when many notes are active. Reducing `glowFalloff` separates them visually.
- The `sin(i*i)` rotation distribution is deterministic — same pattern every session for a given `seed`. This is intentional: reproducible presets.
