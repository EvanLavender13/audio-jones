# Seamless Infinite Zoom via Layered Cross-Fade

Technique for creating infinite zoom effects from existing textures without hard geometric boundaries. Eliminates the harsh circle edges present in traditional Droste/kaleidoscope implementations.

## Problem Statement

Traditional infinite zoom approaches create visible artifacts:

| Approach | Artifact |
|----------|----------|
| Droste spiral | Hard circular boundary at inner/outer ring |
| Polar kaleidoscope | Sharp radial fold lines at segment edges |
| KIFS folding | Discontinuities at fold boundaries |
| Single-layer zoom | Texture edge clipping when UV exits [0,1] |

The `kaleidoscope.fs:157-159` hard boundary demonstrates this:
```glsl
if (radius > 0.5) { radius = 1.0 - radius; }  // Hard edge
```

## Solution: Multi-Layer Temporal Cross-Fade

Render multiple copies of the source texture at exponentially spaced zoom scales. Each layer fades in/out smoothly based on where it sits in the zoom cycle. No single layer ever reaches full opacity at a boundary—transitions are always gradual.

### Core Insight

A single zooming layer eventually hits boundaries (UV outside [0,1] or center singularity). By running N layers at staggered phases, one layer always occupies the "safe" middle zone at full opacity while boundary layers fade to zero.

## Algorithm

### Phase 1: Layer Parameter Calculation

For each layer `i` in `[0, N-1]`:

```
phase_i = fract((i - time * speed) / N)
```

- `phase` ranges `[0, 1)` and cycles continuously
- Each layer's phase offsets by `1/N` from neighbors
- As time advances, all phases shift together, wrapping at 1.0

### Phase 2: Scale Computation

Convert phase to zoom scale via exponential mapping:

```
scale_i = exp2(phase_i * N) * baseFrequency
```

- `exp2` creates exponential spacing (scale doubles each layer)
- Layer at `phase=0` has `scale=baseFrequency`
- Layer at `phase=1` has `scale=baseFrequency * 2^N`
- When phase wraps 1→0, scale jumps from max to min (handled by fade)

### Phase 3: Alpha Weight Computation

Smooth fade based on phase position:

**Cosine fade (smoothest):**
```
alpha_i = (1.0 - cos(phase_i * TWO_PI)) / 2.0
```

**Characteristics:**
- `phase=0`: alpha=0 (layer just appeared, fully transparent)
- `phase=0.5`: alpha=1 (layer at peak visibility)
- `phase=1`: alpha=0 (layer about to wrap, fully transparent)

**Alternative: Smoothstep fade:**
```
alpha_i = smoothstep(0.0, 0.3, phase_i) * smoothstep(1.0, 0.7, phase_i)
```

**Alternative: Linear with smoothing:**
```
alpha_i = phase_i * (1.0 - phase_i) * 4.0  // Parabolic, peak at 0.5
alpha_i = alpha_i * alpha_i * (3.0 - 2.0 * alpha_i)  // Hermite smooth
```

### Phase 4: UV Transformation

Transform UVs for each layer's zoom scale:

```
uv_i = (fragTexCoord - center) * scale_i + center
```

Where `center` is typically `vec2(0.5)` but can animate for focal drift.

### Phase 5: Sampling and Accumulation

```
color = vec3(0.0)
totalWeight = 0.0

for each layer i:
    if (alpha_i > threshold):  // Skip negligible layers
        sample = texture(source, uv_i)
        color += sample.rgb * alpha_i
        totalWeight += alpha_i

color /= totalWeight  // Normalize
```

### Phase 6: Edge Handling

When `uv_i` exits [0,1], options:

1. **Discard:** Don't contribute to accumulation (cleanest)
2. **Mirror:** `uv = 1.0 - abs(mod(uv, 2.0) - 1.0)`
3. **Clamp:** `uv = clamp(uv, 0.0, 1.0)` (creates static edges)
4. **Wrap:** `uv = fract(uv)` (tiles, may show seams)

For seamless zoom, **discard** works best—layers outside valid range have near-zero alpha anyway due to the phase-based weighting.

## Complete GLSL Implementation

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float speed;        // Zoom speed (1.0 = normal)
uniform float baseScale;    // Starting scale (0.5-2.0 typical)
uniform vec2 center;        // Zoom focal point (default 0.5, 0.5)
uniform int layers;         // Layer count (4-8 recommended)

