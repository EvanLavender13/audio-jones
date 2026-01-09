# Mobius Transform

Conformal UV warping via complex-plane fractional linear transformation. Maps circles to circles, preserves angles, creates smooth flow patterns between two control points. Animating parameters produces hypnotic spiral, rotation, and scaling effects.

## Classification

- **Category**: UV Transform
- **Core Operation**: `texture(input, mobius(uv))` where `mobius(z) = (Az + B) / (Cz + D)`
- **Pipeline Position**: Reorderable transform (Transforms > Warp)

## References

- [Wikipedia: Mobius transformation](https://en.wikipedia.org/wiki/M%C3%B6bius_transformation) - Complete mathematical foundation: definition, decomposition, classification, fixed points
- [Shadertoy: Mobius Sierpinski (Shane)](https://www.shadertoy.com/view/ldVXDG) - Clean two-point parameterization with spiral zoom
- [Shadertoy: Mobius + Riemann sphere (neozhaoliang)](https://www.shadertoy.com/view/XsS3Dm) - Full ABCD form, elliptic/hyperbolic/loxodromic classification
- [Shadertoy: Inverse Mobius isolines (FabriceNeyret2)](https://www.shadertoy.com/view/MllBzj) - Inverse transform for drawing in screen space

## Algorithm

### Core Formula

The Mobius transformation maps complex plane coordinates:

```
f(z) = (Az + B) / (Cz + D)
```

where `A, B, C, D` are complex numbers satisfying `AD - BC != 0`.

### Complex Arithmetic in GLSL

Represent complex numbers as `vec2(real, imag)`:

```glsl
vec2 cmul(vec2 z, vec2 w) {
    return vec2(z.x*w.x - z.y*w.y, z.x*w.y + z.y*w.x);
}

vec2 cdiv(vec2 z, vec2 w) {
    return vec2(z.x*w.x + z.y*w.y, -z.x*w.y + z.y*w.x) / dot(w, w);
}

vec2 applyMobius(vec2 z, vec2 A, vec2 B, vec2 C, vec2 D) {
    vec2 num = cmul(A, z) + B;
    vec2 den = cmul(C, z) + D;
    return cdiv(num, den);
}
```

### Two-Point Parameterization

Shane's compact form uses two control points `z1, z2`:

```glsl
vec2 Mobius(vec2 p, vec2 z1, vec2 z2) {
    z1 = p - z1;
    p -= z2;
    return vec2(dot(z1, p), z1.y*p.x - z1.x*p.y) / dot(p, p);
}
```

This creates a transform with fixed points related to `z1` and `z2`.

### Decomposition into Primitives

Any Mobius transform decomposes into four steps (when `C != 0`):

1. **Translation**: `z + d/c`
2. **Inversion**: `1/(-z)` (inversion + reflection)
3. **Scale/Rotation**: `((bc-ad)/c^2) * z`
4. **Translation**: `z + a/c`

### Classification by Behavior

The transform's character depends on the **characteristic constant** `k = e^(rho + alpha*i)`:

| Type | Condition | Visual Effect |
|------|-----------|---------------|
| Elliptic | `rho = 0` | Pure rotation around fixed points |
| Hyperbolic | `alpha = 0` | Scaling flow between fixed points |
| Loxodromic | Both nonzero | Spiral flow (most visually interesting) |
| Parabolic | `k = 1` | Translation (single fixed point) |

### Elliptic/Hyperbolic Primitives

From neozhaoliang's implementation:

```glsl
// Hyperbolic: scaling via log-polar distance
void trans_hyperbolic(inout vec2 p, float speed, float gridSize) {
    float d = log(length(p)) - speed;
    d = mod(d + 0.5*gridSize, gridSize) - 0.5*gridSize;  // wrap to avoid precision loss
    p = normalize(p) * exp(d);
}

// Elliptic: pure rotation
void trans_elliptic(inout vec2 p, float angle) {
    float c = cos(angle), s = sin(angle);
    p = vec2(c*p.x - s*p.y, s*p.x + c*p.y);
}

// Loxodromic: combine both
void trans_loxodromic(inout vec2 p, float speed, float angle) {
    trans_hyperbolic(p, speed, GRID_SIZE);
    trans_elliptic(p, angle);
}
```

### Spiral Zoom (Common Companion Effect)

Often combined with Mobius for Droste-like spirals:

```glsl
vec2 spiralZoom(vec2 p, vec2 center, float branches, float spiralTightness, float zoom, float phase) {
    p -= center;
    float angle = atan(p.y, p.x) / 6.283;
    float logDist = log(length(p));
    return vec2(
        angle * branches + logDist * spiralTightness,
        angle - logDist * zoom
    ) + phase;
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `fixedPoint1` | vec2 | [-2, 2] | First control point / fixed point |
| `fixedPoint2` | vec2 | [-2, 2] | Second control point / fixed point |
| `rho` | float | [-2, 2] | Expansion factor (hyperbolic strength) |
| `alpha` | float | [-PI, PI] | Rotation factor (elliptic strength) |
| `animSpeed` | float | [0, 2] | Animation rate for rho/alpha |
| `spiralBranches` | float | [1, 12] | Number of spiral arms (if spiral zoom enabled) |
| `spiralTightness` | float | [0, 1] | How tight the spiral wraps |

### Suggested UI Grouping

- **Fixed Points**: Two 2D position controls (could use mouse input)
- **Transform Type**: Dropdown (Elliptic / Hyperbolic / Loxodromic) or auto-detect from rho/alpha
- **Strength**: Single slider combining rho+alpha via angle
- **Animation**: Speed slider

## Audio Mapping Ideas

| Parameter | Audio Source | Rationale |
|-----------|--------------|-----------|
| `rho` | Low frequency energy | Bass drives expansion/contraction |
| `alpha` | Mid frequency energy | Mids control rotation speed |
| `fixedPoint1` | Stereo balance | L/R panning moves control point |
| `animSpeed` | Beat intensity | Faster animation on strong beats |
| `spiralBranches` | Spectral centroid | Brighter sound = more spiral arms |

## Notes

### Edge Cases

- **Pole singularity**: When `Cz + D = 0`, the denominator vanishes. Points near `z = -D/C` map toward infinity. Clamp or detect this case.
- **Precision**: Log-polar wrapping in hyperbolic mode prevents precision loss at extreme zoom levels.
- **Aspect ratio**: Convert screen UV to complex plane centered at origin before transform.

### Combination Strategies

- **Mobius + Spiral Zoom**: Classic combination (Shane's example). Apply Mobius first, then spiral zoom.
- **Animated fixed points**: Moving `z1, z2` over time creates flowing distortion.
- **Stacked Mobius**: Multiple transforms compound for complex warping.

### Implementation Approach

1. Start with Shane's two-point form (simpler UI)
2. Add rho/alpha animation for elliptic/hyperbolic motion
3. Optional: spiral zoom as secondary effect
4. Later: expose full ABCD form for power users
