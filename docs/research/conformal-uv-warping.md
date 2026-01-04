# Conformal UV Warping Techniques

Research into conformal (angle-preserving) UV transformations for smooth, organic texture warping without hard edges or tiling artifacts. All techniques transform UV coordinates before sampling an input texture.

## Why Conformal Maps

Conformal mappings preserve local angles. When applied to texture UVs, this means:
- No shearing or stretching that creates visible distortion artifacts
- Smooth, organic-looking warps
- Radially-varying effects without hard boundaries
- Mathematically elegant with simple GLSL implementations

Any holomorphic (complex-differentiable) function produces a conformal map. The techniques below use complex number arithmetic where `z = uv.x + i*uv.y`.

## Techniques

### 1. Möbius Transformations

The Möbius transform `w = (az + b) / (cz + d)` maps the complex plane to itself conformally. Controls zoom, rotation, and translation through four complex parameters.

**Core Algorithm:**

```glsl
// Complex number operations
vec2 cmul(vec2 a, vec2 b) { return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x); }
vec2 cdiv(vec2 a, vec2 b) { return cmul(a, vec2(b.x, -b.y)) / dot(b, b); }

vec2 mobius(vec2 z, vec2 a, vec2 b, vec2 c, vec2 d) {
    vec2 numerator = cmul(a, z) + b;
    vec2 denominator = cmul(c, z) + d;
    return cdiv(numerator, denominator);
}

// Usage: center UV, apply transform, re-center
vec2 z = (uv - 0.5) * 2.0;  // Map to [-1, 1]
vec2 w = mobius(z, a, b, c, d);
vec2 warpedUV = w * 0.5 + 0.5;
```

**Special Cases:**
- `a=1, b=0, c=0, d=1`: Identity (no transform)
- `a=s, b=0, c=0, d=1`: Uniform scale by s
- `a=e^(iθ), b=0, c=0, d=1`: Rotation by θ
- `a=1, b=t, c=0, d=1`: Translation by t
- `a=1, b=0, c=1, d=0`: Inversion `w = 1/z`

**Smooth Animation:**

```glsl
// Animate parameters smoothly
vec2 a = vec2(cos(time * 0.3), sin(time * 0.3));  // Rotating zoom
vec2 b = vec2(0.0);
vec2 c = vec2(0.1 * sin(time * 0.5), 0.0);        // Subtle pole drift
vec2 d = vec2(1.0, 0.0);
```

**Characteristics:**
- Inherently smooth everywhere except at poles (`cz + d = 0`)
- Combines zoom, rotation, translation in one operation
- Parameters interpolate smoothly for animation
- Radially-varying distortion around poles

**Audio-Reactive Mapping:**
- Bass → magnitude of `a` (zoom intensity)
- Mids → angle of `a` (rotation)
- Treble → magnitude of `c` (pole proximity, creates "lensing")

**Parameters:** `a`, `b`, `c`, `d` (vec2 each, representing complex numbers)

### 2. Circle Inversion

Inversion through a circle of radius `r` centered at `c` maps points inside to outside and vice versa. Creates smooth radial distortion without KIFS-style hard fold lines.

**Core Algorithm:**

```glsl
vec2 circleInvert(vec2 p, vec2 center, float radius) {
    vec2 d = p - center;
    float r2 = radius * radius;
    float dist2 = dot(d, d);
    return center + d * (r2 / dist2);
}

// Single inversion
vec2 z = uv - 0.5;
vec2 w = circleInvert(z, inversionCenter, inversionRadius);
vec2 warpedUV = w + 0.5;
```

**Blended Inversion (Soft Falloff):**

```glsl
vec2 blendedInvert(vec2 p, vec2 center, float radius, float blend) {
    vec2 inverted = circleInvert(p, center, radius);
    float dist = length(p - center);
    float t = smoothstep(radius * 0.5, radius * 2.0, dist);
    return mix(inverted, p, t * (1.0 - blend));
}
```

**Double Inversion (Richer Patterns):**

