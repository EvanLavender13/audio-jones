# Heavy Texture Transform Techniques

Research into shader techniques that heavily transform input textures without tiling or hard edges. Focus on kaleidoscope-like effects where output is unrecognizable from input.

## Requirements

- **Non-tiling**: No visible repetition of the original texture
- **Non-repetitive**: Output varies across screen space
- **No hard edges**: Smooth transitions, no visible seams or boundaries
- **Heavy transformation**: Input texture becomes unrecognizable
- **Real-time capable**: 60fps at 1080p
- **Texture-based**: Works with existing render pipeline (not procedural generation)

## Why Orbit Traps Failed

Orbit traps sample the input texture at each orbit point during fractal iteration. The texture appears at every orbit position = **visible tiling**. The original texture remains recognizable, just repeated in fractal patterns.

## Why Simple UV Warping Failed

Möbius, circle inversion, Poincaré disk, etc. transform WHERE you sample but don't mix/blend the texture. Output remains clearly derived from input, just stretched/compressed.

## What Makes Kaleidoscope KIFS Work

The existing kaleidoscope shader succeeds because it:
1. **Polar folding**: Mirrors angle into segments (not just warping)
2. **Iterative depth accumulation**: Samples at multiple iteration depths with weighted blending
3. **Smooth remapping**: Uses `sin()` to avoid hard boundaries
4. **Weight-based blending**: Earlier iterations contribute less, creating smooth falloff

The key insight: **accumulate blended samples from different depths**, not different spatial positions.

---

## Technique 1: Iterated Möbius with Depth Accumulation

Apply the KIFS accumulation pattern to Möbius transforms instead of polar folding.

### Core Algorithm

```glsl
vec2 cmul(vec2 a, vec2 b) { return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x); }
vec2 cdiv(vec2 a, vec2 b) { return cmul(a, vec2(b.x, -b.y)) / dot(b, b); }

vec2 mobius(vec2 z, vec2 a, vec2 b, vec2 c, vec2 d) {
    return cdiv(cmul(a, z) + b, cmul(c, z) + d);
}

vec4 iteratedMobius(vec2 uv, sampler2D tex, int iterations) {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 z = (uv - 0.5) * 2.0;

    for (int i = 0; i < iterations; i++) {
        // Animate Möbius parameters
        float t = time + float(i) * 0.5;
        vec2 a = vec2(cos(t * 0.3), sin(t * 0.3));
        vec2 b = vec2(0.0);
        vec2 c = vec2(0.1 * sin(t * 0.5), 0.1 * cos(t * 0.7));
        vec2 d = vec2(1.0, 0.0);

        z = mobius(z, a, b, c, d);

        // Smooth remap to UV space (no hard boundaries)
        vec2 sampleUV = 0.5 + 0.4 * sin(z * 0.5);

        // Depth-based weight (earlier iterations = less weight)
        float weight = 1.0 / float(i + 2);
        colorAccum += texture(tex, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Characteristics

- Smooth, flowing distortion from conformal mapping
- No folding symmetry (unlike kaleidoscope)
- Radially-varying due to Möbius pole behavior
- Animation through parameter interpolation

### Audio-Reactive Mapping

- Bass → magnitude of `a` (zoom intensity)
- Mids → angle of `a` (rotation speed)
- Treble → magnitude of `c` (pole proximity = stronger lensing)

### Sources

- [ubavic/mobius-shader](https://github.com/ubavic/mobius-shader)
- [Log Moebius Transfo psychedelic - Shadertoy](https://www.shadertoy.com/view/XdyXD3)

---

## Technique 2: Radial Streak Accumulation

Sample along radial or spiral paths with weighted blending. Creates directional smearing that varies with angle and radius.

### Core Algorithm

```glsl
vec4 radialStreak(vec2 uv, sampler2D tex, int samples, float streakLength) {
    vec2 center = vec2(0.5);
    vec2 dir = uv - center;
    float dist = length(dir);
    vec2 normDir = dir / max(dist, 0.001);

    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples);

        // Sample along radial line with distance falloff
        float sampleDist = dist * (1.0 - t * streakLength);
        vec2 sampleUV = center + normDir * sampleDist;

        // Add spiral twist based on sample position
        float angle = t * spiralTwist;
        sampleUV = center + rotate2d(sampleUV - center, angle);

        // Weight: inner samples contribute more
        float weight = 1.0 - t * 0.8;
        colorAccum += texture(tex, clamp(sampleUV, 0.0, 1.0)).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Variant: Spiral Streak

