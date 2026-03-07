# Infinite Zoom v2

Extends the existing Infinite Zoom effect with per-layer UV warping, movable zoom center with lissajous motion, parallax offset with lissajous drift, and selectable layer blend modes. Addresses the "muddying" problem of weighted-average-only blending and adds organic motion and depth.

## Classification

- **Category**: TRANSFORMS > Motion
- **Pipeline Position**: Transform stage (existing effect, same slot)
- **Scope**: Extension of existing `infinite_zoom.h/.cpp/.fs`

## References

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - fbm(p + fbm(p)) recursive warp technique
- [Inigo Quilez - fBM](https://iquilezles.org/articles/fbm/) - Fractal Brownian Motion with optimized octave loop
- Existing codebase: `shaders/texture_warp.fs` hash/noise primitives, `src/config/dual_lissajous_config.h`

## Reference Code

### Value noise (from texture_warp.fs, already in codebase)

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
```

### fBM (from IQ, optimized form)

```glsl
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {  // 4 octaves hardcoded
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
```

### Sine warp per layer

```glsl
// Applied to centered UV before texture sampling
uv.x += sin(uv.y * warpFreq + warpTime) * warpStrength / scale;
uv.y += sin(uv.x * warpFreq * 1.3 + warpTime * 0.7) * warpStrength / scale;
```

### Noise warp per layer

```glsl
// fbm-based displacement, attenuated by scale
vec2 warpOffset = vec2(
    fbm(uv * warpFreq + warpTime),
    fbm(uv * warpFreq + warpTime + vec2(5.2, 1.3))
) - 0.5;  // Center around zero
uv += warpOffset * warpStrength / scale;
```

### Blend modes

```glsl
// Weighted Average (current)
colorAccum += sampleColor * weight;
weightAccum += weight;
// final: colorAccum / weightAccum

// Additive
colorAccum += sampleColor * weight;
// final: clamp(colorAccum, 0.0, 1.0)

// Screen
// Per-layer accumulation: result = 1 - (1-result) * (1 - sample*weight)
colorAccum = 1.0 - (1.0 - colorAccum) * (1.0 - sampleColor * weight);
// final: colorAccum (no normalization needed)
```

## Algorithm

### Shader changes (infinite_zoom.fs)

Add noise primitives (`hash`, `noise`, `fbm`) at top of shader. These are copy-paste from `texture_warp.fs`.

Add new uniforms:
- `vec2 center` replaces hardcoded `vec2(0.5)`
- `vec2 offset` for parallax direction
- `int warpType` (0=None, 1=Sine, 2=Noise)
- `float warpStrength`, `float warpFreq`, `float warpTime`
- `int blendMode` (0=WeightedAvg, 1=Additive, 2=Screen)

In the layer loop, after scale and rotation but before back-to-texture-space:

```
if warpType == 1: apply sine warp (divided by scale)
if warpType == 2: apply noise warp (divided by scale)
```

The `/scale` attenuation is key: deeper layers get less warp, which creates natural depth perspective where nearby layers distort more.

For parallax, before the existing `uv = fragTexCoord - center` line, offset center:
```
vec2 layerCenter = center + offset * float(i);
uv = fragTexCoord - layerCenter;
```
Each layer's center shifts by `offset * layerIndex`, creating parallax separation. Layer 0 samples near center, layer N samples offset by `N * offset`.

For blend modes, replace the accumulation and finalization with a switch on `blendMode`.

### CPU changes

- `InfiniteZoomConfig`: add `centerX`, `centerY`, `DualLissajousConfig centerLissajous`, `offsetX`, `offsetY`, `DualLissajousConfig offsetLissajous`, `warpType` (int), `warpStrength`, `warpFreq`, `warpSpeed`, `blendMode` (int)
- `InfiniteZoomEffect`: add uniform locations for new params, add `warpTime` accumulator
- `InfiniteZoomEffectSetup`: compute lissajous offsets for center and offset, accumulate `warpTime += warpSpeed * deltaTime`, set all new uniforms
- `InfiniteZoomRegisterParams`: register `warpStrength`, `warpFreq`, center lissajous, offset lissajous params
- UI: add Warp section (type combo, strength/freq/speed sliders), Center section (X/Y sliders + lissajous controls), Parallax section (X/Y sliders + lissajous controls), blend mode combo

## Parameters

### Existing (unchanged)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `speed` | float | -2.0 to 2.0 | 1.0 | Zoom animation rate |
| `zoomDepth` | float | 1.0-5.0 | 3.0 | Depth multiplier per cycle |
| `layers` | int | 2-8 | 6 | Blended zoom layers |
| `spiralAngle` | float | +-pi | 0.0 | Base rotation offset |
| `spiralTwist` | float | +-pi | 0.0 | Per-layer cumulative rotation |
| `layerRotate` | float | +-pi | 0.0 | Fixed rotation per layer index |

### New: Warp

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `warpType` | int (enum) | 0-2 | 0 | None / Sine / Noise |
| `warpStrength` | float | 0.0-1.0 | 0.0 | Distortion amplitude (attenuated by depth) |
| `warpFreq` | float | 1.0-10.0 | 3.0 | Spatial frequency of warp |
| `warpSpeed` | float | -2.0 to 2.0 | 0.5 | Warp animation rate |

### New: Center

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `centerX` | float | 0.0-1.0 | 0.5 | Zoom focal point X |
| `centerY` | float | 0.0-1.0 | 0.5 | Zoom focal point Y |
| `centerLissajous` | DualLissajousConfig | - | defaults | Orbital motion for center |

### New: Parallax

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `offsetX` | float | -0.1 to 0.1 | 0.0 | Per-layer X drift |
| `offsetY` | float | -0.1 to 0.1 | 0.0 | Per-layer Y drift |
| `offsetLissajous` | DualLissajousConfig | - | defaults | Orbital drift for parallax |

### New: Blending

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `blendMode` | int (enum) | 0-2 | 0 | Weighted Avg / Additive / Screen |

## Modulation Candidates

- **warpStrength**: pulsing distortion intensity. Zero = clean zoom, high = organic warped tunnel
- **warpFreq**: breathing spatial scale of distortion
- **centerLissajous.amplitude**: how far the zoom center wanders
- **centerLissajous.motionSpeed**: orbital speed of zoom center
- **offsetLissajous.amplitude**: how much parallax drift
- **offsetX/offsetY**: static parallax direction bias

### Interaction Patterns

- **Competing forces: warpStrength vs spiralAngle** — Spiral rotation creates geometric order; warp creates organic chaos. When both are modulated by different audio bands, clean geometric spirals emerge in quiet sections while loud sections dissolve into warped tunnels.
- **Cascading threshold: blendMode + warpStrength** — In additive mode, warp creates bright caustic-like patterns where distorted layers overlap. These bright spots only appear when warpStrength is high enough for layers to fold over each other. Low warp + additive = gentle glow; high warp + additive = intense bright streaks.
- **Resonance: offsetLissajous + centerLissajous** — When both orbital motions align (similar frequencies), the parallax direction tracks the zoom center, creating coherent directional motion. When misaligned, the parallax fights the center creating a disorienting shifting depth field.

## Notes

- Warp division by `scale` is essential: without it, deep (zoomed-out) layers get the same warp magnitude as close layers, breaking the depth illusion
- Noise warp uses 4 hardcoded fBM octaves — no need to expose octave count as a parameter since this is a secondary effect layered on top of the zoom
- The `offset * float(i)` parallax model is linear with layer index, not with depth scale. This keeps the parallax visually even across layers regardless of zoom depth setting
- Screen blend's per-layer formula `1 - (1-a)*(1-b*w)` naturally saturates toward white without exceeding it, making it safe without clamping
- Lissajous configs on CPU side compute offsets that get passed as vec2 uniforms — no shader-side lissajous math needed