const float TWO_PI = 6.28318530718;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;
    float L = float(layers);

    for (int i = 0; i < layers; i++) {
        // Phase: where this layer sits in zoom cycle [0,1)
        float phase = fract((float(i) - time * speed) / L);

        // Scale: exponential zoom factor
        float scale = exp2(phase * L) * baseScale;

        // Alpha: cosine fade (zero at phase boundaries, peak at 0.5)
        float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;

        // Skip layers with negligible contribution
        if (alpha < 0.001) continue;

        // Transform UVs
        vec2 uv = (fragTexCoord - center) / scale + center;

        // Bounds check: discard samples outside texture
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            continue;
        }

        // Edge softening: fade near boundaries
        float edgeFade = 1.0;
        float edgeDist = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
        edgeFade = smoothstep(0.0, 0.1, edgeDist);

        // Sample and accumulate
        vec3 sample = texture(texture0, uv).rgb;
        float weight = alpha * edgeFade;
        colorAccum += sample * weight;
        weightAccum += weight;
    }

    // Normalize and output
    if (weightAccum > 0.0) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
```

## Variations

### Spiral Zoom (Droste-style without hard edges)

Add rotation proportional to zoom phase:

```glsl
float angle = phase * TWO_PI * spiralTurns;
vec2 rotatedUV = fragTexCoord - center;
rotatedUV = vec2(
    rotatedUV.x * cos(angle) - rotatedUV.y * sin(angle),
    rotatedUV.x * sin(angle) + rotatedUV.y * cos(angle)
);
vec2 uv = rotatedUV / scale + center;
```

### Bi-directional Zoom

Half the layers zoom in, half zoom out:

```glsl
float direction = (i < layers / 2) ? 1.0 : -1.0;
float scale = exp2(phase * L * direction) * baseScale;
```

### Audio-Reactive Zoom Speed

Modulate speed by beat energy:

```glsl
float dynamicSpeed = speed * (1.0 + beatIntensity * 2.0);
float phase = fract((float(i) - time * dynamicSpeed) / L);
```

### Kaleidoscope + Infinite Zoom

Apply kaleidoscope transform to each layer's UV before sampling:

```glsl
// After computing uv for layer i:
vec2 centered = uv - 0.5;
float r = length(centered);
float a = atan(centered.y, centered.x);

float segmentAngle = TWO_PI / float(segments);
a = mod(a, segmentAngle);
a = min(a, segmentAngle - a);

