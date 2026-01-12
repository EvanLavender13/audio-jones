# Escape-Time Fractals (Mandelbrot/Julia)

Real-time GPU rendering of Mandelbrot and Julia sets with smooth coloring and audio-reactive parameters. The fractal computes escape iterations per pixel, producing infinite detail at any zoom level within GPU precision limits.

## Classification

- **Category**: SIMULATIONS (alongside Physarum, Curl Flow, Attractor Flow, Boids)
- **Core Operation**: Per-pixel complex iteration until escape or max iterations
- **Pipeline Position**: Standalone simulation - renders to accumulation texture before output stage

## References

- [Smooth Iteration Count (Inigo Quilez)](https://iquilezles.org/articles/msetsmooth/) - Formula for banding-free coloring
- [Distance Fractals (Inigo Quilez)](https://iquilezles.org/articles/distancefractals/) - Distance estimation via derivative tracking
- [Smooth Iteration Count Derivation](https://rubenvannieuwpoort.nl/posts/smooth-iteration-count-for-the-mandelbrot-set) - Mathematical basis
- [Rendering Mandelbrot with Shaders](https://arukiap.github.io/fractals/2019/06/02/rendering-the-mandelbrot-set-with-shaders.html) - GLSL implementation guide
- [Wikipedia: Plotting Algorithms](https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set) - Comprehensive algorithm reference

## Algorithm

### Core Iteration

For complex numbers z and c, iterate:

```
z_{n+1} = z_n^2 + c
```

**Mandelbrot set**: z₀ = 0, c = pixel coordinate. Tests which c values produce bounded orbits.

**Julia set**: z₀ = pixel coordinate, c = constant parameter. Each c produces a different Julia set.

### GLSL Implementation

```glsl
// Complex number squaring: (x + iy)² = (x² - y², 2xy)
vec2 complexSquare(vec2 z) {
    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);
}

// Mandelbrot iteration with smooth coloring
float iterateMandelbrot(vec2 c) {
    const float B = 256.0;  // Bailout radius
    const int MAX_ITER = 200;

    vec2 z = vec2(0.0);
    float n = 0.0;

    for (int i = 0; i < MAX_ITER; i++) {
        z = complexSquare(z) + c;
        if (dot(z, z) > B * B) break;
        n += 1.0;
    }

    // Smooth iteration count (eliminates banding)
    return n - log2(log2(dot(z, z))) + 4.0;
}

// Julia set iteration (c is uniform, z₀ is pixel coordinate)
float iterateJulia(vec2 z0, vec2 c) {
    const float B = 256.0;
    const int MAX_ITER = 200;

    vec2 z = z0;
    float n = 0.0;

    for (int i = 0; i < MAX_ITER; i++) {
        z = complexSquare(z) + c;
        if (dot(z, z) > B * B) break;
        n += 1.0;
    }

    return n - log2(log2(dot(z, z))) + 4.0;
}
```

### Smooth Coloring Formula

The smooth iteration count eliminates discrete color bands:

```
smooth_n = n - log₂(log₂(|zₙ|))
```

Where:
- `n` = discrete iteration count at escape
- `|zₙ|` = magnitude of z when it escaped (use `dot(z,z)` to avoid sqrt)

This formula works because escape velocity is proportional to `|z|^(2^n)`, so the fractional part measures how far past the bailout radius z traveled.

### Optimized Iteration (3 multiplies instead of 5)

```glsl
// Standard: 5 multiplies
xnew = x*x - y*y + cx;
ynew = 2*x*y + cy;

// Optimized: 3 multiplies
x2 = x * x;
y2 = y * y;
y = 2.0 * x * y + cy;  // or (x + x) * y + cy
x = x2 - y2 + cx;
```

### Cardioid/Bulb Early Exit

Skip iterations for points definitely inside the Mandelbrot set:

```glsl
bool insideMainCardioid(vec2 c) {
    float q = (c.x - 0.25) * (c.x - 0.25) + c.y * c.y;
    return q * (q + (c.x - 0.25)) <= 0.25 * c.y * c.y;
}

bool insidePeriod2Bulb(vec2 c) {
    return (c.x + 1.0) * (c.x + 1.0) + c.y * c.y <= 0.0625;
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `center` | vec2 | unbounded | Center of view in complex plane |
| `zoom` | float | 0.5 to 2^15 | Magnification (log scale recommended) |
| `maxIter` | int | 50 to 1000 | Iteration limit (higher = more detail, slower) |
| `bailout` | float | 4.0 to 65536.0 | Escape radius squared (256 typical) |
| `juliaC` | vec2 | approximately [-2, 2] | Julia set c parameter |
| `colorCycleSpeed` | float | 0.0 to 10.0 | Animation speed of color palette |
| `colorScale` | float | 0.1 to 10.0 | Density of color bands |
| `mode` | int | 0 or 1 | 0 = Mandelbrot, 1 = Julia |

### Julia Set c-Parameter Sweet Spots

The most visually interesting Julia sets have c values near the Mandelbrot set boundary:

| c value | Visual character |
|---------|------------------|
| (-0.7, 0.27) | Spiral arms |
| (-0.8, 0.156) | Dendrite fractal |
| (-0.4, 0.6) | Rabbit-like |
| (0.285, 0.01) | Sea creature |
| (-0.835, -0.2321) | Spiraling tendrils |
| (-0.75, 0.0) | Douady rabbit |

## Audio Mapping Ideas

| Parameter | Audio Source | Mapping |
|-----------|--------------|---------|
| `zoom` | Bass RMS | Log scale, 1.0 → 100.0+ |
| `juliaC.x` | Low-mid frequency | Wander in range [-1.5, 0.5] |
| `juliaC.y` | Mid frequency | Wander in range [-1.0, 1.0] |
| `colorCycleSpeed` | Beat detection | Pulse on transients |
| `maxIter` | High frequency energy | 100 → 500 |
| `center` | Slow LFO + beat triggers | Drift with occasional jumps |

### c-Parameter Animation

Animate Julia c along a path near the Mandelbrot boundary for continuous morphing:

```glsl
// Circle around cardioid boundary
vec2 animateC(float t) {
    float r = 0.7885;  // Near boundary
    return vec2(r * cos(t), r * sin(t));
}

// Lemniscate path
vec2 animateCLemniscate(float t) {
    float s = sin(t);
    float c = cos(t);
    return vec2(c / (1.0 + s * s), c * s / (1.0 + s * s)) * 0.8;
}
```

## Notes

### GPU Precision Limits

Standard 32-bit float limits zoom to approximately 2^15 (32768x) before artifacts appear. Deeper zooms require:
- Double precision (if available)
- Perturbation theory (compute reference orbit at high precision, nearby pixels as deltas)

For audio visualization, standard precision is sufficient - the visual interest is in parameter variation, not extreme zoom.

### Performance Considerations

- 200 iterations at 1080p ≈ 400M complex multiplies per frame
- Modern GPUs handle this easily at 60fps
- Cardioid/bulb checking reduces iterations for Mandelbrot interior by ~30%
- Julia sets have no equivalent early exit (c is constant, not pixel-dependent)

### Mode Switching

Consider smooth transitions between Mandelbrot and Julia:
1. Mandelbrot mode: z₀ = 0, c = uv
2. Julia mode: z₀ = uv, c = juliaC
3. Blend: z₀ = mix(0, uv, blend), c = mix(uv, juliaC, blend)
