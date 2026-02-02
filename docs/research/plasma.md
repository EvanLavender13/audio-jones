# Plasma

Glowing vertical lightning bolts that drift horizontally across the screen. FBM noise displaces the bolt paths to create jagged lightning or smooth plasma tendrils depending on octave count. Bolts persist and animate continuously.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, after trail boost, before transforms (same as Constellation)
- **Chosen Approach**: Balanced - multiple configurable vertical bolts with noise displacement, drift animation, and style control via octaves/falloff

## References

- [The Book of Shaders: Fractal Brownian Motion](https://thebookofshaders.com/13/) - FBM algorithm, octaves, lacunarity, persistence
- [Inigo Quilez: Useful Little Functions](https://iquilezles.org/articles/functions/) - Falloff curves, glow functions
- [Glow Shader in Shadertoy](https://inspirnathan.com/posts/65-glow-shader-in-shadertoy/) - 1/distance glow technique with SDFs
- User-provided Shadertoy lightning shader - core technique: FBM-displaced UV + inverse distance falloff

## Algorithm

### Core Technique (from reference shader)

```glsl
// Displace UV with FBM noise
uv += 2.0 * fbm(uv + time * animSpeed, octaves) - 1.0;

// Distance to vertical line at position x
float dist = abs(uv.x - boltX);

// Inverse distance glow
vec3 col = boltColor * (glowRadius / dist);
```

### FBM Implementation

```glsl
float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p *= rotate2d(0.45);  // Rotation breaks up grid artifacts
        p *= 2.0;             // Lacunarity
        amplitude *= 0.5;     // Persistence
    }
    return value;
}
```

### Multiple Bolts

Each bolt has a base X position evenly distributed across screen width. Per-bolt phase offset creates independent motion:

```glsl
for (int i = 0; i < boltCount; i++) {
    float baseX = (float(i) + 0.5) / float(boltCount) * 2.0 - 1.0;
    float phase = time * driftSpeed + float(i) * 1.618;  // Golden ratio offset
    float boltX = baseX + sin(phase) * driftAmount;

    // Displace and render bolt at boltX
}
```

### Falloff Types

| Type | Formula | Visual |
|------|---------|--------|
| Sharp | `1.0 / (d * d)` | Tight bright core, lightning-like |
| Linear | `1.0 / d` | Balanced glow |
| Soft | `1.0 / sqrt(d)` | Wide hazy plasma |

### Flicker

```glsl
float flicker = mix(1.0, hash(floor(time * 30.0)), flickerAmount);
col *= flicker;
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| boltCount | int | 1-8 | 3 | Number of vertical bolts |
| driftSpeed | float | 0.0-2.0 | 0.5 | Horizontal wandering rate |
| driftAmount | float | 0.0-1.0 | 0.3 | Horizontal wandering distance |
| octaves | int | 1-10 | 6 | Jaggedness: 1-3 = smooth plasma, 6+ = jagged lightning |
| displacement | float | 0.0-2.0 | 1.0 | Path distortion strength |
| animSpeed | float | 0.0-5.0 | 0.8 | Noise animation rate |
| falloffType | int | 0-2 | 1 | 0=Sharp, 1=Linear, 2=Soft |
| glowRadius | float | 0.01-0.3 | 0.07 | Halo width multiplier |
| coreBrightness | float | 0.5-3.0 | 1.5 | Overall intensity |
| flickerAmount | float | 0.0-1.0 | 0.2 | Random intensity jitter (0=smooth, 1=harsh) |

## Modulation Candidates

- **displacement**: Bolts become more chaotic when modulated
- **coreBrightness**: Pulses bolt intensity
- **flickerAmount**: Increases crackling effect
- **animSpeed**: Speeds up noise evolution
- **driftAmount**: Bolts spread wider

## Notes

- Clamp final color or apply tonemap `1.0 - exp(-col)` to avoid harsh clipping when bolts overlap
- Per-bolt color variation possible by sampling gradient LUT with bolt index
- GPU cost scales linearly with boltCount and octaves; 8 bolts at 10 octaves remains fast
