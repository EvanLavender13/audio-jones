# Fractal and Psychedelic Shader Techniques Research

Research into fractal/psychedelic shader techniques for audio-reactive post-processing effects.

## Summary Table

| Technique | UV Math Only? | Generates vs Modifies Texture | Cost (iterations) | Animatable? |
|-----------|--------------|------------------------------|-------------------|-------------|
| Julia Sets | Yes | Generates patterns | 32-256 per pixel | Yes (c parameter) |
| Newton Fractals | Yes | Generates patterns | 32-128 per pixel | Yes (polynomial coefficients) |
| Fractal Flames (IFS) | No | Generates patterns | Histogram-based | Partial |
| Orbital Traps | Yes | Can sample existing textures | 32-128 per pixel | Yes (trap position/shape) |
| Lyapunov Fractals | Yes | Generates patterns | 50-200 per pixel | Limited |

---

## 1. Julia Sets

### Overview
Julia sets iterate the function `z = z^2 + c` where `z` starts at the pixel's UV coordinate (mapped to complex plane) and `c` is a constant complex parameter.

### Is it just UV math?
**Yes.** Pure mathematical iteration on UV coordinates mapped to the complex plane. No external data required.

### Generates or modifies texture?
**Generates patterns from scratch.** Each pixel computes an iteration count (escape time) which maps to color. The fractal emerges from the iteration behavior.

### Computational cost
- **32-256 iterations per pixel** typical
- Each iteration: 4 multiplications, 2 additions, 1 comparison
- WebGL/GLSL limits: max 255 iterations with float index due to GPU limitations
- Deep zooms require higher iteration counts and FP64 precision

### Animation potential
**Excellent.** The `c` parameter animates smoothly:
```glsl
// Animate c along a circle for smooth morphing
vec2 c = vec2(0.7885 * cos(time), 0.7885 * sin(time));
```

Moving `c` along paths (cardioid boundary, circles) creates organic morphing. Audio-reactive mapping to `c.x` and `c.y` produces responsive visuals.

### Core GLSL implementation
```glsl
vec3 julia(vec2 uv, vec2 c, int maxIter) {
    vec2 z = uv * 2.0 - 1.0;  // Map UV to [-1, 1]
    z *= 1.5;  // Zoom factor

    int i;
    for (i = 0; i < maxIter; i++) {
        float x = z.x * z.x - z.y * z.y + c.x;
        float y = 2.0 * z.x * z.y + c.y;
        z = vec2(x, y);
        if (dot(z, z) > 4.0) break;
    }

    // Smooth coloring
    float t = float(i) / float(maxIter);
    return vec3(t);
}
```

### Using Julia for UV distortion (post-processing)
Julia sets can distort existing textures by using the final `z` value or iteration count to offset UVs:
```glsl
vec4 juliaDistort(vec2 uv, sampler2D tex, vec2 c) {
    vec2 z = uv * 2.0 - 1.0;
    for (int i = 0; i < 32; i++) {
        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        if (dot(z, z) > 4.0) break;
    }
    // Use final z to offset UV
    vec2 distortedUV = uv + z * 0.01;
    return texture(tex, distortedUV);
}
```

---

## 2. Newton Fractals

### Overview
Newton fractals visualize Newton-Raphson root finding on polynomials in the complex plane. Color indicates which root the iteration converges to; brightness indicates convergence speed.

### Is it just UV math?
**Yes.** Complex polynomial evaluation and division at each iteration.

### Generates or modifies texture?
**Generates patterns from scratch.** Produces distinctive organic shapes with smooth basins of attraction.

### Computational cost
- **32-128 iterations per pixel**
- Each iteration: polynomial evaluation + derivative + complex division
- More expensive than Julia due to division: `cdiv(f(z), df(z))`

### Animation potential
**Good.** Polynomial coefficients animate smoothly:
- Vary the polynomial (e.g., `z^3 - 1` to `z^3 + c`)
- Animate root positions
- Smooth blending between different polynomials

