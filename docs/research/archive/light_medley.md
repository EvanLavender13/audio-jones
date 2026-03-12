# Light Medley

Fly-through raymarcher rendering a warped crystalline lattice. A rigid repeating grid is distorted by coordinate swirling and depth-dependent twisting, producing organic tunnel-like structures. Inverse-distance glow accumulation lights surfaces — each march step maps to a frequency band, so depth encodes pitch: near surfaces react to bass, deep surfaces react to treble. The visual ranges from geometric and angular at low distortion to fluid and biological at high distortion.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (before transforms)

## Attribution

- **Based on**: "Light Medley [353]" by diatribes (golfed by @bug)
- **Source**: https://www.shadertoy.com/view/3cXBWB
- **License**: CC BY-NC-SA 3.0

## Reference Code

```glsl
// Light Medley [353] — diatribes (golfed by @bug)
// https://www.shadertoy.com/view/3cXBWB

void mainImage(out vec4 o, vec2 u) {
    float i,s,d=.4,t=iTime;
    vec3  c,p = iResolution;
    for(
       u = (u+u-p.xy)/p.y+cos(t*vec2(.6,.3))*vec2(.8,.7)
     ; i++ < 64.
     ; c += 1./s
         + .1*vec3(1,2,6)/length(sin(t)*u+u+vec2(sin(t+t),-.3))
    )
        p = vec3(u * d, d+t)-3.,
        p += cos(.1*t+p.yzx*.3)*3.,
        p.xy *= mat2(cos(.1*p.z+vec4(0,33,11,0))),
        d += s = max(cos(p.x), dot(abs(fract(p)-.5), p-p+.25));
    o.rgb = tanh(c*c/2e6);
}
```

## Algorithm

### De-golfed reference

The shader is heavily code-golfed. Here is the equivalent readable form with substitution notes for AudioJones adaptation:

```
// UV: centered, aspect-corrected, with Lissajous camera pan
uv = (fragCoord * 2.0 - resolution) / resolution.y
uv += cos(time * vec2(panFreqX, panFreqY)) * vec2(panAmpX, panAmpY)

d = 0.4  // initial march distance
c = vec3(0)  // color accumulator

for i in 0..maxSteps:
    // Build 3D sample point: XY from UV * depth, Z advances with depth + time
    p = vec3(uv * d, d + time * flySpeed) - 3.0

    // Coordinate swirl: each axis displaced by cosine of another axis (yzx permutation)
    p += cos(swirlTimeRate * time + p.yzx * swirlSpatialRate) * swirlAmplitude

    // Z-dependent XY rotation (33 ≈ π/2 mod 2π, 11 ≈ 3π/2 mod 2π → proper rotation matrix)
    p.xy *= mat2(cos(twistRate * p.z + vec4(0, 33, 11, 0)))

    // Distance function: max of cosine slabs and octahedral lattice
    s = max(cos(p.x * slabFreq), dot(abs(fract(p * latticeScale) - 0.5), vec3(0.25)))
    d += s

    // Color accumulation: gradient color × FFT brightness × inverse distance
    // (point light term from original is dropped entirely)
    t = float(i) / float(MAX_STEPS)
    freq = baseFreq * pow(maxFreq / baseFreq, t)
    mag = bandSample(fftTexture, freq, nextFreq, sampleRate, gain, curve)
    brightness = baseBright + mag
    color = texture(gradientLUT, vec2(t, 0.5)).rgb
    c += color * brightness / s
```

### Substitutions from reference → AudioJones

| Reference | Replace with | Notes |
|-----------|-------------|-------|
| `iTime` | `time` uniform (CPU-accumulated) | Standard generator pattern |
| `iResolution` | `resolution` uniform | Standard |
| `1/s` + `vec3(1,2,6)/length(...)` | Drop the point light entirely. Replace `1/s` with gradient + FFT per step | See FFT integration below |
| Hardcoded `3.0` swirl amplitude | `swirlAmount` uniform | Key modulation param |
| Hardcoded `0.1` twist/swirl rates | `twistRate`, `swirlRate` uniforms | |
| Hardcoded `64` steps | `MAX_STEPS` define or uniform | Could be fixed at 64 for perf |
| Hardcoded camera pan values | `panFreqX/Y`, `panAmpX/Y` uniforms | Lissajous drift |
| `tanh(c*c/2e6)` tonemap | Keep or use `1.0 - exp(-c * exposure)` | tanh works well here |

