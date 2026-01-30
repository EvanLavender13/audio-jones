# Interference Warp

Displaces UVs along a multi-axis harmonic interference field. Summed sine waves across configurable axes create lattice-like distortion patterns (triangular, rectangular, quasicrystal) that evolve as harmonics drift in and out of phase.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Transform stage (user-ordered with other transforms)

## References

- User-provided Shadertoy "Interference Frenzy" - Fourier harmonic summation with three-axis sine interference

## Algorithm

### Core Displacement Function

The warp generates a 2D displacement vector by summing harmonics across multiple interference axes.

**Generalized multi-axis interference:**

```glsl
float psi(float k, vec2 s, int numAxes, float axisRotation) {
    float sum = 0.0;
    for (int i = 0; i < numAxes; i++) {
        float angle = axisRotation + float(i) * PI / float(numAxes);
        vec2 dir = vec2(cos(angle), sin(angle));
        sum += sin(k * dot(s, dir));
    }
    return sum;
}
```

**Time-varying phase rotation per harmonic:**

```glsl
vec2 omega(float k, float t, float driftExp) {
    float p = pow(k, driftExp) * t;
    return vec2(sin(p), cos(p));
}
```

- `driftExp = 2.0`: Higher harmonics rotate faster (original behavior)
- `driftExp = 1.0`: Linear drift, all harmonics move together
- `driftExp > 2.0`: Rapid high-frequency chaos

**Harmonic summation with amplitude decay:**

```glsl
vec2 phi(vec2 s, float t, int harmonics, int axes, float axisRot, float decay, float drift) {
    vec2 p = vec2(0.0);
    for (int j = 1; j < harmonics; j++) {
        float k = float(j) * TAU;
        float amp = pow(k, -decay);  // 1/k^decay falloff
        p += omega(k, t, drift) * psi(k, s, axes, axisRot) * amp;
    }
    return p;
}
```

**Final displacement:**

```glsl
vec2 displacedUV = uv + phi(uv * scale, time * speed, ...) * amplitude;
```

### Axis Count Effects

| Axes | Pattern | Visual Character |
|------|---------|------------------|
| 2 | Rectangular | Grid-aligned waves |
| 3 | Triangular/hexagonal | Honeycomb lattice |
| 4 | Square + diagonal | Diamond grid |
| 5 | Penrose-like | Quasicrystal, aperiodic |
| 6 | Hexagonal dense | Snowflake symmetry |
| 7+ | Complex quasicrystal | Increasingly intricate |

### Performance Considerations

- **Harmonic count** directly impacts performance: each harmonic requires `numAxes` sine evaluations
- Cost: `O(harmonics × axes)` trig operations per pixel
- Recommended ranges: 16-128 harmonics, 2-6 axes
- Full 256 harmonics × 6 axes = 1536 sine calls per pixel (heavy)

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| amplitude | float | 0.0 - 0.5 | 0.1 | Displacement strength |
| scale | float | 0.5 - 10.0 | 2.0 | Pattern frequency/density |
| axes | int | 2 - 8 | 3 | Lattice symmetry type |
| axisRotation | float | 0 - π | 0.0 | Rotates entire pattern |
| harmonics | int | 8 - 256 | 64 | Detail level (sharpness of lines) |
| decay | float | 0.5 - 2.0 | 1.0 | Amplitude falloff (higher = smoother) |
| speed | float | 0.0 - 2.0 | 0.2 | Time evolution rate |
| drift | float | 1.0 - 3.0 | 2.0 | Harmonic phase drift exponent |

## Modulation Candidates

- **amplitude**: Pulse displacement with beat, creates breathing distortion
- **axisRotation**: Slow rotation creates kaleidoscope-like drift
- **speed**: Audio-reactive time acceleration
- **scale**: Zoom in/out on interference pattern

## Notes

- Higher `decay` values produce smoother, more fluid warps; lower values create sharper interference lines
- `axes = 2` with high harmonics resembles fabric/textile distortion
- `axes = 5` or `7` (prime numbers) create non-repeating quasicrystal patterns
- The time evolution is deterministic - same time value produces same pattern (no randomness)
- Consider offering "quality" preset that adjusts harmonics: Low (16), Medium (64), High (128), Ultra (256)
