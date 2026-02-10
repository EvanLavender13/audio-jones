# Density Wave Spiral

Warps UV coordinates through concentric, differentially-rotating ellipse rings with progressive tilt, creating logarithmic spiral arm structure reminiscent of galaxy arms. Based on Lin-Shu density wave theory - a visual approximation that sidesteps expensive N-body simulation.

## Classification

- **Category**: TRANSFORMS → Motion
- **Core Operation**: Multi-ring UV warp with differential rotation and progressive tilt
- **Pipeline Position**: Trail boost → Chromatic → **Transforms (user order)** → Clarity → FXAA → Gamma

## References

- [Galaxy Rendering Revisited (dexyfex)](https://dexyfex.com/2016/09/09/galaxy-rendering-revisited/) - Ellipse-based particle placement, incremental rotation creates barred spiral patterns
- [Logarithmic Spiral (Wolfram MathWorld)](https://mathworld.wolfram.com/LogarithmicSpiral.html) - Polar equation r = ae^(bθ), pitch angle formulas
- [Shadertoy 4dcyzX](https://shadertoy.com/view/4dcyzX) - Original implementation by Fabrice Neyret

## Algorithm

### Core Concept

The winding problem: if spiral arms were material, differential rotation would wind them up within a few orbits. Lin-Shu's insight: arms are density waves, not fixed structures. Stars move through the arms like cars through a traffic jam.

For rendering, we approximate this by:
1. Drawing concentric ellipse rings
2. Tilting each ring proportionally to its radius (creates spiral)
3. Rotating inner rings faster than outer rings (differential rotation)
4. Sampling input texture at transformed coordinates

### Spiral Arm Formation

Each ring at radius `l` receives tilt angle `a * l` where `a` controls arm tightness:

```glsl
// Tilt per radius - creates logarithmic spiral
float a = -PI / 2.0;  // Tightness: -π/2 gives ~2 arms

// Transform UV to ellipse coordinates with tilt
vec2 V = (rot(a * l) * uv) / aspect;  // aspect = vec2(0.5, 0.3)
```

### Differential Rotation

Real galaxies have flat rotation curves (inner and outer stars have similar tangential velocity). This means angular velocity scales inversely with radius:

```glsl
// Angular velocity inversely proportional to radius
float va = time * (rotationSpeed / l);

// Sample input texture at rotated coordinates
vec2 sampleUV = rot(va + ringOffset) * 0.5 * V / l;
vec4 color = texture(inputTex, sampleUV);
```

### Ring Blending

Each ring contributes based on distance from its ellipse boundary:

```glsl
float d = dot(V, V);  // Quadratic form for ellipse distance
float ring = smoothstep(thickness, 0.0, abs(sqrt(d) - l));
```

### Complete Loop Structure

```glsl
vec4 result = vec4(0.0);
for (float l = 0.1, n = 1.0; l < 3.0; l += 0.1, n += 1.0) {
    // 1. Apply tilt based on radius
    vec2 V = (rot(a + a * l) * uv) / aspect;

    // 2. Calculate ring mask
    float d = dot(V, V);
    float ring = smoothstep(thickness, 0.0, abs(sqrt(d) - l));

    // 3. Sample texture with differential rotation
    float va = time * (rotationSpeed / l);
    vec2 sampleUV = rot(va + n) * 0.5 * V / l;
    vec4 texColor = texture(inputTex, sampleUV);

    // 4. Accumulate with distance falloff
    result += ring * texColor / l;
}
```

### Noise/Turbulence Enhancement

The Shadertoy versions use multiplicative FBM for dust/nebula appearance:

```glsl
// Multiplicative cascade creates dusty falloff
vec4 turbulence(vec2 uv) {
    uv /= 16.0;
    vec4 t = 1.0 - abs(2.0 * texture(noiseTex, uv) - 1.0);
    return t(uv) * t(2.0*uv)*2.0 * t(4.0*uv)*2.0 * t(8.0*uv)*2.0;
}
```

For AudioJones, the input texture (trails) serves as the turbulence source - no separate noise texture needed.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| center | vec2 | [-1, 1] | (0, 0) | Galaxy center position |
| aspect | vec2 | [0.1, 1.0] | (0.5, 0.3) | Ellipse eccentricity (x < y = barred) |
| tightness | float | [-π, π] | -π/2 | Arm winding: negative = trailing arms |
| rotationSpeed | float | [0, 10] | 0.5 | Differential rotation rate (inner faster) |
| globalRotationSpeed | float | [-10, 10] | 0 | Whole-spiral rotation rate |
| thickness | float | [0.05, 0.5] | 0.3 | Ring softness |
| ringCount | int | [10, 50] | 30 | Number of concentric rings |
| falloff | float | [0.5, 2.0] | 1.0 | Brightness decay with radius |

## Modulation Candidates

- **rotationSpeed**: Differential rotation (inner rings faster). Audio modulation creates pulsing spiral wind.
- **globalRotationSpeed**: Rotates entire spiral uniformly. Creates steady spin or beat-synced rotation.
- **tightness**: Changes arm count and winding. Small changes dramatically alter spiral structure.
- **aspect.y**: Morphs between circular (1.0) and barred spiral (0.3). Animating creates breathing effect.
- **thickness**: Soft (0.3) vs crisp (0.1) ring edges. Beat-reactive thickness creates pumping.
- **center**: Moving center creates swirl migration across screen.

## Notes

- Ring count affects both quality and performance. 30 rings = 30 texture samples per pixel.
- The `n` offset in rotation (`rot(va + n)`) prevents all rings from aligning - creates variety.
- `1/l` falloff matches real galaxy surface brightness profiles (exponential disk).
- Negative tightness creates trailing arms (realistic); positive creates leading arms.
- At tightness = 0, all rings align = concentric ellipses (bar-only structure).
