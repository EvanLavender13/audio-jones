# Radial Kaleidoscope Enhancement Techniques

Research into enhancements for polar radial kaleidoscopes that add organic movement and visual interest without introducing grid/tile artifacts.

## Current Implementation

AudioJones uses polar coordinate kaleidoscope (`shaders/kaleidoscope.fs:17-45`):

1. Centers UV at screen origin (`uv - 0.5`)
2. Converts to polar coordinates (`radius`, `angle`)
3. Divides 2π into N segments via `mod(angle, segmentAngle)`
4. Mirrors within segments via `min(angle, segmentAngle - angle)`
5. Converts back to Cartesian

This produces clean radial symmetry but can feel static and mechanical.

## Enhancement Techniques

### 1. Log-Polar / Droste Spiral Transform

Combines kaleidoscope mirroring with log-polar transformation. The pattern spirals infinitely inward/outward instead of repeating in fixed radial segments.

**Core math:** Translation in log-polar space produces scaling in Cartesian space. Animating along the ρ axis creates infinite zoom without tile boundaries.

```glsl
float lnr = log(length(uv));
float th = atan(uv.y, uv.x);
float spiralFactor = -log(scale) / (2.0 * PI);

float newAngle = th + spiralFactor * lnr;
float newRadius = exp(lnr - th * spiralFactor);
```

**Characteristics:**
- Continuous self-similarity via spiral, not discrete tiles
- Infinite zoom effect when animated
- Can combine with standard segment mirroring

**Parameters:** `scale` (float), `spiralFactor` (float)

### 2. Domain Warping with fBM Noise

Applies noise-based displacement before the kaleidoscope transform. Segment boundaries become organic curves instead of straight radial lines.

```glsl
// Stacked fBM warping for organic complexity
vec2 q = vec2(fbm(uv + vec2(0.0, 0.0)),
               fbm(uv + vec2(5.2, 1.3)));
vec2 r = vec2(fbm(uv + 4.0*q + vec2(1.7, 9.2)),
               fbm(uv + 4.0*q + vec2(8.3, 2.8)));
vec2 warpedUV = uv + warpStrength * r;

// Then standard polar kaleidoscope on warpedUV
```

**Characteristics:**
- Symmetry preserved but boundaries feel natural
- Multiple warp layers increase organic quality
- Intermediate `q` and `r` values can drive coloring

**Parameters:** `warpStrength` (float), `noiseScale` (float), `noiseSpeed` (float)

### 3. Radial Twist

Rotation that varies with distance from center. Creates spiral motion without any tiling.

```glsl
float radius = length(centered);
float angle = atan(centered.y, centered.x);

// Option A: Twist (inner rotates more than outer)
angle += twistAmount * (1.0 - radius);

// Option B: Spiral (rotation proportional to radius)
angle += spiralTwist * radius;

// Option C: Both with time animation
angle += twistAmount * (1.0 - radius) + spiralTwist * radius * time;
```

**Characteristics:**
- Simple to implement
- Produces dynamic spiral motion
- Combines well with other techniques

**Parameters:** `twistAmount` (float), `spiralTwist` (float)

### 4. Kaleidoscopic IFS (Fractal)

Kaleidoscopic Iterated Function Systems apply multiple reflection operations iteratively. Creates fractal complexity at multiple scales while maintaining symmetry.

```glsl
vec2 p = uv;
for (int i = 0; i < iterations; i++) {
    // Fold across axes
    p = abs(p);

    // Diagonal fold
    if (p.y > p.x) p = p.yx;

    // Scale and translate
    p = p * scale - offset;
}
```

**Characteristics:**
- Fractal detail emerges at multiple scales
- Symmetry maintained through folding operations
- High visual complexity from simple rules
- Can run entirely on GPU in fragment shader

**Parameters:** `iterations` (int), `scale` (float), `offset` (vec2)

### 5. Moving Focal Point

Animate the kaleidoscope center position over time. Prevents the static "staring at a fixed point" feeling.

```glsl
vec2 center = vec2(0.5, 0.5) + centerOffset;
vec2 centered = uv - center;

// Animation options:
// - Circular drift: centerOffset = amplitude * vec2(sin(time), cos(time))
// - Lissajous: centerOffset = vec2(sin(time * freqX), cos(time * freqY))
// - Noise-driven: centerOffset = vec2(noise(time), noise(time + 100.0))
```

**Characteristics:**
- Subtle movement breaks static appearance
- Works with all other techniques
- Can follow external input (mouse, etc.)

**Parameters:** `centerOffset` (vec2) or animation parameters

## Combination Strategies

These techniques layer well:

1. **Domain warp → Polar kaleidoscope** - Organic segment boundaries
2. **Polar kaleidoscope → Radial twist** - Spiral motion on mirrored pattern
3. **Log-polar → Segment mirror** - Infinite zoom with radial symmetry
4. **Moving center + Any technique** - Adds drift to any effect

Order matters: domain warping applies to input UVs, twist/spiral applies to polar angle after conversion.

## Implementation Considerations

### Derivative Handling

Domain warping and log-polar transforms require computing texture derivatives before transformation for correct filtering:

```glsl
vec2 dx = dFdx(uv);
vec2 dy = dFdy(uv);
// Apply transforms to uv...
vec4 color = textureGrad(tex, transformedUV, dx, dy);
```

### Performance

All techniques run single-pass with O(1) per-pixel complexity:

| Technique | Operations/pixel | Notes |
|-----------|------------------|-------|
| Log-polar | 3 trig + 5 arith | log, exp, atan, sin, cos |
| Domain warp | N noise samples | Depends on fBM octaves |
| Radial twist | 2 trig + 3 arith | Minimal overhead |
| KIFS | N iterations × 5 arith | Scales with iteration count |
| Moving center | 2 arith | Negligible |

fBM noise is the most expensive; consider baking to texture for complex warps.

## Sources

- [Log-spherical Mapping in SDF Raymarching](https://www.osar.fr/notes/logspherical/) - Log-polar transform theory
- [Domain Warping (Inigo Quilez)](https://iquilezles.org/articles/warp/) - fBM noise warping technique
- [Escher Droste Effect WebGL Shader](https://reindernijhoff.net/2014/05/escher-droste-effect-webgl-fragment-shader/) - Spiral zoom implementation
- [Fractal Forums - Kaleidoscopic IFS](http://www.fractalforums.com/ifs-iterated-function-systems/kaleidoscopic-(escape-time-ifs) - KIFS technique origin
- [Polar Coordinates Tutorial (Cyanilux)](https://www.cyanilux.com/tutorials/polar-coordinates/) - Noise + polar combinations
- [Daniel Ilett - Crazy Kaleidoscopes](https://danielilett.com/2020-02-19-tut3-8-crazy-kaleidoscopes/) - Polar coordinate fundamentals
