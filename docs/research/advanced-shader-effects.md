# Advanced Shader Effects Research

Background research for reactive visual effects: reaction-diffusion, SDF morphing, and domain warping.

## 1. Reaction-Diffusion (Gray-Scott Model)

### Core Concept

Gray-Scott simulates two chemicals (A and B) diffusing across a 2D grid. Chemical B converts A into more B. A continuously enters the system (feed rate). B continuously exits (kill rate). Both chemicals spread at different diffusion rates.

The simulation produces organic patterns: spots, stripes, fingerprints, coral, maze structures. Pattern type depends entirely on feed/kill parameter combination.

### Update Equations

Per-pixel per-timestep:

```
A' = A + (DA * laplacian(A) - A*B*B + f*(1-A)) * dt
B' = B + (DB * laplacian(B) + A*B*B - (k+f)*B) * dt
```

Where:
- `DA`, `DB` = diffusion rates (typically 1.0 and 0.5)
- `f` = feed rate (0.01 to 0.1)
- `k` = kill rate (0.045 to 0.07)
- `laplacian` = 3x3 convolution with center -1, adjacent 0.2, diagonals 0.05

### Parameter Space

| Pattern | Feed (f) | Kill (k) | Visual |
|---------|----------|----------|--------|
| Mitosis | 0.0367 | 0.0649 | Dividing cells |
| Fingerprint | 0.0545 | 0.062 | Parallel stripes |
| Coral | 0.055 | 0.062 | Branching structures |
| Spots | 0.038 | 0.099 | Isolated dots |
| Maze | 0.029 | 0.057 | Connected pathways |

Small parameter changes (0.001) produce drastically different patterns. A crescent-shaped region in f/k space generates complex behaviors; outside yields uniform A or B.

### GPU Implementation

Requires texture ping-ponging (identical to existing `accumTexture`/`tempTexture` approach):

```glsl
// Fragment shader - one iteration
uniform sampler2D uState;      // Previous frame (RG = A, B)
uniform vec2 uResolution;
uniform float uFeed, uKill;
uniform float uDA, uDB;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;

    vec2 state = texture(uState, uv).rg;
    float A = state.r;
    float B = state.g;

    // 3x3 Laplacian (center -1, adjacent 0.2, diagonal 0.05)
    float lapA = -A;
    float lapB = -B;

    lapA += 0.2 * texture(uState, uv + vec2(texel.x, 0)).r;
    lapA += 0.2 * texture(uState, uv - vec2(texel.x, 0)).r;
    lapA += 0.2 * texture(uState, uv + vec2(0, texel.y)).r;
    lapA += 0.2 * texture(uState, uv - vec2(0, texel.y)).r;
    lapA += 0.05 * texture(uState, uv + vec2(texel.x, texel.y)).r;
    lapA += 0.05 * texture(uState, uv - vec2(texel.x, texel.y)).r;
    lapA += 0.05 * texture(uState, uv + vec2(texel.x, -texel.y)).r;
    lapA += 0.05 * texture(uState, uv - vec2(texel.x, -texel.y)).r;
    // Same for lapB...

    float ABB = A * B * B;
    float newA = A + uDA * lapA - ABB + uFeed * (1.0 - A);
    float newB = B + uDB * lapB + ABB - (uKill + uFeed) * B;

    fragColor = vec4(newA, newB, 0.0, 1.0);
}
```

### Audio-Reactive Integration

| Audio Signal | Modulates | Effect |
|--------------|-----------|--------|
| Bass energy | Feed rate | Triggers pattern formation bursts |
| Beat detection | Kill rate spike | Dissolves existing patterns |
| Spectral centroid | f/k balance | Shifts pattern type over time |
| Amplitude | Diffusion ratio | Sharpens or blurs structure |

Initialization: seed B at waveform positions or beat-triggered locations. Patterns grow organically from audio-driven injection points.

### Performance

- Fragment shader: one pass per simulation step
- Multiple steps per frame increase pattern speed (4-8 typical)
- Cost comparable to blur pass (9 texture fetches + arithmetic)
- Compute shader variant runs 2-3x faster but adds pipeline complexity