```glsl
vec4 spiralStreak(vec2 uv, sampler2D tex, int samples) {
    vec2 center = vec2(0.5);
    vec2 offset = uv - center;
    float r = length(offset);
    float a = atan(offset.y, offset.x);

    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples);

        // Spiral path: angle increases as radius decreases
        float sampleR = r * (1.0 - t * 0.5);
        float sampleA = a + t * spiralTurns * 6.28318;

        vec2 sampleUV = center + sampleR * vec2(cos(sampleA), sin(sampleA));

        float weight = 1.0 - t * 0.7;
        colorAccum += texture(tex, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Characteristics

- Creates motion-blur-like streaking from center
- Spiral variant adds rotational flow
- Naturally radially-varying
- Cheap: just multiple texture samples, no complex math

### Audio-Reactive Mapping

- Bass → `streakLength` (longer streaks on beats)
- Mids → `spiralTwist` (rotation per sample)
- Treble → sample count (more = smoother but heavier)

### Sources

- [Harry Alisavakis - Radial Blur](https://halisavakis.com/my-take-on-shaders-radial-blur/)
- [GM Shaders - Turbulence](https://mini.gmshaders.com/p/turbulence)
- [Shadertoy - Radial Blur](https://www.shadertoy.com/view/4sfGRn)

---

## Technique 3: Turbulence Cascade

Stack sine-based coordinate shifts with rotation and scaling. Creates organic swirl patterns that heavily distort the texture.

### Core Algorithm

```glsl
vec4 turbulenceCascade(vec2 uv, sampler2D tex, int octaves) {
    vec2 p = (uv - 0.5) * 2.0;

    // Cascade: shift, rotate, scale, repeat
    for (int i = 0; i < octaves; i++) {
        float freq = pow(2.0, float(i));
        float amp = 1.0 / freq;

        // Sine-based shift
        p.x += sin(p.y * freq + time) * amp * turbulenceStrength;
        p.y += sin(p.x * freq + time * 1.3) * amp * turbulenceStrength;

        // Rotation per octave
        float angle = float(i) * 0.5 + time * 0.1;
        float c = cos(angle), s = sin(angle);
        p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);
    }

    // Smooth remap to UV
    vec2 finalUV = 0.5 + 0.4 * sin(p * 0.3);
    return texture(tex, finalUV);
}
```

### Variant: Multi-Sample Accumulation

```glsl
vec4 turbulenceAccum(vec2 uv, sampler2D tex, int octaves) {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 p = (uv - 0.5) * 2.0;

    for (int i = 0; i < octaves; i++) {
        float freq = pow(2.0, float(i));
        float amp = 1.0 / freq;

        p.x += sin(p.y * freq + time) * amp * turbulenceStrength;
        p.y += sin(p.x * freq + time * 1.3) * amp * turbulenceStrength;

        // Sample at each octave depth
        vec2 sampleUV = 0.5 + 0.4 * sin(p * 0.3);
        float weight = amp;
        colorAccum += texture(tex, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Characteristics

- Creates fluid, swirling distortion
- Higher octaves = finer detail in swirls
- Accumulation variant blends multiple distortion levels
- Fast: just trig operations + texture samples

### Audio-Reactive Mapping

- Bass → `turbulenceStrength` (distortion intensity)
- Mids → rotation speed per octave
- Treble → number of octaves (detail level)

### Sources

- [GM Shaders - Turbulence](https://mini.gmshaders.com/p/turbulence)
- [Geeks3D - Swirl Post Processing](https://www.geeks3d.com/20110428/shader-library-swirl-post-processing-filter-in-glsl/)

---

## Technique 4: Multi-Inversion Blend

Chain circle inversions at different centers with depth-weighted blending.

### Core Algorithm

```glsl
vec2 circleInvert(vec2 p, vec2 center, float radius) {
    vec2 d = p - center;
    float r2 = radius * radius;
    return center + d * (r2 / dot(d, d));
}

vec4 multiInversion(vec2 uv, sampler2D tex, int iterations) {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 z = uv - 0.5;

    for (int i = 0; i < iterations; i++) {
        // Animated inversion center (Lissajous pattern)
        float t = time * 0.3 + float(i) * 1.2;
        vec2 center = vec2(sin(t * 0.7), cos(t * 1.1)) * 0.3;
        float radius = 0.2 + 0.1 * sin(t * 0.5);

        z = circleInvert(z, center, radius);

        // Smooth remap
        vec2 sampleUV = 0.5 + 0.4 * sin(z * 2.0);

        float weight = 1.0 / float(i + 2);
        colorAccum += texture(tex, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Characteristics

- Creates bulging/pinching distortion with depth
- Multiple inversions interact in complex ways
- Smooth: inversions are conformal (angle-preserving)
- Naturally radially-varying around inversion centers

### Audio-Reactive Mapping

- Bass → inversion radius (strength of bulge)
- Mids → center position (drift)
- Treble → number of iterations (complexity)

### Sources

- [Circle Inversion Fractals](https://sites.nova.edu/mjl/graphics/circles-and-spheres/circle-inversions/circle-inversion-fractals/)
- [OneShader - Circle Inversion](https://oneshader.net/shader/ab89ae6583)

---

## Technique 5: Log-Polar Spiral Blur

Convert to log-polar coordinates, apply spiral motion, blend multiple scales.

### Core Algorithm

```glsl
vec2 toLogPolar(vec2 p) {
    return vec2(log(length(p) + 0.001), atan(p.y, p.x));
}

vec2 fromLogPolar(vec2 lp) {
    return exp(lp.x) * vec2(cos(lp.y), sin(lp.y));
}

vec4 logPolarSpiral(vec2 uv, sampler2D tex, int layers) {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 centered = uv - 0.5;

    for (int i = 0; i < layers; i++) {
        float phase = fract((float(i) - time * speed) / float(layers));

        // Convert to log-polar
        vec2 lp = toLogPolar(centered);

        // Scale shift (zoom) based on layer
        lp.x += phase * zoomDepth;

        // Spiral twist: angle varies with log-radius
        lp.y += lp.x * spiralTwist + phase * spiralTurns * 6.28318;

        // Convert back
        vec2 warped = fromLogPolar(lp);
        vec2 sampleUV = warped + 0.5;

        // Bounds check with soft fade
        float edgeDist = min(min(sampleUV.x, 1.0 - sampleUV.x),
                            min(sampleUV.y, 1.0 - sampleUV.y));
        float edgeFade = smoothstep(0.0, 0.1, edgeDist);

        // Cosine fade between layers
        float alpha = (1.0 - cos(phase * 6.28318)) * 0.5 * edgeFade;

        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 &&
            sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
            colorAccum += texture(tex, sampleUV).rgb * alpha;
            weightAccum += alpha;
        }
    }

    return vec4(colorAccum / max(weightAccum, 0.001), 1.0);
}
```

### Characteristics

- Creates infinite zoom/spiral effect
- Log transform naturally compresses outer regions
- Multiple layers blend for smooth motion
- Similar to existing infinite_zoom but with spiral distortion

### Audio-Reactive Mapping

- Bass → `zoomDepth` (zoom range per cycle)
- Mids → `spiralTwist` (spiral tightness)
- Treble → `spiralTurns` (rotation per zoom cycle)

### Sources

- [Log-spherical Mapping in SDF Raymarching](https://www.osar.fr/notes/logspherical/)
- [Shadertoy - Logarithmic Spiral](https://www.shadertoy.com/view/MXVfRc)
- [Shadertoy - Quasi Infinite Zoom Voronoi](https://www.shadertoy.com/view/XlBXWw)

---

## Technique 6: Domain Warping with Texture Feedback

Apply Inigo Quilez's domain warping technique using the input texture itself (not procedural noise).

### Core Algorithm

```glsl
// Sample texture as displacement source
vec2 textureWarp(vec2 p, sampler2D tex, float scale, float strength) {
    vec4 sample = texture(tex, fract(p * scale));
    return vec2(sample.r - 0.5, sample.g - 0.5) * strength;
}

vec4 domainWarpTexture(vec2 uv, sampler2D tex) {
    vec2 p = uv;

    // First warp layer
    vec2 q = vec2(
        textureWarp(p + vec2(0.0, 0.0), tex, warpScale1, warpStrength1).x,
        textureWarp(p + vec2(0.31, 0.17), tex, warpScale1, warpStrength1).y
    );

    // Second warp layer (feeds back into itself)
    vec2 r = vec2(
        textureWarp(p + q + vec2(0.17, 0.92), tex, warpScale2, warpStrength2).x,
        textureWarp(p + q + vec2(0.83, 0.28), tex, warpScale2, warpStrength2).y
    );

    // Final sample with accumulated warp
    vec2 finalUV = p + r * finalWarpStrength;
    return texture(tex, fract(finalUV));
}
```

### Variant: Multi-Pass Accumulation

```glsl
vec4 domainWarpAccum(vec2 uv, sampler2D tex, int passes) {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 p = uv;

    for (int i = 0; i < passes; i++) {
        // Warp coordinates using texture itself
        vec4 sample = texture(tex, fract(p * warpScale));
        vec2 offset = (sample.rg - 0.5) * warpStrength;
        p += offset;

        // Sample at this warp depth
        vec2 sampleUV = 0.5 + 0.4 * sin(p * 2.0);
        float weight = 1.0 / float(i + 1);
        colorAccum += texture(tex, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Characteristics

- Uses texture's own structure to drive distortion
- Cascading warps compound complexity
- Non-repetitive by nature (depends on texture content)
- Can create organic, flowing patterns

### Audio-Reactive Mapping

- Bass → `warpStrength` (distortion intensity)
- Mids → `warpScale` (frequency of warp pattern)
- Treble → number of warp passes

### Sources

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)
- [The Book of Shaders - Fractal Brownian Motion](https://thebookofshaders.com/13/)

---

## Technique 7: Tunnel Effect (Demoscene Classic)

Pre-compute distance/angle lookup, animate texture coordinates through it.

### Core Algorithm

```glsl
vec4 tunnelEffect(vec2 uv, sampler2D tex) {
    vec2 centered = uv - 0.5;
    float dist = length(centered);
    float angle = atan(centered.y, centered.x);

    // Texture coordinates from polar transform
    // Distance → texture Y (scrolling creates depth illusion)
    // Angle → texture X (rotation creates spin)
    float texY = 1.0 / (dist + 0.1) + time * tunnelSpeed;
    float texX = angle / 6.28318 + time * rotationSpeed;

    // Spiral twist: angle varies with depth
    texX += texY * spiralTwist;

    vec2 tunnelUV = vec2(texX, texY);

    // Depth-based darkening (optional)
    float darkness = dist * 2.0;

    return texture(tex, fract(tunnelUV)) * (1.0 - darkness * darkenAmount);
}
```

### Variant: Accumulation (Multi-Depth Blend)

```glsl
vec4 tunnelAccum(vec2 uv, sampler2D tex, int depths) {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 centered = uv - 0.5;
    float dist = length(centered);
    float angle = atan(centered.y, centered.x);

    for (int i = 0; i < depths; i++) {
        float depthOffset = float(i) * depthSpacing;

        float texY = 1.0 / (dist + 0.1) + time * tunnelSpeed + depthOffset;
        float texX = angle / 6.28318 + texY * spiralTwist;

        vec2 tunnelUV = vec2(texX, texY);

        // Depth-based weight
        float weight = 1.0 / float(i + 1);
        colorAccum += texture(tex, fract(tunnelUV)).rgb * weight;
        weightAccum += weight;
    }

    return vec4(colorAccum / weightAccum, 1.0);
}
```

### Characteristics

- Classic demoscene effect, well-understood
- Creates strong depth illusion
- Polar transform naturally radially-varying
- Fast: mostly just coordinate transforms

### Audio-Reactive Mapping

- Bass → `tunnelSpeed` (depth motion)
- Mids → `spiralTwist` (spiral tightness)
- Treble → `rotationSpeed` (angular motion)

### Sources

- [Oldschool Demo Effects](https://seancode.com/demofx/)
- [Realtime Visualization Methods in the Demoscene](https://old.cescg.org/CESCG-2002/BBurger/index.html)

---

## Technique 8: Reaction-Diffusion Seeding

Use input texture to seed reaction-diffusion initial state. Evolves into organic patterns that heavily transform the original.

### Core Concept

Standard reaction-diffusion requires ping-pong buffers (already have in pipeline). Seed chemical B concentration from input texture brightness, then evolve.

### Implementation Notes

```glsl
// Initial state shader (run once when effect enabled)
void seedFromTexture(vec2 uv, sampler2D inputTex, out vec4 state) {
    vec4 input = texture(inputTex, uv);
    float brightness = dot(input.rgb, vec3(0.299, 0.587, 0.114));

    // Chemical A = 1.0 everywhere
    // Chemical B = seeded from texture brightness
    float a = 1.0;
    float b = brightness > threshold ? 1.0 : 0.0;

    state = vec4(a, b, 0.0, 1.0);
}

// Evolution shader (run each frame, ping-pong between buffers)
void evolve(vec2 uv, sampler2D prevState, out vec4 newState) {
    // Gray-Scott reaction-diffusion
    float dA = diffusionA;
    float dB = diffusionB;
    float feed = feedRate;
    float kill = killRate;

    vec4 center = texture(prevState, uv);
    float a = center.r;
    float b = center.g;

    // Laplacian (9-point stencil)
    float laplacianA = /* sum of neighbors */ - a;
    float laplacianB = /* sum of neighbors */ - b;

    float reaction = a * b * b;
    float newA = a + (dA * laplacianA - reaction + feed * (1.0 - a)) * dt;
    float newB = b + (dB * laplacianB + reaction - (kill + feed) * b) * dt;

    newState = vec4(newA, newB, 0.0, 1.0);
}
```

### Characteristics

- **Heavy transformation**: Input barely recognizable after evolution
- Organic, flowing patterns emerge
- Non-tiling, non-repetitive by nature
- Requires state management (ping-pong buffers)
- Can be slow if evolving many steps per frame

### Audio-Reactive Mapping

- Bass → `feedRate` (growth speed)
- Mids → `killRate` (pattern scale)
- Treble → diffusion ratio (pattern sharpness)

### Caveat

Reaction-diffusion evolves over time. For instant effect, needs pre-evolution or high iteration count per frame. May not be suitable for real-time varying input, better for feedback texture that persists.

### Sources

- [Reaction Diffusion on Shader](https://ciphrd.com/2019/08/24/reaction-diffusion-on-shader/)
- [Reaction-Diffusion Playground](https://github.com/jasonwebb/reaction-diffusion-playground)
- [Shadertoy - Reaction-Diffusion](https://www.shadertoy.com/view/3styzM)

---

## Comparison Matrix

| Technique | Heavy Transform | No Tiling | No Hard Edges | Radially Varying | Cost | Complexity |
|-----------|----------------|-----------|---------------|------------------|------|------------|
| Iterated Möbius | High | Yes | Yes | Yes (around poles) | Medium | Medium |
| Radial Streak | Medium | Yes | Yes | Yes (by design) | Low | Low |
| Turbulence Cascade | High | Yes | Yes | Partial | Low | Low |
| Multi-Inversion | High | Yes | Yes | Yes (around centers) | Medium | Medium |
| Log-Polar Spiral | Medium | Yes | Yes | Yes (by design) | Medium | Medium |
| Domain Warp Texture | High | Depends | Yes | Depends | Medium | Medium |
| Tunnel Effect | High | *Tiles* | Yes | Yes (by design) | Low | Low |
| Reaction-Diffusion | Very High | Yes | Yes | Depends on seed | High | High |

**Note**: Tunnel effect tiles along angle axis (wraps at 2π). Can mitigate by using non-repeating texture or blending multiple offset samples.

---

## Recommendations

### Primary Candidates

1. **Iterated Möbius with Accumulation** - Closest to KIFS approach, proven pattern
2. **Turbulence Cascade with Accumulation** - Simple, fast, organic results
3. **Log-Polar Spiral** - Natural extension of existing infinite_zoom

### Secondary Candidates

4. **Radial Streak** - Good for motion-like effects
5. **Multi-Inversion** - Interesting pole-based distortion

### Experimental

6. **Domain Warp Texture** - Results depend heavily on input texture
7. **Reaction-Diffusion** - Heavy transform but complex state management

---

## Implementation Priority

1. **Turbulence Cascade** - Simplest to implement, fast, good results
2. **Iterated Möbius** - Applies proven KIFS accumulation pattern
3. **Log-Polar Spiral** - Similar to infinite_zoom, extends naturally

All three can share the same accumulation/blending infrastructure. Start with turbulence to validate the approach, then add Möbius and log-polar variants.

---

## Sources

### Shadertoy
- [Quasi Infinite Zoom Voronoi](https://www.shadertoy.com/view/XlBXWw)
- [Spiral Distortion](https://www.shadertoy.com/view/XsfSzN)
- [Swirl Distortion Effect](https://www.shadertoy.com/view/Xscyzn)
- [Log Moebius Transfo psychedelic](https://www.shadertoy.com/view/XdyXD3)
- [Logarithmic Spiral](https://www.shadertoy.com/view/MXVfRc)
- [Reaction-Diffusion](https://www.shadertoy.com/view/3styzM)
- [Radial Blur](https://www.shadertoy.com/view/4sfGRn)
- [Time Accumulation](https://www.shadertoy.com/view/Xlt3DX)

### Tutorials & Articles
- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)
- [Inigo Quilez - Texture Repetition](https://iquilezles.org/articles/texturerepetition/)
- [GM Shaders - Turbulence](https://mini.gmshaders.com/p/turbulence)
- [Harry Alisavakis - Radial Blur](https://halisavakis.com/my-take-on-shaders-radial-blur/)
- [Geeks3D - Swirl Post Processing](https://www.geeks3d.com/20110428/shader-library-swirl-post-processing-filter-in-glsl/)
- [The Book of Shaders - fBM](https://thebookofshaders.com/13/)
- [Log-spherical Mapping](https://www.osar.fr/notes/logspherical/)
- [Catlike Coding - Texture Distortion](https://catlikecoding.com/unity/tutorials/flow/texture-distortion/)

### Demoscene & Classic Effects
- [Oldschool Demo Effects](https://seancode.com/demofx/)
- [Realtime Visualization in Demoscene](https://old.cescg.org/CESCG-2002/BBurger/index.html)
- [MilkDrop Preset Authoring](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html)

### Technical Research
- [GPU Nonlinear IFS Rendering](https://www.cs.uaf.edu/~olawlor/2011/gpuifs/)
- [Reaction-Diffusion Playground](https://github.com/jasonwebb/reaction-diffusion-playground)
- [ubavic/mobius-shader](https://github.com/ubavic/mobius-shader)
