# Tunnel Generator

A raymarched infinite tunnel generator with fully composable parameters. Every feature is a continuous dial that blends with all others—branching + morphing + noise all combine rather than being mutually exclusive modes.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage (after feedback, before transforms)
- **Chosen Approach**: Full configurable playground—all parameters exposed, defaults produce a clean corridor, features layer on via non-zero values

## References

- [Inigo Quilez: Smooth Minimum](https://iquilezles.org/articles/smin/) — blending multiple tunnel branches with `smin()`
- [Inigo Quilez: Domain Warping](https://iquilezles.org/articles/warp/) — turbulence via `p += h(p)` displacement
- [The Book of Shaders: FBM](https://thebookofshaders.com/13/) — octave-based noise layering
- [GM Shaders: Volumetric Raymarching](https://mini.gmshaders.com/p/volumetric) — translucent accumulation technique
- [Paul Bourke: Supershapes](https://paulbourke.net/geometry/supershape/) — circle-to-square morphing via superformula

## Algorithm

### Core Raymarching Loop

Fixed structure that never changes when adding features:

```glsl
for (float i = 0.; i < MAX_STEPS; i++) {
    vec3 p = ro + rd * d;           // current position
    float s = sceneSDF(p);          // evaluate composable SDF
    d += stepSize(s);               // accumulate distance
    color += colorAccum(s, d, p);   // accumulate color
}
color = toneMap(color, d, uv);
```

### Camera Path

Lissajous curve with configurable frequencies and amplitude:

```glsl
vec3 getPath(float z) {
    vec2 freq = vec2(pathFreqX, pathFreqY);
    vec2 xy = pathAmplitude * cos(z * freq);
    xy += helixTwist * vec2(cos(z * helixFreq), sin(z * helixFreq));
    return vec3(xy, z);
}
```

Look-at camera setup:

```glsl
vec3 p = getPath(t * flySpeed);
vec3 Z = normalize(getPath(t * flySpeed + lookahead) - p);  // forward
vec3 X = normalize(vec3(Z.z, 0, -Z.x));                     // right
vec3 Y = cross(X, Z);                                        // up
vec3 rd = normalize(vec3(uv, 1.0)) * mat3(X, Y, Z);
```

### Composable SDF

Every feature is a term that either adds geometry via `min()` or carves detail via subtraction:

```glsl
float sceneSDF(vec3 p) {
    vec3 pathPos = getPath(p.z);

    // === Domain transforms (applied before SDF evaluation) ===
    p += turbulence * cos(p.z + time + p.yzx * turbulenceFreq);  // turbulence
    p.xy *= rot2D(p.z * twistRate + time * twistSpeed);          // twist

    // === Base tunnel (always present) ===
    float radius = tunnelRadius + radiusPulse * sin(p.z * radiusPulseFreq);
    float s = radius - tunnelDist(p.xy - pathPos.xy);

    // === Branching (blends in via smin when branchCount > 1) ===
    for (int i = 1; i < branchCount; i++) {
        vec2 offset = branchSpread * vec2(cos(i * TAU / branchCount), sin(i * TAU / branchCount));
        s = smin(s, radius - tunnelDist(p.xy - pathPos.xy - offset), branchBlend);
    }

    // === Noise layers (each octave subtracts) ===
    float n = 0.;
    float amp = noiseScale;
    float freq = noiseFreq;
    for (int i = 0; i < noiseOctaves; i++) {
        n += amp * abs(dot(sin(time * noiseSpeed + p * freq), vec3(1)));
        freq *= noiseLacunarity;
        amp *= noiseGain;
    }
    s -= n;

    // === Additional features (scale=0 disables) ===
    s -= mengerScale * mengerSDF(p);
    s -= triScale * triNoise(p);

    return s;
}
```

### Cross-Section Shape (Boxiness)

Blend between circular and square cross-section:

```glsl
float tunnelDist(vec2 p) {
    float circle = length(p);
    float box = max(abs(p.x), abs(p.y));
    return mix(circle, box, boxiness);
}
```

For smoother morphing, use superformula:

```glsl
float superDist(vec2 p, float m, float n) {
    float theta = atan(p.y, p.x);
    float r = pow(pow(abs(cos(m * theta / 4.)), n) + pow(abs(sin(m * theta / 4.)), n), -1./n);
    return length(p) / r;
}
// boxiness 0→1 maps n from 2 (circle) to high value (square), m = 4
```

### Translucent Accumulation

Step size based on absolute SDF for volumetric feel:

```glsl
float stepSize(float s) {
    return stepBase + abs(s) * stepScale;
}

vec4 colorAccum(float s, float d, vec3 p) {
    vec3 baseColor = mix(vec3(1), 0.5 + 0.5 * cos(d * depthColorFreq + vec3(0, 2, 4)), depthColorCycle);
    return vec4(baseColor, 1) / s;
}
```

### Tone Mapping

```glsl
vec4 toneMap(vec4 color, float d, vec2 uv) {
    color /= brightness;
    color /= mix(1., d, depthFade);
    color /= mix(1., length(uv), vignetteStrength);
    return tanh(color);
}
```

### Smooth Minimum (for branching)

Quadratic polynomial blend:

```glsl
float smin(float a, float b, float k) {
    k *= 4.0;
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}
```

## Parameters

### Path

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| flySpeed | float | 0–20 | 4.0 | Forward velocity through tunnel |
| pathAmplitude | float | 0–50 | 12.0 | Width of path oscillation |
| pathFreqX | float | 0.01–0.5 | 0.1 | Horizontal oscillation frequency |
| pathFreqY | float | 0.01–0.5 | 0.12 | Vertical oscillation frequency |
| helixTwist | float | 0–10 | 0 | Corkscrew overlay on path |
| helixFreq | float | 0.01–0.5 | 0.05 | Corkscrew rotation rate |
| lookahead | float | 0.5–10 | 4.0 | Camera look-at distance |
| cameraWobble | float | 0–0.5 | 0.1 | Subtle camera drift |

### Tunnel Geometry

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| tunnelRadius | float | 1–20 | 4.0 | Base tunnel size |
| radiusPulse | float | 0–5 | 0 | Breathing modulation depth |
| radiusPulseFreq | float | 0.1–2 | 0.6 | Breathing rate along z |
| boxiness | float | 0–1 | 0 | Round (0) to square (1) cross-section |
| branchCount | int | 1–4 | 1 | Number of parallel tunnels |
| branchSpread | float | 0–20 | 6.0 | Distance between branches |
| branchBlend | float | 0–5 | 1.0 | Smooth blend radius between branches |
| planeBlend | float | 0–1 | 0 | Mix in infinite floor/ceiling |

### Surface Detail

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| noiseOctaves | int | 0–8 | 0 | Noise iterations (0 = smooth) |
| noiseScale | float | 0–2 | 0.3 | How deep noise carves |
| noiseFreq | float | 0.1–32 | 4.0 | Base detail frequency |
| noiseLacunarity | float | 1.5–3 | 2.0 | Frequency multiplier per octave |
| noiseGain | float | 0.3–0.7 | 0.5 | Amplitude decay per octave |
| noiseSpeed | float | 0–2 | 0.3 | Surface animation rate |
| turbulence | float | 0–2 | 0 | Domain warp intensity |
| turbulenceFreq | float | 0.1–2 | 0.5 | Domain warp scale |
| twistRate | float | 0–0.2 | 0 | Rotation accumulation with depth |
| twistSpeed | float | 0–1 | 0.1 | Time-based rotation |

### Rendering

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| maxSteps | int | 32–256 | 100 | Raymarch iterations |
| stepBase | float | 0.001–0.1 | 0.02 | Minimum step size |
| stepScale | float | 0.05–0.5 | 0.1 | Step size multiplier |

### Color

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| brightness | float | 100–100000 | 2000 | Overall exposure |
| depthColorCycle | float | 0–1 | 0 | Grayscale (0) to rainbow (1) |
| depthColorFreq | float | 0.01–0.5 | 0.1 | Rainbow wavelength |
| vignetteStrength | float | 0–2 | 1.0 | Center glow falloff |
| depthFade | float | 0–1 | 0.5 | Distance darkening |

## Modulation Candidates

- **flySpeed**: acceleration on beats
- **tunnelRadius**: breathing/pulse with bass
- **radiusPulse**: reactive tunnel contractions
- **noiseScale**: surface roughness with energy
- **turbulence**: chaos intensity with high frequencies
- **brightness**: flash on transients
- **depthColorCycle**: shift color mode with song sections

## Extensibility

Adding a new feature from a discovered shader:

1. Add uniform parameter with default 0 (disabled)
2. Add term to `sceneSDF()`: `s -= newScale * newFeatureSDF(p);`
3. Add slider to UI
4. Register in param_registry for modulation

The core loop and accumulation logic remain unchanged.

## Notes

- Performance scales with `maxSteps` and `noiseOctaves`
- Consider LOD parameter that reduces both for lower-end GPUs
- `stepBase` prevents infinite loops; too small = slow, too large = artifacts
- Cinema bars (letterbox) can be a simple UI toggle, not shader logic