### Sources

- [Karl Sims - Reaction-Diffusion Tutorial](https://www.karlsims.com/rd.html)
- [MROB Xmorphia - Pearson's Parameterization](http://www.mrob.com/pub/comp/xmorphia/index.html)
- [Biological Modeling - Gray-Scott](https://biologicalmodeling.org/prologue/gray-scott)
- [ciphrd - Reaction Diffusion on Shader](https://ciphrd.com/2019/08/24/reaction-diffusion-on-shader/)
- [Codrops - WebGPU Compute Shader](https://tympanus.net/codrops/2024/05/01/reaction-diffusion-compute-shader-in-webgpu/)

---

## 2. SDF Shape Morphing

### Core Concept

Signed distance fields encode shapes as distance-to-surface at every point. Negative values indicate interior, positive indicates exterior, zero marks the boundary. SDFs enable smooth blending between arbitrary shapes through simple arithmetic.

### 2D SDF Primitives

```glsl
// Circle: radius r centered at origin
float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

// Box: half-dimensions b
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// Equilateral triangle: radius r
float sdTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r / k;
    if (p.x + k*p.y > 0.0) p = vec2(p.x - k*p.y, -k*p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0*r, 0.0);
    return -length(p) * sign(p.y);
}

// N-pointed star: outer radius r, inner ratio m (0-1)
float sdStar(vec2 p, float r, int n, float m) {
    float an = 3.141593 / float(n);
    float en = 3.141593 / m;
    vec2 acs = vec2(cos(an), sin(an));
    vec2 ecs = vec2(cos(en), sin(en));
    float bn = mod(atan(p.x, p.y), 2.0*an) - an;
    p = length(p) * vec2(cos(bn), abs(sin(bn)));
    p -= r * acs;
    p += ecs * clamp(-dot(p, ecs), 0.0, r * acs.y / ecs.y);
    return length(p) * sign(p.x);
}
```

### Shape Operations

**Union** (combine shapes):
```glsl
float opUnion(float d1, float d2) {
    return min(d1, d2);
}
```

**Intersection** (overlap only):
```glsl
float opIntersection(float d1, float d2) {
    return max(d1, d2);
}
```

**Subtraction** (cut away):
```glsl
float opSubtraction(float d1, float d2) {
    return max(d1, -d2);
}
```

### Smooth Blending (smin)

Smooth minimum creates organic transitions between shapes. Parameter k controls blend radius in distance units.

**Polynomial (quadratic)** - fast, most common:
```glsl
float smin(float a, float b, float k) {
    k *= 4.0;
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}
```

**Polynomial (cubic)** - smoother profile:
```glsl
float smin(float a, float b, float k) {
    k *= 6.0;
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * h * k * (1.0/6.0);
}
```

**Exponential** - associative (order-independent):
```glsl
float smin(float a, float b, float k) {
    float r = exp2(-a/k) + exp2(-b/k);
    return -k * log2(r);
}
```

### Shape Morphing

Linear interpolation between two SDFs creates smooth transformation:

```glsl
float morphShapes(vec2 p, float t) {
    float d1 = sdCircle(p, 0.3);
    float d2 = sdStar(p, 0.35, 5, 0.4);
    return mix(d1, d2, t);  // t: 0=circle, 1=star
}
```

Smooth-min morphing (more organic):

```glsl
float morphSmooth(vec2 p, float t) {
    float d1 = sdCircle(p, 0.3);
    float d2 = sdStar(p, 0.35, 5, 0.4);
    float k = 0.3 * (1.0 - abs(2.0*t - 1.0));  // Max blend at t=0.5
    return smin(d1 * (1.0-t), d2 * t, k);
}
```

### Rendering SDF to Color

```glsl
vec3 renderSDF(float d) {
    // Sharp edge
    float mask = step(0.0, -d);

    // Anti-aliased edge (1px feather)
    float aa = fwidth(d);
    float maskAA = smoothstep(aa, -aa, d);

    // Glowing edge
    float glow = 0.02 / (abs(d) + 0.02);

    return vec3(maskAA) + vec3(0.2, 0.5, 1.0) * glow;
}
```

### Shape Modifiers

**Rounding** (soften corners):
```glsl
float sdRounded(float d, float r) {
    return d - r;
}
```

**Onion** (hollow shell):
```glsl
float sdOnion(float d, float thickness) {
    return abs(d) - thickness;
}
```

**Repetition** (infinite tiling):
```glsl
vec2 opRepeat(vec2 p, vec2 spacing) {
    return mod(p + 0.5*spacing, spacing) - 0.5*spacing;
}
```

### Audio-Reactive Integration

| Audio Signal | Modulates | Effect |
|--------------|-----------|--------|
| Beat intensity | Morph t | Snaps between shapes on beat |
| Bass | smin k | Shapes blob together on bass |
| Treble | Star points | More complex geometry |
| Amplitude | Shape scale | Breathing/pulsing |
| FFT bands | Multiple shapes | Spectral shape array |

### Performance

- Per-pixel cost: ~10-30 ALU ops depending on primitive complexity
- No texture fetches required (pure math)
- Star/polygon primitives more expensive than circle/box
- smin adds ~5 ops per blend

### Sources

- [Inigo Quilez - 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/)
- [Inigo Quilez - Smooth Minimum](https://iquilezles.org/articles/smin/)
- [Ronja's Tutorials - 2D SDF Basics](https://www.ronja-tutorials.com/post/034-2d-sdf-basics/)
- [Ronja's Tutorials - 2D SDF Combination](https://www.ronja-tutorials.com/post/035-2d-sdf-combination/)
- [Shadertoy - 2D SDF Primitives Playlist](https://www.shadertoy.com/playlist/MXdSRf)

---

## 3. Domain Warping / FBM

### Core Concept

Domain warping distorts coordinate space before evaluating a pattern function. Instead of `f(p)`, compute `f(p + h(p))` where h displaces each point. When h uses noise (fBM), organic flowing distortions result.

Fractal Brownian Motion (fBM) sums multiple noise octaves with increasing frequency and decreasing amplitude, producing natural-looking complexity at multiple scales.

### FBM Implementation

```glsl
// Standard fBM with configurable parameters
float fbm(vec2 p, int octaves, float lacunarity, float gain) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= lacunarity;  // Typically 2.0
        amplitude *= gain;        // Typically 0.5
    }
    return value;
}
```

### FBM Parameters

| Parameter | Typical Value | Effect |
|-----------|---------------|--------|
| Octaves | 4-8 | Detail levels (more = finer detail) |
| Lacunarity | 2.0 | Frequency multiplier per octave |
| Gain | 0.5 | Amplitude multiplier per octave |

**Lacunarity 2.0**: each octave doubles frequency (standard fractal).
**Gain 0.5**: each octave halves amplitude (energy-preserving).

### Noise Functions

**Value noise** (cheapest):
```glsl
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
```

**Simplex noise** (better gradients, ~2x slower): use external library or precomputed texture.

### Domain Warping Layers

**Single warp** (basic distortion):
```glsl
float pattern(vec2 p) {
    vec2 q = vec2(
        fbm(p + vec2(0.0, 0.0)),
        fbm(p + vec2(5.2, 1.3))
    );
    return fbm(p + 4.0 * q);
}
```

**Double warp** (complex organic flow):
```glsl
float pattern(vec2 p) {
    vec2 q = vec2(
        fbm(p + vec2(0.0, 0.0)),
        fbm(p + vec2(5.2, 1.3))
    );
    vec2 r = vec2(
        fbm(p + 4.0*q + vec2(1.7, 9.2)),
        fbm(p + 4.0*q + vec2(8.3, 2.8))
    );
    return fbm(p + 4.0 * r);
}
```

The offset constants (5.2, 1.3, etc.) break correlation between x/y components. Any arbitrary values work; different values produce different patterns.

**Triple warp** (maximum complexity):
```glsl
// f(p + fbm(p + fbm(p + fbm(p))))
```

Each layer adds ~4-8 fBM evaluations. Diminishing returns beyond 2-3 layers.

### Turbulence Variant

Absolute value of noise creates sharp creases:

```glsl
float turbulence(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * abs(noise(p * frequency));
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
```

Produces fire, marble, wood grain textures.

### Animation

**Time-based flow**:
```glsl
float animatedPattern(vec2 p, float time) {
    vec2 q = vec2(
        fbm(p + vec2(0.0, time * 0.1)),
        fbm(p + vec2(5.2, 1.3 + time * 0.1))
    );
    return fbm(p + 4.0 * q);
}
```

**3D noise slice** (smoother animation):
```glsl
float pattern3D(vec2 p, float time) {
    return fbm3D(vec3(p, time * 0.3));
}
```

### Audio-Reactive Integration

| Audio Signal | Modulates | Effect |
|--------------|-----------|--------|
| Bass | Warp strength (4.0) | Stronger bass = more distortion |
| Beat | Time offset | Jump animation on beat |
| Spectral centroid | Lacunarity | Brighter = finer detail |
| Amplitude | Overall scale | Zoom in/out with volume |
| FFT bands | Per-octave amplitude | Spectral detail control |

### Combining with Existing Effects

Domain warping integrates with current pipeline:

1. **Warp feedback UV**: Apply warp to `feedback.fs` sample coordinates
2. **Warp voronoi seed positions**: Organic cell movement
3. **Warp physarum sensor directions**: Agent behavior modulation
4. **Warp accumulation output**: Final image distortion

### Performance

| Configuration | Texture Fetches | ALU Ops |
|---------------|-----------------|---------|
| 4 octaves, no warp | 4 | ~50 |
| 4 octaves, 1 warp | 12 | ~150 |
| 4 octaves, 2 warp | 28 | ~350 |
| 6 octaves, 2 warp | 42 | ~500 |

Cost scales linearly with octaves and exponentially with warp layers. 4 octaves with single warp provides good quality/performance balance.

### Sources

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)
- [Inigo Quilez - fBM](https://iquilezles.org/articles/fbm/)
- [The Book of Shaders - Fractal Brownian Motion](https://thebookofshaders.com/13/)
- [Godot Shaders - FBM Snippet](https://godotshaders.com/snippet/fractal-brownian-motion-fbm/)

---

## Integration Priority Assessment

### Reaction-Diffusion

**Pros:**
- Produces unique organic patterns unlike existing effects
- Fits ping-pong texture architecture
- Audio can drive pattern injection and parameter shifts

**Cons:**
- Requires multiple simulation steps per frame (4-8)
- New texture pair for R-D state (separate from accumulation)
- Pattern growth rate requires tuning for audio sync

**Integration complexity:** Medium. New compute pass, new texture pair, new config struct.

### SDF Morphing

**Pros:**
- Pure math (no texture fetches)
- Integrates as alternative waveform renderer
- Beat-driven morphing straightforward
- Enables procedural shape variety

**Cons:**
- Replaces rather than enhances existing waveform rendering
- Complex shapes (star, polygon) require careful tuning
- Anti-aliasing adds fwidth() dependency

**Integration complexity:** Low. Fragment shader only, no pipeline changes.

### Domain Warping

**Pros:**
- Applies to existing effects (feedback, voronoi, output)
- Configurable complexity via octaves/layers
- Creates organic flowing distortion
- Works as UV modifier (minimal pipeline change)

**Cons:**
- High ALU cost with multiple warp layers
- Adds latency to affected passes
- Requires noise function (texture or procedural)

**Integration complexity:** Low-Medium. UV coordinate modification in existing shaders.

---

## Recommended Next Steps

1. **Domain warping** in `feedback.fs` - lowest risk, enhances existing effect
2. **SDF shapes** as waveform option - isolated change, clear audio mapping
3. **Reaction-diffusion** as new effect layer - largest scope, highest reward

TBD: Benchmark performance impact of each approach on target hardware.
