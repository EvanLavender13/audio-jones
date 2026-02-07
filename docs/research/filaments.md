# Filaments

Radial burst of luminous line segments where each filament represents one semitone. Active notes flare bright while quiet semitones persist as dim embers. Triangle-wave fractal noise displaces the UV field so filaments writhe organically along their length. The whole structure rotates slowly. Additive inverse-distance glow accumulates across all filaments, producing a cohesive radial shape with individually traceable strands.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, same slot as pitch spiral / spectral arcs / muons
- **Chosen Approach**: Balanced — captures the original's organic writhing character via triangle-noise displacement plus semitone-mapped FFT glow, without the complexity of cross-connections or length modulation

## References

- [IQ 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) — Exact `sdSegment` line-segment distance function used as the filament primitive
- [Glow Shader Techniques](https://inspirnathan.com/posts/65-glow-shader-in-shadertoy/) — Inverse-distance glow formula (`intensity / distance`) and intensity/falloff control
- [Book of Shaders: fBM](https://thebookofshaders.com/13/) — Fractal octave accumulation pattern for the triangle-wave noise
- [Filaments by nimitz](https://www.shadertoy.com/view/4lcSWs) — Original Shadertoy (user-provided source code). Radial segment burst with triangle-noise distortion and additive glow

## Algorithm

### Filament Geometry

Each semitone index `i` (0 to `numOctaves * 12 - 1`) defines one filament as a line segment from near-center to outer radius:

- Angular position: `angle = (float(i) / float(totalFilaments)) * TAU`
- Inner endpoint: `a = vec2(cos(angle), sin(angle)) * innerRadius`
- Outer endpoint: `b = vec2(cos(angle), sin(angle)) * outerRadius`

Both endpoints rotate over time via `rotationAccum` (same convention as spectral arcs).

### Segment Distance (from IQ)

```
vec2 pa = p - a, ba = b - a;
float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
float dist = length(pa - ba * h);
```

Standard point-to-segment distance. `h` is the normalized position along the segment (0 = inner end, 1 = outer end).

### Triangle-Wave Noise Displacement

Before evaluating segment distances, displace the UV coordinate with nimitz's triangle-wave fBM:

- `tri(x) = abs(fract(x) - 0.5)` — triangle wave, range [0, 0.5]
- `tri2(p) = vec2(tri(p.x + tri(p.y*2)), tri(p.y + tri(p.x*2)))` — 2D coupled triangle waves
- 4 octaves: each iteration rotates the displacement by time, scales UV by ~1.2x with a fixed rotation matrix, accumulates `tri(p.x + tri(p.y)) / z` where `z` grows by lacunarity ~1.7x

The noise output (0–1 range, clamped) offsets the pixel coordinate before segment distance evaluation. A `noiseStrength` uniform scales displacement magnitude.

### Additive Glow

Per filament, accumulate:

```
glow = glowIntensity / (pow(dist, falloffExponent) + epsilon)
color += filamentColor * glow * (baseBright + mag)
```

- `dist` from segment distance function
- `glowIntensity` controls peak brightness
- `falloffExponent` (~1.0–2.0) shapes how quickly glow drops off
- `baseBright` keeps quiet filaments as dim embers
- `mag` is the FFT magnitude for this semitone

### FFT Semitone Lookup

Same pattern as spectral arcs:

```
freq = baseFreq * pow(2.0, float(i) / 12.0)
bin = freq / (sampleRate * 0.5)
mag = texture(fftTexture, vec2(bin, 0.5)).r
mag = pow(clamp(mag * gain, 0.0, 1.0), curve)
```

### Color

Gradient LUT indexed by pitch class: `pitchClass = fract(float(i) / 12.0)`. Same convention as spectral arcs and pitch spiral — same semitone across octaves shares a color.

### HDR Rolloff

`tanh(result * brightness)` for soft clamp (same as muons), preventing blowout from additive accumulation.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| numOctaves | int | 1–8 | 5 | Filament count (octaves × 12 semitones) |
| gain | float | 1.0–20.0 | 5.0 | FFT magnitude amplification |
| curve | float | 0.5–4.0 | 1.5 | Contrast curve on FFT response |
| baseBright | float | 0.0–0.5 | 0.05 | Dim ember level for quiet semitones |
| innerRadius | float | 0.01–0.3 | 0.05 | Filament start distance from center |
| outerRadius | float | 0.3–1.5 | 0.8 | Filament end distance from center |
| glowIntensity | float | 0.001–0.05 | 0.008 | Peak filament brightness |
| falloffExponent | float | 0.8–2.0 | 1.2 | Glow distance falloff sharpness |
| noiseStrength | float | 0.0–1.0 | 0.4 | Triangle-noise displacement magnitude |
| noiseSpeed | float | 0.0–10.0 | 4.5 | Triangle-noise rotation rate |
| rotationSpeed | float | -3.14–3.14 | 0.3 | Radial spin rate (rad/s) |
| brightness | float | 0.1–5.0 | 1.0 | Overall HDR brightness before rolloff |

## Modulation Candidates

- **gain**: Breathing FFT sensitivity — louder/quieter overall response
- **noiseStrength**: Calm vs turbulent filament motion
- **glowIntensity**: Pulsing brightness of the entire structure
- **innerRadius / outerRadius**: Expanding/contracting filament reach
- **rotationSpeed**: Accelerating/decelerating spin

## Notes

- Loop count = `numOctaves * 12` (max 96 at 8 octaves). Triangle-noise has a 4-iteration inner loop but uses only triangle waves (no trig), so total cost per pixel stays well below muons' raymarch loop.
- The `epsilon` in the glow denominator prevents division-by-zero when a pixel sits exactly on a filament.
- `rotationAccum` accumulates on the C++ side (like spectral arcs) — `accum += rotationSpeed * deltaTime`.