### Core GLSL implementation
```glsl
// For f(z) = z^3 - 1
vec2 f(vec2 n) {
    return vec2(
        n.x*n.x*n.x - 3.0*n.x*n.y*n.y - 1.0,
        -n.y*n.y*n.y + 3.0*n.x*n.x*n.y
    );
}

vec2 df(vec2 n) {
    return 3.0 * vec2(
        n.x*n.x - n.y*n.y,
        2.0 * n.x * n.y
    );
}

vec2 cdiv(vec2 a, vec2 b) {
    float d = dot(b, b);
    if (d == 0.0) return a;
    return vec2(
        (a.x*b.x + a.y*b.y) / d,
        (a.y*b.x - a.x*b.y) / d
    );
}

vec3 newton(vec2 uv, int maxIter) {
    vec2 z = uv * 3.0;  // Scale to visible region

    for (int i = 0; i < maxIter; i++) {
        vec2 zn = z - cdiv(f(z), df(z));
        if (distance(zn, z) < 0.00001) {
            // Color by which root converged to
            float angle = atan(z.y, z.x);
            return hsv2rgb(vec3(angle / 6.28, 0.8, 1.0 - float(i)/float(maxIter)));
        }
        z = zn;
    }
    return vec3(0.0);
}
```

### Unique characteristics
- Produces smooth, organic boundaries (no hard edges)
- Basins of attraction create non-tiling, non-repetitive patterns
- Radial variation naturally emerges from polynomial symmetry

---

## 3. Fractal Flames (IFS with Soft Blending)

### Overview
Fractal flames use Iterated Function Systems with nonlinear transforms, log-density display, and color-by-structure. Created by Scott Draves in 1992.

### Is it just UV math?
**No.** Traditional fractal flames use stochastic point iteration to build a histogram, not per-pixel UV calculation.

### Generates or modifies texture?
**Generates patterns from scratch.** Produces soft, organic imagery with complex color gradients.

### Computational cost
**High and different.** Not iteration-per-pixel but point-sampling:
- Chaos game: millions/billions of point iterations
- Histogram accumulation: memory bandwidth intensive
- Log-density tone mapping: post-process step
- Traditional algorithm is inherently sequential

### GPU approaches
1. **Deterministic texture-based**: Use texture mapping with iterative refinement
   - Much less sampling noise than stochastic method
   - Real-time HD animation possible
2. **Compute shader histogram**: GPU parallel point iteration with atomic histogram updates

### Animation potential
**Partial.** Transform parameters animate smoothly, but:
- Stochastic rendering creates flickering between frames
- Histogram requires accumulation time for quality
- Deterministic GPU method enables smooth real-time animation

### Why it's less suitable for post-processing
Fractal flames fundamentally generate structure rather than distort input. The algorithm builds images from scattered points, not from transforming existing pixel data.

### Alternative: Fractal Brownian Motion domain warping
For similar organic aesthetics with post-processing capability:
```glsl
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 6; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

vec2 warp(vec2 p) {
    vec2 q = vec2(fbm(p + vec2(0.0, 0.0)), fbm(p + vec2(5.2, 1.3)));
    vec2 r = vec2(fbm(p + 4.0*q + vec2(1.7, 9.2)), fbm(p + 4.0*q + vec2(8.3, 2.8)));
    return p + 4.0*r;
}
```

---

## 4. Orbital Traps

### Overview
Orbital traps track the minimum distance from iteration orbit points to geometric shapes (point, line, circle, cross, image). The trap distance becomes the color signal.

### Is it just UV math?
**Yes.** Distance calculations during standard fractal iteration.

### Generates or modifies texture?
**Both!** This is the key finding:
- Can generate patterns using geometric trap shapes
- Can sample existing textures as the trap source

### Computational cost
- **32-128 iterations per pixel** (same as underlying fractal)
- Additional distance calculation per iteration
- Texture sampling adds minimal overhead

### Animation potential
**Excellent.** Multiple parameters animate smoothly:
- Trap position
- Trap size/scale
- Trap rotation (orbitTrapSpin)
- Trap shape blending
- Julia/Mandelbrot c parameter

### Texture-based orbit traps
```glsl
uniform sampler2D trapTexture;
uniform vec2 trapOffset;
uniform float trapScale;
uniform float trapRotation;

mat2 rot(float a) { return mat2(cos(a), sin(a), -sin(a), cos(a)); }

vec4 orbitTrap(vec2 uv, vec2 c) {
    vec2 z = uv;
    vec4 color = vec4(0.0);

    for (int i = 0; i < 64; i++) {
        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;

        // Map z to texture coordinates
        vec2 sp = 0.5 + (z / trapScale * rot(trapRotation) - trapOffset);
        vec4 sample = texture(trapTexture, sp);

        // Blend trapped samples
        if (sample.a > 0.0) {
            color = mix(color, sample, sample.a * 0.5);
        }

        if (dot(z, z) > 4.0) break;
    }
    return color;
}
```