uv = vec2(cos(a), sin(a)) * r + 0.5;
```

### Color Shift per Layer

Add depth perception via hue/saturation shift:

```glsl
vec3 sample = texture(texture0, uv).rgb;
float hueShift = phase * 0.1;  // Subtle color evolution with depth
sample = hueRotate(sample, hueShift);
```

## Parameter Tuning

| Parameter | Range | Effect |
|-----------|-------|--------|
| `layers` | 4-8 | More layers = smoother transitions, higher cost |
| `speed` | 0.1-2.0 | Zoom rate (negative = zoom out) |
| `baseScale` | 0.5-2.0 | Initial zoom level |
| `center` | [0,1]² | Focal point (animate for drift) |

### Layer Count Selection

| Layers | Smoothness | GPU Cost | Notes |
|--------|------------|----------|-------|
| 4 | Acceptable | 4 samples | Minimum for seamless |
| 6 | Good | 6 samples | Balanced default |
| 8 | Excellent | 8 samples | Best quality |
| 12+ | Diminishing returns | High | Only for slow zoom |

Rule of thumb: `layers >= log2(maxScale / minScale) * 2`

## Mathematical Foundation

### Why Exponential Scaling?

Human perception of zoom is logarithmic. Equal perceptual steps require multiplicative scale factors:

```
perceived_zoom = log(scale)
```

Exponential scale spacing (`exp2(phase * N)`) creates uniform perceptual motion.

### Why Cosine Weighting?

The cosine function `(1 - cos(x * 2π)) / 2`:
- Has zero derivative at endpoints (no sudden acceleration)
- Integrates to 1.0 over [0,1] (energy conservation)
- Symmetric around midpoint (balanced fade in/out)
- C∞ continuous (no discontinuities in any derivative)

Alternative weightings and their properties:

| Function | Smoothness | Energy | Use Case |
|----------|------------|--------|----------|
| Linear tent | C0 | 1.0 | Fast, visible edges |
| Quadratic | C1 | 0.67 | Acceptable smoothness |
| Cosine | C∞ | 1.0 | Best quality |
| Gaussian | C∞ | ~1.0 | Requires normalization |

### Phase Offset Mathematics

With N layers offset by 1/N:
- Adjacent layers: `|phase_i - phase_{i+1}| = 1/N`
- At any time t, exactly one layer at phase ≈ 0.5 (peak alpha)
- Layers at phase ≈ 0 or ≈ 1 have alpha ≈ 0

This guarantees continuous coverage without gaps or double-bright regions.

## Performance Considerations

### GPU Cost

Per-pixel operations: `N * (transform + sample + blend)`

| Operation | Approx. Cost |
|-----------|--------------|
| Phase/scale computation | 10 ALU |
| UV transform | 6 ALU |
| Texture sample | 4-16 cycles (dependent) |
| Alpha blend | 4 ALU |
| **Total per layer** | ~20-30 cycles |

For 6 layers at 1080p: ~180 cycles/pixel × 2M pixels = 360M cycles/frame
At 1.5 GHz GPU: ~4ms/frame (well within 16.6ms budget)

### Optimization Strategies

1. **Early-out on alpha threshold:** Skip layers with `alpha < 0.01`
2. **Reduce layers at low zoom speeds:** Fewer layers needed when motion is slow
3. **Half-resolution source:** Sample from downscaled texture for distant layers
4. **Temporal stability:** Use previous frame's result for layers at extreme scales

## Integration with AudioJones

### As Post-Process Effect

Add to `PostEffectType` enum and pipeline:

```cpp
// post_effect.h
struct InfiniteZoomConfig {
    float speed = 1.0f;
    float baseScale = 1.0f;
    Vector2 center = {0.5f, 0.5f};
    int layers = 6;
    bool spiralEnabled = false;
    float spiralTurns = 1.0f;
};
```

### Shader Uniforms

```cpp
SetShaderValue(shader, GetShaderLocation(shader, "time"), &time, SHADER_UNIFORM_FLOAT);
SetShaderValue(shader, GetShaderLocation(shader, "speed"), &config.speed, SHADER_UNIFORM_FLOAT);
SetShaderValue(shader, GetShaderLocation(shader, "baseScale"), &config.baseScale, SHADER_UNIFORM_FLOAT);
SetShaderValue(shader, GetShaderLocation(shader, "center"), &config.center, SHADER_UNIFORM_VEC2);
SetShaderValue(shader, GetShaderLocation(shader, "layers"), &config.layers, SHADER_UNIFORM_INT);
```

### Modulation Targets

| Parameter | Modulation Source | Effect |
|-----------|-------------------|--------|
| `speed` | Beat energy | Zoom pulses on beats |
| `center.x` | Bass frequency | Horizontal drift |
| `center.y` | Treble energy | Vertical drift |
| `baseScale` | RMS amplitude | Breathing zoom |

## Comparison with Existing Techniques

| Technique | Seams | True Infinite | Texture Input | Complexity |
|-----------|-------|---------------|---------------|------------|
| Polar kaleidoscope | Radial lines | No | Yes | Low |
| KIFS fractal | Fold lines | Visual only | Yes | Medium |
| Droste spiral | Circle edge | Yes | Yes | Medium |
| **Layered cross-fade** | **None** | **Yes** | **Yes** | Medium |
| IFS attractor | None | True infinite | No (generates) | High |

The layered cross-fade technique uniquely combines:
- No visible seams or hard edges
- Works with arbitrary input textures
- True continuous zoom (not just visual recursion)
- Moderate computational cost

## Sources

- [Quasi Infinite Zoom Voronoi - libretro](https://github.com/libretro/glsl-shaders/blob/master/procedural/shane-quasi-infinite-zoom-voronoi.glsl) - Layer accumulation technique
- [Infinite Mandelbrot Zoom - OneShader](https://oneshader.net/shader/6a54c2c771) - Cross-fade weight calculation
- [GLSL Infinite Voronoi Zoom - CodePen](https://codepen.io/shubniggurath/pen/JMzQRw) - Smoothstep opacity control
- [Infinite Zoom - Shadertoy](https://www.shadertoy.com/view/tt23WK) - Fractional time cycling
- [CobraFX Droste - GitHub](https://github.com/LordKobra/CobraFX/blob/master/Shaders/Droste.fx) - Logarithmic distance mapping
- [Fabrice Neyret - Infinite Fall](https://www.shadertoy.com/view/XlBXWw) - Original layer cycling technique