```glsl
vec2 z = uv - 0.5;
z = circleInvert(z, center1, radius1);
z = circleInvert(z, center2, radius2);
vec2 warpedUV = z + 0.5;
```

**Characteristics:**
- Single inversion: smooth radial "bulge" or "pinch"
- Multiple inversions: more complex but still smooth
- Unlike KIFS, no folding = no hard edges
- Naturally radially-varying

**Audio-Reactive Mapping:**
- Bass → `inversionRadius` (stronger inversion on beats)
- Mids → `inversionCenter` position (drifting focal point)

**Parameters:** `center` (vec2), `radius` (float), `blend` (float, 0-1)

### 3. Poincaré Disk / Hyperbolic Warp

Maps Euclidean coordinates into the Poincaré disk model of hyperbolic space. Creates smooth compression toward edges with infinite detail near the boundary.

**Core Algorithm:**

```glsl
// Euclidean to Poincaré disk
vec2 euclideanToPoincare(vec2 p, float curvature) {
    float r = length(p);
    // Hyperbolic tangent maps [0, inf) to [0, 1)
    float rHyp = tanh(r * curvature);
    return p * (rHyp / max(r, 0.001));
}

// Poincaré disk to Euclidean (inverse)
vec2 poincareToEuclidean(vec2 p, float curvature) {
    float r = length(p);
    float rEuc = atanh(min(r, 0.999)) / curvature;
    return p * (rEuc / max(r, 0.001));
}

// Usage: warp texture lookup
vec2 z = (uv - 0.5) * 2.0;
vec2 w = poincareToEuclidean(z, curvature);
vec2 warpedUV = w * 0.5 + 0.5;
```

**Hyperbolic Rotation (Smooth Spin):**

```glsl
// Rotation in hyperbolic space (Möbius rotation around origin)
vec2 hyperbolicRotate(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 z = (uv - 0.5) * 2.0;
z = euclideanToPoincare(z, curvature);
z = hyperbolicRotate(z, time * rotationSpeed);
z = poincareToEuclidean(z, curvature);
```

**Characteristics:**
- Smooth compression: center stays clear, edges compress infinitely
- No hard boundaries—asymptotic approach to unit circle
- Creates "fisheye" effect with mathematical elegance
- Radially-varying by definition

**Audio-Reactive Mapping:**
- Bass → `curvature` (higher = more compression, pulsing on beats)
- Full spectrum → hyperbolic rotation speed

**Parameters:** `curvature` (float, typically 0.5-3.0)

### 4. Joukowski and Analytic Function Warping

Apply any holomorphic function to UV coordinates. All such functions are conformal (angle-preserving), creating smooth organic distortions.

**Joukowski Transform (`z + 1/z`):**

```glsl
vec2 joukowski(vec2 z) {
    vec2 zInv = cdiv(vec2(1.0, 0.0), z);  // 1/z
    return z + zInv;
}

// Usage
vec2 z = (uv - 0.5) * 2.0;
// Offset slightly to avoid singularity at origin
z += vec2(0.1, 0.0);
vec2 w = joukowski(z);
vec2 warpedUV = w * 0.25 + 0.5;  // Scale down (Joukowski expands)
```

**Power Functions (`z^n`):**

```glsl
vec2 cpow(vec2 z, float n) {
    float r = length(z);
    float theta = atan(z.y, z.x);
    float newR = pow(r, n);
    float newTheta = theta * n;
    return newR * vec2(cos(newTheta), sin(newTheta));
}

// z^2: doubles angles, squares radii (creates 2-fold symmetry)
// z^0.5: halves angles, square-roots radii (creates smooth spread)
vec2 w = cpow(z, 2.0);
```

**Exponential (`exp(z)`):**

```glsl
vec2 cexp(vec2 z) {
    return exp(z.x) * vec2(cos(z.y), sin(z.y));
}

// Creates spiral-outward effect
// Real part controls radius, imaginary part controls angle
vec2 z = (uv - 0.5) * 3.0;
vec2 w = cexp(z);
```

