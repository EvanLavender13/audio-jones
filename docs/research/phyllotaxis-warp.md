# Phyllotaxis Warp

Continuous UV displacement along phyllotaxis spiral arms. Treats the pattern as flowing curves rather than discrete seeds, creating smooth swirling distortion like galaxy arms or water spirals.

## Classification

- **Category**: TRANSFORMS → Warp
- **Core Operation**: `texture(input, uv + armDisplacement(uv))` — analytical arm distance calculation, displace perpendicular or along spiral direction
- **Pipeline Position**: Reorderable transform (alongside Sine Warp, Wave Ripple, Mobius)

## References

- [Jason Davies: Sunflower Phyllotaxis](https://www.jasondavies.com/sunflower-phyllotaxis/) - Interactive visualization of Vogel's model: `r = c√n`, `θ = n × 137.5°`. Shows how angle changes dramatically alter the pattern.
- [GM Shaders Mini: Phi](https://mini.gmshaders.com/p/phi) - GLSL implementation: `vec2 dir = cos(i * G_A + vec2(0, HPI)); float rad = sqrt(i/n) * r;`
- [Wikipedia: Golden Angle](https://en.wikipedia.org/wiki/Golden_angle) - Mathematical definition: 137.5077...° or 2.39996... radians.

## Core Algorithm

### Phyllotaxis Fundamentals

Sunflower seeds follow Vogel's model (1979):
- **Radius**: `r = c * sqrt(n)` — seeds spread outward as square root of index
- **Angle**: `θ = n * goldenAngle` — each seed rotated by ≈137.5° from previous

For warp effects, we treat this as continuous spiral arms rather than discrete seeds. Given a pixel's polar position, we calculate its angular distance from the nearest "arm" analytically — no loop required.

### Constants

```glsl
#define PHI 1.618033988749
#define GOLDEN_ANGLE 2.39996322972865  // radians, ≈137.5°
#define PI 3.14159265359
#define TWO_PI 6.28318530718
```

### Analytical Arm Distance

The key insight: at radius `r`, we can compute what angle a seed "should" be at, then measure how far off that angle the pixel is.

```glsl
// Given radius r, what seed index would be here?
// r = scale * sqrt(n)  →  n = (r/scale)²
float estimatedN = (r / scale) * (r / scale);

// What angle would that seed have?
float expectedTheta = estimatedN * divergenceAngle;

// How far off the expected angle is this pixel?
float angularOffset = theta - expectedTheta;

// Wrap to find nearest arm (could be ± several divergence angles away)
float armPhase = angularOffset / divergenceAngle;
float nearestArm = round(armPhase);
float distToArm = (armPhase - nearestArm) * divergenceAngle;
```

### Complete Arm Distance Function

```glsl
float getArmDistance(vec2 uv, float scale, float divergenceAngle) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    float theta = atan(centered.y, centered.x);

    if (r < 0.001) return 0.0;  // Avoid singularity at center

    float estimatedN = (r / scale) * (r / scale);
    float expectedTheta = estimatedN * divergenceAngle;
    float armPhase = (theta - expectedTheta) / divergenceAngle;
    float distToArm = (armPhase - round(armPhase)) * divergenceAngle;

    return distToArm;  // Signed: negative = one side, positive = other
}
```

## Shared Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `scale` | float | [0.02, 0.15] | Arm spacing. Smaller = tighter spirals. |
| `divergenceAngle` | float | [2.0, 2.8] | Angle between arms. 2.4 = golden angle. |
| `warpStrength` | float | [0.0, 1.0] | Overall displacement magnitude |
| `warpFalloff` | float | [1.0, 20.0] | How quickly displacement fades with distance from arm center. Higher = sharper arms. |
| `spinSpeed` | float | [-2.0, 2.0] | Animation: rotate spiral pattern (radians/sec) |

### Divergence Angle Reference

| Angle (radians) | Angle (degrees) | Visual Result |
|-----------------|-----------------|---------------|
| 2.39996 | 137.5° | Golden angle: classic sunflower spirals |
| 2.22 | 127° | 8 visible spokes |
| 2.51 | 144° | 5 visible spokes |
| 1.57 | 90° | 4 radial spokes, cross pattern |
| 3.14 | 180° | 2 spokes, bilateral split |

## Sub-Effects

### Tangent Warp

Displace perpendicular to radius — creates swirling/twisting motion.

```glsl
vec2 tangentWarp(vec2 uv, float scale, float divergenceAngle,
                 float strength, float falloff) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    if (r < 0.001) return uv;

    float distToArm = getArmDistance(uv, scale, divergenceAngle);

    // Displacement falls off with distance from arm center
    float displacement = distToArm * exp(-abs(distToArm) * falloff);

    // Tangent direction (perpendicular to radius)
    vec2 tangent = vec2(-centered.y, centered.x) / r;

    return uv + tangent * displacement * strength * r;
}
```

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `tangentIntensity` | float | [0.0, 1.0] | Swirl/twist amount |

### Radial Warp

Displace along radius — creates breathing/pulsing motion.

```glsl
vec2 radialWarp(vec2 uv, float scale, float divergenceAngle,
                float strength, float falloff) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    if (r < 0.001) return uv;

    float distToArm = getArmDistance(uv, scale, divergenceAngle);
    float displacement = distToArm * exp(-abs(distToArm) * falloff);

    // Radial direction (toward/away from center)
    vec2 radial = centered / r;

    return uv + radial * displacement * strength;
}
```

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `radialIntensity` | float | [0.0, 1.0] | Breathing/ripple amount |

### Combined Warp

Both tangent and radial for complex motion.

```glsl
vec2 phyllotaxisWarp(vec2 uv, float scale, float divergenceAngle,
                     float tangentStrength, float radialStrength, float falloff) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    if (r < 0.001) return uv;

    float distToArm = getArmDistance(uv, scale, divergenceAngle);
    float displacement = distToArm * exp(-abs(distToArm) * falloff);

    vec2 tangent = vec2(-centered.y, centered.x) / r;
    vec2 radial = centered / r;

    vec2 offset = tangent * displacement * tangentStrength * r
                + radial * displacement * radialStrength;

    return uv + offset;
}
```

### Animation

Rotate the spiral pattern over time by offsetting theta:

```glsl
float getArmDistanceAnimated(vec2 uv, float scale, float divergenceAngle, float time, float spinSpeed) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    float theta = atan(centered.y, centered.x) - time * spinSpeed;  // Animated rotation

    // ... rest of calculation
}
```

## Modulation Candidates

- **divergenceAngle**: At golden angle (2.4 rad), smooth spirals. Slightly off creates visible Fibonacci spoke patterns. Most dramatic parameter for changing the overall structure.
- **warpStrength**: Low values create subtle organic waviness; high values create intense vortex.
- **warpFalloff**: Low = smooth, flowing warp across entire pattern. High = sharp displacement only near arm centers.
- **tangentIntensity vs radialIntensity**: Balance between swirling and breathing motion.
- **spinSpeed**: Creates flowing animation along spiral arms.

## Notes

### Comparison to Cell-Based Phyllotaxis

| Aspect | Phyllotaxis (Cellular) | Phyllotaxis Warp |
|--------|------------------------|------------------|
| Algorithm | Loop to find nearest seed | Analytical (no loop) |
| Performance | ~40 iterations/pixel | Constant time |
| Visual output | Discrete cells/petals | Continuous flowing warp |
| Sub-effects | Fill, iso rings, glow | Tangent/radial displacement |
| Feel | Flower head, seeds | Galaxy arms, water swirls |

### Comparison to Other Warps

| Aspect | Sine Warp | Wave Ripple | Phyllotaxis Warp |
|--------|-----------|-------------|------------------|
| Pattern | Parallel waves | Concentric circles | Spiral arms |
| Symmetry | Translational | Radial | Golden ratio scaling |
| Organic feel | Rippling fabric | Water drops | Plant growth, galaxies |
| Center behavior | Uniform | Radiates from point | Singularity (mask recommended) |

### Edge Cases

- **Center singularity**: At r≈0, the math produces extreme values. Use `max(r, 0.001)` or mask the center region.
- **Large displacements**: High warpStrength can push UVs outside [0,1]. Clamp or wrap based on preference.
- **Falloff tuning**: Very low falloff creates global displacement; very high makes arms nearly invisible. Range [2.0, 10.0] typically works best.

### Implementation Tips

1. **Animate divergenceAngle**: Small oscillations around 2.4 create morphing spoke patterns as Fibonacci numbers emerge and dissolve.
2. **Layer with radial effects**: Phyllotaxis Warp + Radial Blur creates convincing galaxy-arm motion blur.
3. **Combine modes**: Tangent + radial together creates complex organic motion that neither achieves alone.