### Keep verbatim
- The `mat2(cos(rate * p.z + vec4(0, 33, 11, 0)))` rotation trick — the magic numbers 33 and 11 are integral to the rotation matrix construction
- The `max(cos(p.x), dot(abs(fract(p)-0.5), vec3(0.25)))` distance function
- The `p.yzx` permutation in the swirl term
- The `vec3(uv * d, d + t)` ray construction

### FFT + Gradient LUT integration

Each march step maps to a frequency band via log spread. The step's normalized position `t = i / MAX_STEPS` serves double duty:
- Samples `gradientLUT` at `t` for color
- Maps to a frequency band between `baseFreq` and `maxFreq` for FFT magnitude

The original's two color terms (`1/s` white glow + `vec3(1,2,6)` point light) are both replaced by a single term: `gradientColor * brightness / s`, where brightness comes from the FFT band at that step's frequency.

```glsl
// Per-step FFT band sampling (standard BAND_SAMPLES pattern)
float t0 = float(i) / float(MAX_STEPS);
float t1 = float(i + 1) / float(MAX_STEPS);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float mag = 0.0;
const int BAND_SAMPLES = 4;
for (int b = 0; b < BAND_SAMPLES; b++) {
    float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0)
        mag += texture(fftTexture, vec2(bin, 0.5)).r;
}
mag = pow(clamp(mag / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;

vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;
c += color * brightness / s;
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5–440.0 | 55.0 | Low end of frequency mapping across march depth |
| maxFreq | float | 1000–16000 | 14000.0 | High end of frequency mapping across march depth |
| gain | float | 0.1–10.0 | 2.0 | FFT magnitude scale |
| curve | float | 0.1–3.0 | 1.5 | FFT magnitude contrast curve |
| baseBright | float | 0.0–1.0 | 0.15 | Minimum glow when no audio present |
| swirlAmount | float | 0.0–10.0 | 3.0 | Coordinate displacement amplitude. Low = geometric grid, high = organic blobs |
| swirlRate | float | 0.1–2.0 | 0.3 | Spatial frequency of the swirl displacement |
| swirlTimeRate | float | 0.01–0.5 | 0.1 | How fast the swirl pattern evolves |
| twistRate | float | 0.01–0.5 | 0.1 | How quickly XY rotates with Z depth |
| flySpeed | float | 0.1–3.0 | 1.0 | Forward motion speed through the lattice |
| slabFreq | float | 0.5–4.0 | 1.0 | Frequency of the cosine slab planes |
| latticeScale | float | 0.5–4.0 | 1.0 | Cell density of the octahedral lattice |
| panAmpX | float | 0.0–2.0 | 0.8 | Camera drift amplitude (horizontal) |
| panAmpY | float | 0.0–2.0 | 0.7 | Camera drift amplitude (vertical) |
| panFreqX | float | 0.1–2.0 | 0.6 | Camera drift frequency (horizontal) |
| panFreqY | float | 0.1–2.0 | 0.3 | Camera drift frequency (vertical) |
| glowIntensity | float | 0.1–5.0 | 1.0 | Brightness of inverse-distance glow |
| exposure | float | 0.1–5.0 | 1.0 | Tonemap exposure control |

## Modulation Candidates

- **gain**: Controls how strongly the lattice reacts to audio
- **baseBright**: Floor brightness — high values make the lattice visible even in silence
- **swirlAmount**: The primary visual axis — modulating this shifts the entire look from crystalline to molten
- **flySpeed**: Forward acceleration through the structure
- **twistRate**: Corkscrew intensity, changes corridor feel
- **glowIntensity**: Surface brightness, useful for beat-synced flashes
- **panAmpX/Y**: Camera drift scale, subtle but changes framing
- **slabFreq / latticeScale**: Geometry density, slower modulation works best

### Interaction Patterns

**swirlAmount × latticeScale (competing forces):** High swirl smears cells together while high lattice scale multiplies them. At certain ratios the lattice fights back against the distortion, creating a shimmering instability where structure keeps trying to resolve but can't.

**flySpeed × twistRate (resonance):** When forward speed and twist rate align, the camera corkscrews through aligned corridors. When misaligned, the view tumbles chaotically. Different audio bands driving each create alternating locked/unlocked states.

## Notes

- 64 raymarch steps is moderate — should run fine at full resolution. If perf is tight, 48 steps still looks good.
- The `mat2(cos(... + vec4(0,33,11,0)))` trick depends on `33 mod 2π ≈ π/2` and `11 mod 2π ≈ 3π/2`. These are not tunable — changing them breaks the rotation matrix.
- Initial march distance `d = 0.4` prevents sampling behind the camera. Could be a param but low value.
- The `- 3.0` offset in ray construction shifts the scene origin. Not worth parameterizing.