**Sine/Cosine (`sin(z)`, `cos(z)`):**

```glsl
vec2 csin(vec2 z) {
    return vec2(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));
}

vec2 ccos(vec2 z) {
    return vec2(cos(z.x) * cosh(z.y), -sin(z.x) * sinh(z.y));
}

// Creates oscillating bands with smooth transitions
vec2 z = (uv - 0.5) * PI;
vec2 w = csin(z);
```

**Characteristics:**
- All holomorphic functions produce smooth warps
- Can combine: `f(g(z))` is also holomorphic
- Each function has unique visual character
- Singularities (poles) create strong local distortion

**Function Visual Characteristics:**

| Function | Effect | Singularities |
|----------|--------|---------------|
| `z + 1/z` | Airfoil-like asymmetric flow | Origin |
| `z^2` | 2-fold symmetry, angle doubling | Origin (mild) |
| `z^0.5` | Smooth radial spread | Origin (branch cut) |
| `exp(z)` | Spiral outward | None |
| `sin(z)` | Vertical oscillating bands | None |
| `log(z)` | Spiral inward | Origin |

**Parameters:** Function choice, scale factors, offset to avoid singularities

### 5. Log-Polar with Continuous Modulation

Log-polar mapping without the hard seams of traditional Droste. Uses smooth modulation and blending rather than strict periodicity.

**Core Algorithm:**

```glsl
vec2 cartesianToLogPolar(vec2 p) {
    return vec2(log(length(p)), atan(p.y, p.x));
}

vec2 logPolarToCartesian(vec2 lp) {
    return exp(lp.x) * vec2(cos(lp.y), sin(lp.y));
}
```

**Smooth Spiral (No Seams):**

```glsl
vec2 z = uv - 0.5;
vec2 lp = cartesianToLogPolar(z);

// Spiral rotation: angle varies with log-radius
float spiralTwist = 0.5;
lp.y += lp.x * spiralTwist;

// Smooth radial modulation instead of hard wrap
float radialWave = sin(lp.x * frequency + time) * amplitude;
lp.x += radialWave;

vec2 w = logPolarToCartesian(lp);
vec2 warpedUV = w + 0.5;
```

**Blended Multi-Scale:**

```glsl
// Sample at multiple log-scales, blend smoothly
vec4 color = vec4(0.0);
float totalWeight = 0.0;

for (int i = 0; i < 3; i++) {
    float scale = 1.0 + float(i) * 0.5;
    vec2 lp = cartesianToLogPolar(z);
    lp.x += log(scale);
    vec2 sampleUV = logPolarToCartesian(lp) + 0.5;

    float weight = 1.0 / scale;
    color += texture(tex, sampleUV) * weight;
    totalWeight += weight;
}
color /= totalWeight;
```

**Characteristics:**
- Smooth modulation avoids Droste's hard repeat boundaries
- Spiral twist creates organic flowing motion
- Multi-scale blending adds depth without edges
- Radially-varying by nature of log transform

**Audio-Reactive Mapping:**
- Bass → `spiralTwist` (spiral intensity on beats)
- Mids → `radialWave` amplitude
- Time → phase offset for continuous motion

**Parameters:** `spiralTwist` (float), `frequency` (float), `amplitude` (float)

## Complex Number Utilities

Standard GLSL helper functions for complex arithmetic:

```glsl
// Complex multiplication: (a + bi)(c + di) = (ac - bd) + (ad + bc)i
vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// Complex division: a/b = a * conj(b) / |b|^2
vec2 cdiv(vec2 a, vec2 b) {
    return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / dot(b, b);
}

// Complex conjugate
vec2 conj(vec2 z) { return vec2(z.x, -z.y); }

// Complex exponential
vec2 cexp(vec2 z) { return exp(z.x) * vec2(cos(z.y), sin(z.y)); }

// Complex logarithm (principal branch)
vec2 clog(vec2 z) { return vec2(log(length(z)), atan(z.y, z.x)); }

// Complex power
vec2 cpow(vec2 z, float n) {
    float r = pow(length(z), n);
    float theta = atan(z.y, z.x) * n;
    return r * vec2(cos(theta), sin(theta));
}

// Complex sine
vec2 csin(vec2 z) {
    return vec2(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));
}
```