### Why orbital traps fit the requirements
- **Non-repetitive**: fractal structure is inherently aperiodic
- **Non-tiling**: no seams or repeating units
- **Non-hard-edged**: minimum distance creates smooth gradients
- **Radially varying**: trap distance naturally varies with position
- **Texture integration**: can distort/sample existing visuals

---

## 5. Lyapunov Fractals

### Overview
Lyapunov fractals visualize chaotic behavior of 1D iterative maps using the Lyapunov exponent. Each pixel's color represents stability/chaos of a logistic map sequence.

### Is it just UV math?
**Yes.** Per-pixel calculation of Lyapunov exponent for parameter values mapped from UV.

### Generates or modifies texture?
**Generates patterns from scratch.** Produces distinctive "zircon" or "Markus-Lyapunov" patterns.

### Computational cost
- **50-200 iterations per pixel**
- Includes logarithm calculation (expensive)
- Optimization: accumulate product during iteration, single log at end

```glsl
// Optimized: single log at end instead of per-iteration
float lyapunov(vec2 uv, int warmup, int iterations) {
    float a = 2.0 + uv.x * 2.0;  // Map to [2, 4]
    float b = 2.0 + uv.y * 2.0;

    float x = 0.5;
    float product = 1.0;

    // Warmup iterations (discard)
    for (int i = 0; i < warmup; i++) {
        float r = ((i & 1) == 0) ? a : b;  // Alternate a, b
        x = r * x * (1.0 - x);
    }

    // Accumulate for Lyapunov exponent
    for (int i = 0; i < iterations; i++) {
        float r = ((i & 1) == 0) ? a : b;
        float derivative = abs(r * (1.0 - 2.0 * x));
        product *= derivative;
        x = r * x * (1.0 - x);
    }

    return log(product) / float(iterations);
}
```

### Animation potential
**Limited.** The sequence pattern (AB, AABB, etc.) is discrete. Smooth animation requires:
- Interpolating between sequence patterns
- Animating the parameter space mapping
- Using the exponent value to modulate other effects

### Unique characteristics
- Produces organic, flowing boundaries
- Distinct visual style from escape-time fractals
- Strong contrast between stable (blue) and chaotic (yellow/red) regions

---

## Recommendations for AudioJones

### Best candidates for psychedelic post-processing

**1. Orbital Traps with texture sampling** (Primary recommendation)
- Feeds existing visualization through fractal iteration
- Multiple animation parameters map to audio
- Produces kaleidoscope-like distortion without hard edges
- Natural radial variation from fractal structure

**2. Julia Set UV distortion** (Simple, performant)
- Use iteration output to offset texture sampling
- Single `c` parameter maps to bass/treble
- Low iteration count (16-32) sufficient for distortion
- Very smooth animation

**3. Domain warping with fBM** (Organic movement)
- Not strictly fractal but produces similar aesthetics
- Stacked warping creates complex, flowing patterns
- Excellent for slow, evolving visual drift

### Implementation priority

1. **Orbital traps**: Most flexible, directly samples input texture
2. **Julia UV distortion**: Simple addition to existing shader pipeline
3. **Newton fractal overlay**: Blend generated pattern with visualization

### Performance budget
- Target: 60fps at 1080p
- Budget: ~10ms per frame for effects
- Safe iteration counts:
  - Julia/Newton: 32-64 iterations
  - Orbital traps: 32 iterations with texture sampling
  - Avoid: Lyapunov (expensive log), Fractal flames (histogram)

---

## Sources

- [Inigo Quilez - Geometric Orbit Traps](https://iquilezles.org/articles/ftrapsgeometric/)
- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)
- [Inigo Quilez - Lyapunov Fractals](https://iquilezles.org/articles/lyapunovfractals/)
- [The Book of Shaders - Fractal Brownian Motion](https://thebookofshaders.com/13/)
- [OGLplus Newton Fractal Tutorial](https://oglplus.org/oglplus/html/oglplus_tut_004_newton_fractal.html)
- [GPU Nonlinear IFS Rendering](https://www.cs.uaf.edu/~olawlor/2011/gpuifs/)
- [Softology - Orbit Traps](https://softologyblog.wordpress.com/2011/02/01/orbit-traps/)
- [Monogon - Orbit Trap Fractals](https://www.monogon.net/orbittrap/)
- [Fractalysis OrbitTrap.glsl](https://github.com/Hellenic/fractalysis/blob/master/public/shaders/OrbitTrap.glsl)
- [Wikipedia - Fractal Flame](https://en.wikipedia.org/wiki/Fractal_flame)
- [Fractal Lab](http://sub.blue/fractal-lab)
