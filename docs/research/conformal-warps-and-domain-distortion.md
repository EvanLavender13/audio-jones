# Conformal Warps and Texture-Driven Domain Distortion

Research on three texture transform techniques for potential implementation: Poincaré disk projection, analytic complex function maps (Joukowski variants), and texture-driven domain warping.

## Technique 1: Poincaré Disk Projection

Hyperbolic geometry warp that compresses space toward the unit disk boundary. Creates infinite detail perception near edges—everything shrinks and densifies approaching the boundary.

### Mathematical Foundation

The Poincaré disk model maps the hyperbolic plane to the interior of a unit disk. Points near the boundary represent "infinity" in hyperbolic space.

**Radial compression formula:**
```glsl
float r = length(uv - center);
float newR = tanh(r * curvature) / tanh(curvature);
vec2 newUV = normalize(uv - center) * newR + center;
```

- `curvature > 0`: Compresses toward edge (hyperbolic)
- `curvature < 0`: Expands from center (inverse effect)
- `curvature → 0`: Approaches identity (no distortion)

**Why tanh?** Maps `[0, ∞)` to `[0, 1)` smoothly. The division by `tanh(curvature)` normalizes so `r=1` maps to `newR=1` (boundary preserved).

### Alternative: Atanh-based (inverse)

For expanding center detail outward:
```glsl
float newR = atanh(r * tanh(curvature)) / curvature;
```

### GLSL Implementation Sketch

```glsl
vec2 poincareWarp(vec2 uv, vec2 center, float curvature) {
    vec2 d = uv - center;
    float r = length(d);
    if (r < 0.001) return uv;  // Avoid division by zero

    float tanhK = tanh(curvature);
    float newR = tanh(r * curvature) / tanhK;

    return center + normalize(d) * newR;
}
```

**Note:** GLSL 3.30+ includes `tanh()` natively. For older versions:
```glsl
float tanh(float x) {
    float e2x = exp(2.0 * x);
    return (e2x - 1.0) / (e2x + 1.0);
}
```

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| `curvature` | -3.0 to 3.0 | Compression intensity (negative = expand) |
| `center` | UV space | Focal point of compression |

### Audio Mapping Ideas

- Bass → curvature magnitude (breathing compression)
- Beat → curvature sign flip (expand/contract pulse)
- Treble → center drift via Lissajous

### Visual Character

Opposite of fisheye. Fisheye expands center and compresses edges. Poincaré compresses center and preserves edges. Creates "looking down a well" or "infinite tiling toward boundary" aesthetic.

### Full Hyperbolic Tiling (Advanced)

The Shadertoy examples implement full hyperbolic tessellation, which is significantly more complex than simple radial compression. Key components from analyzed shaders:

**Hyperbolic translation** (Möbius-based):
```glsl
vec2 hyperTrans(vec2 p, vec2 o) {
    // Complex division: (p - o) / (1 - conj(o) * p)
    return cdiv(p - o, cmul(-conj(o), p) + vec2(1, 0));
}
```

**Circle inversion** (reflection across geodesic):
```glsl
vec2 cInvert(vec2 p, vec2 o, float r) {
    p -= o;
    return p * r * r / dot(p, p) + o;
}
```

**Iterative folding** (tessellation core):
```glsl
void fold(inout vec2 p, inout float count) {
    for (int k = 0; k < MAX_ITER; k++) {
        // Reflect across each edge of fundamental domain
        try_reflect(p, mirrorA, count);  // Line reflection
        try_reflect(p, mirrorB, count);  // Line reflection
        try_reflect(p, circleC, count);  // Circle inversion
    }
}
```

**Schlafli symbols** `{P, Q}` define tiling: P-sided polygons with Q meeting at each vertex. Valid tilings require `1/P + 1/Q < 1/2`.

For AudioJones, the simple tanh compression is recommended over full tiling—tiling generates procedural patterns rather than transforming existing texture content.

### References