## Implementation Considerations

### Singularity Handling

Most conformal maps have singularities (poles) where the function diverges. Strategies:

```glsl
// Clamp distance from singularity
vec2 safeDivide(vec2 a, vec2 b) {
    float denom = max(dot(b, b), 0.0001);
    return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / denom;
}

// Blend to identity near singularities
float singularityDist = length(z - pole);
float blend = smoothstep(0.0, 0.1, singularityDist);
vec2 w = mix(z, transformedZ, blend);
```

### Texture Boundary Handling

Conformal maps often produce UVs outside [0,1]. Options:

```glsl
// Mirror repeat (smooth)
warpedUV = abs(mod(warpedUV + 1.0, 2.0) - 1.0);

// Fade to black at edges
float edge = smoothstep(1.0, 0.9, max(abs(warpedUV.x - 0.5), abs(warpedUV.y - 0.5)) * 2.0);
color *= edge;
```

### Derivative Correction for Filtering

For correct texture filtering after warping:

```glsl
// Compute derivatives before transform
vec2 dx = dFdx(uv);
vec2 dy = dFdy(uv);

// Apply transform to uv...
vec2 warpedUV = transform(uv);

// Use gradients for proper mip selection
vec4 color = textureGrad(tex, warpedUV, dx, dy);
```

## Performance

All techniques are single-pass with O(1) per-pixel complexity:

| Technique | Trig Ops | Arith Ops | Notes |
|-----------|----------|-----------|-------|
| Möbius | 0 | 12 | Pure arithmetic |
| Circle Inversion | 0 | 8 | Per inversion |
| Poincaré Disk | 2 | 10 | tanh/atanh via exp |
| Joukowski | 0 | 14 | Includes 1/z |
| Power (`z^n`) | 2 | 6 | atan + sin/cos |
| Exponential | 2 | 4 | exp + sin/cos |
| Log-Polar | 3 | 4 | log + atan + exp |

All are cheaper than fBM noise (multiple texture/noise samples).

## Sources

- [ubavic/mobius-shader](https://github.com/ubavic/mobius-shader) - GLSL Möbius implementation
- [Log Moebius Transfo psychedelic](https://www.shadertoy.com/view/XdyXD3) - Animated Möbius on Shadertoy
- [Nathan Reed - Conformal Texture Mapping](https://www.reedbeta.com/blog/conformal-texture-mapping/) - Theory and aesthetics
- [Circle Inversion Fractals - Michael Laszlo](https://sites.nova.edu/mjl/graphics/circles-and-spheres/circle-inversions/circle-inversion-fractals/) - Inversion geometry
- [Circle Inversion on OneShader](https://oneshader.net/shader/ab89ae6583) - GLSL example
- [Rendering a 3D hyperbolic space (thesis)](https://www.pdf.inf.usi.ch/papers/bachelor_projects/alessio_giovagnini.pdf) - Poincaré disk shaders
- [Number Analytics - Poincaré Disk](https://www.numberanalytics.com/blog/advanced-poincare-disk-model) - Hyperbolic geometry overview
- [Interactive Joukowski airfoil](https://grahampullan.github.io/viz/2021/05/04/Joukowski-airfoils.html) - Joukowski transform visualization
- [Log-spherical Mapping in SDF Raymarching](https://www.osar.fr/notes/logspherical/) - Log-polar theory
- [Logarithmic Spiral on Shadertoy](https://www.shadertoy.com/view/MXVfRc) - Log-polar spiral example
- [David Bau - Conformal Map Viewer](https://davidbau.com/conformal/) - Interactive complex function explorer
- [Complex Analysis - Conformal Mapping](https://complex-analysis.com/content/conformal_mapping.html) - Mathematical foundations
