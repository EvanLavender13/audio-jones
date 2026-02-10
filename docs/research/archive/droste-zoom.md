# Droste Zoom

Seamless recursive self-similarity via conformal log-polar mapping. The image contains scaled/rotated copies of itself spiraling infinitely inward or outward. Named after the Dutch cocoa tin featuring a picture containing itself. Creates the "infinite corridor" or "Escher spiral" effect seen in M.C. Escher's Print Gallery.

## Classification

- **Category**: TRANSFORMS → Motion
- **Core Operation**: `texture(input, droste(uv))` — conformal mapping through complex logarithm space
- **Pipeline Position**: Reorderable transform (alongside Infinite Zoom, Radial Blur)

## References

- [Jos Leys - The Mathematics Behind the Droste Effect](https://www.josleys.com/article_show.php?id=82) - Canonical math reference: three-stage transform z→log(z), rotation/scale, z→exp(z). Derives α = atan(log(r₂/r₁)/2π) for Escher's spiral.
- [Reinder Nijhoff - Escher Droste Effect WebGL](https://reindernijhoff.net/2014/05/escher-droste-effect-webgl-fragment-shader/) - GLSL implementation with `escherDeformation()` function, deformationScale parameter.
- [Shadertoy: Droste Zoom (RadoKirov/FabriceNeyret2)](https://shadertoy.com/view/3sjyWG) - Golfed antialiased version with analytic form. User-provided source.
- [Shadertoy: Droste Mandelbrot (vgs)](https://www.shadertoy.com/view/XssXWr) - Droste + fractal combination showing adaptability.

## Algorithm

### Core Concept

The Droste effect maps the image plane through complex logarithm space:

1. **Log transform**: Convert to log-polar coordinates `(r, θ) → (log(r), θ)`
2. **Spiral alignment**: Rotate the log-polar rectangle so the diagonal becomes vertical
3. **Infinite tiling**: Modulo the log(r) axis for seamless recursion
4. **Animation**: Shift along the log(r) axis over time
5. **Exp transform**: Convert back to Cartesian via exponentiation

### Mathematical Foundation

From Jos Leys: The combined formula is `z → (z/r₁)^β` where `β = f·e^(iα)`:

- `α = atan(log(r₂/r₁) / 2π)` — the spiral angle
- `f = cos(α)` — scaling factor to preserve rectangle height at 2π

For a scale ratio of 256:1 (Escher's Print Gallery), α ≈ 41.43°.

### Complex Arithmetic in GLSL

```glsl
vec2 clog(vec2 z) {
    return vec2(log(length(z)), atan(z.y, z.x));
}

vec2 cexp(vec2 z) {
    return exp(z.x) * vec2(cos(z.y), sin(z.y));
}

vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}
```

### Basic Droste Transform

From FabriceNeyret2's Shadertoy:

```glsl
vec2 droste(vec2 uv, float scale, float time) {
    // scale = ratio between recursive copies (e.g., 2.3 = each copy 2.3x larger)
    float s = log(scale);          // log of scale ratio
    float a = s / TWO_PI;          // spiral shear factor: α = atan(s/2π)

    // Conformal spiral mapping: rotate log-polar space
    vec2 z = mat2(1.0, -a, a, 1.0) * vec2(log(length(uv)), atan(uv.y, uv.x));

    // Droste transform: mod the log-radius axis, animate
    z = exp(mod(z.x - time, s)) * vec2(cos(z.y), sin(z.y));

    return z;
}
```

### Escher Deformation (from Reinder Nijhoff)

```glsl
vec2 escherDeformation(vec2 uv, float deformationScale) {
    float lnr = log(length(uv));
    float th = atan(uv.y, uv.x);
    float sn = -log(deformationScale) / TWO_PI;  // spiral factor
    float l = exp(lnr - th * sn);

    return l * vec2(cos(sn * lnr + th), sin(sn * lnr + th));
}
```

### Antialiased Version (from RadoKirov's golfed shader)

The 322-char version uses analytic antialiasing via `fwidth`:

```glsl
// Grid function with smoothstep antialiasing
#define g(e, r) ( \
    c = abs(fract(e + 0.5) - 0.5), \
    c = min(c, c.y) - r/20.0, \
    smoothstep(-1.0, 1.0, c / fwidth(c)).x \
)

vec2 drosteAA(vec2 uv, float scale, float time) {
    vec2 e = mat2(0.75, 0.1103, -0.1103, 0.75)
             * vec2(log(length(uv)), atan(uv.y, uv.x));

    e = exp(e.y - 0.7 * fract(time / 2.0))
        * abs(cos(e.x + vec2(0.0, 1.57)));

    e *= exp2(ceil(-log(max(e.x, e.y)) / 0.7));

    return e;
}
```

### Direction Control

Zoom in vs out via time direction:

```glsl
// Zoom OUT (moving away from center)
z.x = mod(z.x - time, s);

// Zoom IN (moving toward center)
z.x = mod(z.x + time, s);
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `scale` | float | [1.5, 10.0] | Ratio between recursive copies. Higher = more visible nesting. |
| `speed` | float | [-2.0, 2.0] | Animation speed. Negative = zoom in, positive = zoom out. |
| `spiralAngle` | float | [-π, π] | Additional rotation per zoom cycle. 0 = pure radial zoom. |
| `innerRadius` | float | [0.0, 0.5] | Mask center to hide singularity at origin. |
| `twist` | float | [-1.0, 1.0] | Extra spiral shear beyond the natural α. |
| `branches` | int | [1, 8] | Number of spiral arms (modifies θ component). |

### Suggested UI Grouping

- **Zoom**: scale, speed (core recursion controls)
- **Spiral**: spiralAngle, twist, branches (spiral character)
- **Masking**: innerRadius (hide center singularity)

## Audio Mapping Ideas

| Parameter | Audio Source | Rationale |
|-----------|--------------|-----------|
| `speed` | Beat phase | Pulse zoom in/out with beat |
| `scale` | Low frequency energy | Bass expands the recursion ratio |
| `spiralAngle` | Stereo difference | L/R imbalance creates rotation |
| `twist` | Mid frequency energy | Mids tighten/loosen the spiral |
| `branches` | Spectral spread | Wider spectrum = more arms |

## Notes

### Singularity at Origin

The center point (r=0) maps to negative infinity in log space. Options:
- Mask with `innerRadius` parameter
- Clamp `length(uv)` to minimum value
- Blend to solid color at center

### Comparison to Existing Infinite Zoom

| Aspect | Infinite Zoom (current) | Droste Zoom (new) |
|--------|-------------------------|-------------------|
| Method | Discrete layer blending | Continuous conformal map |
| Samples | N texture reads | 1 texture read |
| Seams | Visible at high layer counts | Mathematically seamless |
| Spiral | Post-hoc rotation | Intrinsic to the mapping |
| Flexibility | More parameter tweaking | More "correct" Escher look |

Both effects have value: Infinite Zoom allows artistic control over blending, while Droste Zoom produces the mathematically "true" recursive spiral.

### Antialiasing

The log-polar mapping creates high-frequency detail near the center. Options:
- Use `fwidth()` for analytic AA (RadoKirov's approach)
- Supersample near center only
- Accept some aliasing (often masked by motion)

### Edge Handling

When transformed UV exits [0,1] bounds:
- Clamp to edge (stretches pixels)
- Wrap (requires seamless source texture)
- Fade to black (cleanest for visualizer)

### Implementation Approach

1. Start with basic droste() function
2. Add speed/direction control
3. Add spiralAngle for extra rotation
4. Add innerRadius masking
5. Optional: branches parameter for multi-arm spirals
6. Optional: analytic AA via fwidth