- [Poincaré disk model - Wikipedia](https://en.wikipedia.org/wiki/Poincar%C3%A9_disk_model)
- [Hyperbolic Poincare Disc Sketch - Shadertoy](https://www.shadertoy.com/view/tsffDl) (Shane)
- [Hyperbolic Truchet tiles - Shadertoy](https://www.shadertoy.com/view/3llXR4) (mattz)
- [Hyperbolic tilings intro - Shadertoy](https://www.shadertoy.com/view/7dcXDB)
- [MathWorld - Poincaré Hyperbolic Disk](https://mathworld.wolfram.com/PoincareHyperbolicDisk.html)

---

## Technique 2: Analytic Complex Function Maps

Conformal transforms via complex functions beyond power maps. Each function produces distinct visual distortion while preserving local angles.

### Complex Number GLSL Primitives

```glsl
// Multiplication: (a+bi)(c+di) = (ac-bd) + (ad+bc)i
vec2 cx_mul(vec2 a, vec2 b) {
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

// Division: (a+bi)/(c+di)
vec2 cx_div(vec2 a, vec2 b) {
    float denom = b.x*b.x + b.y*b.y;
    return vec2((a.x*b.x + a.y*b.y)/denom, (a.y*b.x - a.x*b.y)/denom);
}

// Exponential: e^(a+bi) = e^a * (cos(b) + i*sin(b))
vec2 cx_exp(vec2 z) {
    return exp(z.x) * vec2(cos(z.y), sin(z.y));
}

// Sine: sin(a+bi) = sin(a)cosh(b) + i*cos(a)sinh(b)
vec2 cx_sin(vec2 z) {
    return vec2(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));
}

// Cosine: cos(a+bi) = cos(a)cosh(b) - i*sin(a)sinh(b)
vec2 cx_cos(vec2 z) {
    return vec2(cos(z.x) * cosh(z.y), -sin(z.x) * sinh(z.y));
}

// Power: z^p in polar form
vec2 cx_pow(vec2 z, float p) {
    float r = length(z);
    float theta = atan(z.y, z.x);
    return pow(r, p) * vec2(cos(theta * p), sin(theta * p));
}

// Logarithm: log(z) = log|z| + i*arg(z)
vec2 cx_log(vec2 z) {
    return vec2(log(length(z)), atan(z.y, z.x));
}
```

### Function Catalog

**Joukowski Transform: `z + 1/z`**
```glsl
vec2 joukowski(vec2 z) {
    return z + cx_div(vec2(1.0, 0.0), z);
}
```
- Maps circles to airfoil shapes
- Creates elongated, asymmetric bulging
- Singularity at origin (needs clamping)

**Generalized Joukowski: `z + c/z`**
```glsl
vec2 joukowskiGen(vec2 z, float c) {
    return z + cx_div(vec2(c, 0.0), z);
}
```
- `c > 1`: More extreme elongation
- `c < 1`: Subtler distortion
- `c = 0`: Identity

**Exponential Map: `exp(z)`**
```glsl
vec2 expMap(vec2 z) {
    return cx_exp(z);
}
```
- Converts vertical lines to circles, horizontal to radial rays
- Creates spiraling/radiating patterns
- Explodes to infinity quickly—needs input scaling

**Sine Map: `sin(z)`**
```glsl
vec2 sinMap(vec2 z) {
    return cx_sin(z);
}
```
- Creates oscillating bands
- Hyperbolic growth along imaginary axis
- Periodic along real axis

**Square Root: `z^0.5`**
```glsl
vec2 sqrtMap(vec2 z) {
    return cx_pow(z, 0.5);
}
```
- Branch cut creates fold
- Halves angular space (180° becomes 360°)

### Comparison with Existing Power Map

Current kaleidoscope power map implements `z^n`. These alternatives provide different visual character:

| Function | Visual Effect | Symmetry |
|----------|--------------|----------|
| `z^n` (power map) | N-fold rotational, petals | Radial |
| `z + 1/z` (Joukowski) | Elongated airfoil bulges | Bilateral |
| `exp(z)` | Spiraling rays, explosive | Radial (different) |
| `sin(z)` | Oscillating bands | Periodic |

### Implementation Considerations

- All functions need input UV remapping to complex plane (center at origin, scale appropriately)
- Output remapping back to UV space with clamping
- Singularity handling near poles (divide by zero, log of zero)
- May want blend/intensity parameter like other kaleidoscope techniques

### References

- [Visualizing Complex Numbers Using GLSL](https://hturan.com/writing/complex-numbers-glsl)
- [Conformal Texture Mapping – Nathan Reed](https://www.reedbeta.com/blog/conformal-texture-mapping/)
- [Complex Function Graph Viewer](https://davidbau.com/conformal/)
- [Joukowsky transform - Wikipedia](https://en.wikipedia.org/wiki/Joukowsky_transform)

---

## Technique 3: Texture-Driven Domain Warping

Uses the texture's own pixel values to determine UV offsets. Creates self-referential distortion that follows image structure rather than arbitrary noise.

### Core Concept

Standard domain warping: `newUV = uv + noise(uv) * strength`

Texture-driven warping: `newUV = uv + (texture(tex, uv).rg - 0.5) * strength`

The texture's R and G channels drive X and Y displacement. Bright regions push pixels one direction, dark regions push the other.

### Basic Implementation

```glsl
vec2 textureDrivenWarp(sampler2D tex, vec2 uv, float strength) {
    vec3 sample = texture(tex, uv).rgb;
    vec2 offset = (sample.rg - 0.5) * 2.0;  // Remap [0,1] to [-1,1]
    return uv + offset * strength;
}
```

### Iterative/Recursive Warping

Multiple passes create cascading distortion:

```glsl
vec2 iterativeWarp(sampler2D tex, vec2 uv, float strength, int iterations) {
    vec2 warpedUV = uv;
    for (int i = 0; i < iterations; i++) {
        vec3 sample = texture(tex, warpedUV).rgb;
        vec2 offset = (sample.rg - 0.5) * 2.0;
        warpedUV += offset * strength;
    }
    return warpedUV;
}
```

Each iteration warps based on the already-warped position, creating feedback-like effects without temporal state.

### Variations

**Luminance-driven (single-channel):**
```glsl
float lum = dot(texture(tex, uv).rgb, vec3(0.299, 0.587, 0.114));
vec2 gradient = vec2(dFdx(lum), dFdy(lum));  // Edge detection
newUV = uv + gradient * strength;
```
Warps along edges/gradients in the image.

**Rotational warp:**
```glsl
float lum = texture(tex, uv).r;
float angle = (lum - 0.5) * TWO_PI * strength;
vec2 d = uv - center;
newUV = center + vec2(d.x*cos(angle) - d.y*sin(angle),
                      d.x*sin(angle) + d.y*cos(angle));
```
Brightness drives local rotation instead of translation.

**Scale warp:**
```glsl
float lum = texture(tex, uv).r;
float scale = 1.0 + (lum - 0.5) * strength;
newUV = center + (uv - center) * scale;
```
Brightness drives local zoom.

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| `strength` | 0.0 to 0.5 | Warp magnitude (higher = more extreme) |
| `iterations` | 1 to 8 | Cascade depth (more = feedback-like) |
| `channel` | R/G/B/Lum | Which channel(s) drive offset |

### Audio Mapping Ideas

- Bass → strength (pulse distortion on beats)
- Mids → iteration count (complexity modulation)
- Treble → warp direction rotation

### Visual Character

Unlike noise-based warping (which applies arbitrary distortion), texture-driven warping creates distortions that follow the image's own structure:
- Edges warp differently than flat regions
- Bright areas and dark areas push in opposite directions
- Result feels organic and content-aware

### Comparison with Existing Effects

| Effect | Distortion Source | Character |
|--------|-------------------|-----------|
| Turbulence | Sine functions | Wavy, predictable |
| Möbius | Complex poles | Smooth, lensing |
| **Texture warp** | Image content | Self-referential, organic |

### References

- [Domain Warping - Inigo Quilez](https://iquilezles.org/articles/warp/)
- [Texture Distortion - Catlike Coding](https://catlikecoding.com/unity/tutorials/flow/texture-distortion/)
- [Surfaces - Recursive Domain Warping](https://github.com/palmdrop/surfaces)

---

## Implementation Priority Assessment

| Technique | Complexity | Novelty vs Existing | Recommended |
|-----------|------------|---------------------|-------------|
| Poincaré disk | Low | High (unique compression) | Yes |
| Joukowski | Low | Medium (similar to power map) | Maybe |
| exp/sin maps | Low | Medium | Lower priority |
| Texture-driven warp | Low-Medium | High (self-referential) | Yes |

**Poincaré** and **texture-driven warp** offer the most distinct visual character compared to existing transforms. Joukowski and other complex functions are variations on the conformal theme already covered by Möbius and power map.
