# Depth Layers

A general-purpose multi-layer compositing effect. Renders multiple copies of the input texture at varying depths with independent scale, rotation, and offset, then blends them together. Reproduces Infinite Zoom and enables parallax, ghosting, and cycling depth effects.

## Classification

- **Category**: TRANSFORMS → Motion
- **Core Operation**: Multi-sample UV transform with weighted blending
- **Pipeline Position**: Transform stage (user-ordered with other transforms)

## Design Philosophy

**Orthogonal to General Warp:** This effect handles layering and compositing only. No UV distortion—that's General Warp's job. Chain them: `Warp → Depth Layers` or `Depth Layers → Warp`.

**What it does:** Sample the input multiple times at different scales/rotations/offsets, weight each sample by depth, blend them together, optionally cycle through depths over time.

## References

- [Alan Zucconi - Parallax Shaders](https://www.alanzucconi.com/2019/01/01/parallax-shader/) - Depth-based UV offset techniques
- [Harry Alisavakis - Radial Blur](https://halisavakis.com/my-take-on-shaders-radial-blur/) - Multi-sample radial motion blur

## Algorithm

### Core Loop

```glsl
vec3 colorAccum = vec3(0.0);
float weightAccum = 0.0;

for (int i = 0; i < layers; i++) {
    // 1. Compute depth value for this layer
    float depth = depthFunction(i, layers, time, cycleSpeed);

    // 2. Compute weight (contribution) based on depth
    float weight = weightFunction(depth, falloffType, falloffPower);

    // 3. Transform UV for this layer
    vec2 layerUV = uv;
    layerUV = applyOffset(layerUV, center, depth, offsetX, offsetY);
    layerUV = applyScale(layerUV, center, depth);
    layerUV = applyRotation(layerUV, center, depth, rotationTwist);

    // 4. Optional: cascade mode feeds UV to next iteration
    if (cascade) uv = layerUV;

    // 5. Bounds handling
    if (outOfBounds(layerUV, boundsMode)) continue;

    // 6. Sample and accumulate
    vec3 sample = texture(inputTex, layerUV).rgb;
    colorAccum = blend(colorAccum, sample, weight, blendMode);
    weightAccum += weight;
}

// Normalize for weighted average mode
if (blendMode == WEIGHTED_AVERAGE && weightAccum > 0.0) {
    colorAccum /= weightAccum;
}
```

### Depth Function

```glsl
float depthFunction(int i, int layers, float time, float cycleSpeed) {
    float phase = float(i) / float(layers);

    // Cycle layers through depth over time (creates infinite zoom)
    if (cycleSpeed != 0.0) {
        phase = fract(phase + time * cycleSpeed);
    }

    switch (depthCurve) {
        case EXPONENTIAL:  return exp2(phase * depthScale);
        case LINEAR:       return 1.0 + phase * depthScale;
        case LOGARITHMIC:  return log2(1.0 + phase * depthScale);
        case INVERSE:      return 1.0 / (1.0 + phase * depthScale);
    }
}
```

### Weight Function

```glsl
float weightFunction(float depth, int falloffType, float power) {
    switch (falloffType) {
        case LINEAR:       return max(0.0, 1.0 - depth * power);
        case INVERSE:      return 1.0 / (depth * power);
        case INVERSE_SQ:   return 1.0 / (depth * depth * power);
        case EXPONENTIAL:  return exp(-depth * power);
        case GAUSSIAN:     return exp(-pow(depth - focusDepth, 2.0) / (2.0 * power * power));
    }
}
```

### Transform Functions

```glsl
vec2 applyScale(vec2 uv, vec2 center, float depth) {
    return center + (uv - center) / depth;
}

vec2 applyOffset(vec2 uv, vec2 center, float depth, float offsetX, float offsetY) {
    return uv + vec2(offsetX, offsetY) * (depth - 1.0);
}

vec2 applyRotation(vec2 uv, vec2 center, float depth, float twist) {
    float angle = twist * log2(depth);
    vec2 d = uv - center;
    float c = cos(angle), s = sin(angle);
    return center + vec2(c * d.x - s * d.y, s * d.x + c * d.y);
}
```

## Parameters

### Layers

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `layers` | int | 2-16 | 6 | Number of depth layers |
| `depthCurve` | enum | - | Exponential | How depth maps to layer index |
| `depthScale` | float | 0.5-10.0 | 2.0 | Depth range multiplier |
| `cascade` | bool | - | false | Feed each layer's UV to the next |

### Transform

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `offsetX` | float | -1.0 to 1.0 | 0.0 | Parallax X per depth unit |
| `offsetY` | float | -1.0 to 1.0 | 0.0 | Parallax Y per depth unit |
| `rotationTwist` | float | -π to π | 0.0 | Rotation per log2(depth) |

### Blending

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `blendMode` | enum | - | WeightedAvg | Layer compositing mode |
| `falloffType` | enum | - | Inverse | Weight curve shape |
| `falloffPower` | float | 0.1-4.0 | 1.0 | Falloff curve intensity |
| `focusDepth` | float | 0.0-1.0 | 0.5 | Focus plane (Gaussian only) |

### Animation

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `cycleSpeed` | float | -2.0 to 2.0 | 0.0 | Layer depth cycling rate |
| `centerX` | float | 0.0-1.0 | 0.5 | Focal center X |
| `centerY` | float | 0.0-1.0 | 0.5 | Focal center Y |

### Bounds

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `boundsMode` | enum | - | Discard | Out-of-bounds handling |

## Enum Types

### Depth Curves

| Name | Formula | Character |
|------|---------|-----------|
| Exponential | `exp2(phase * scale)` | Dramatic near/far separation |
| Linear | `1 + phase * scale` | Even spacing |
| Logarithmic | `log2(1 + phase * scale)` | Compressed far layers |
| Inverse | `1 / (1 + phase * scale)` | Rapid falloff |

### Blend Modes

| Name | Formula |
|------|---------|
| Weighted Average | `Σ(c*w) / Σw` |
| Additive | `Σ(c*w)` clamped |
| Max | `max(a, b)` |
| Screen | `1 - (1-a)*(1-b)` |

### Falloff Types

| Name | Formula |
|------|---------|
| Linear | `1 - depth * power` |
| Inverse | `1 / (depth * power)` |
| Inverse Square | `1 / (depth² * power)` |
| Exponential | `exp(-depth * power)` |
| Gaussian | `exp(-(depth - focus)² / σ²)` |

### Bounds Modes

| Name | Behavior |
|------|----------|
| Discard | Skip layer if UV out of [0,1] |
| Clamp | Clamp UV to [0,1] |
| Mirror | Reflect at boundaries |
| Wrap | Tile with `fract(uv)` |

## Modulation Candidates

- **cycleSpeed** — Beat/bass for rhythmic zoom pulsing
- **depthScale** — Bass for depth breathing
- **rotationTwist** — Mid for spiral motion
- **offsetX/Y** — Spectral centroid for tonal drift
- **falloffPower** — Treble for depth-of-field shifts

## Preset Recipes

### Infinite Zoom
```
layers: 8, depthCurve: Exponential, depthScale: 4.0
cycleSpeed: 0.5, cascade: false
blendMode: WeightedAvg, falloffType: Inverse
```

### Parallax Drift
```
layers: 6, depthCurve: Linear, depthScale: 2.0
offsetX: 0.1, offsetY: 0.05
blendMode: Additive, falloffType: InverseSquare
```

### Spiral Tunnel
```
layers: 10, depthCurve: Exponential, depthScale: 3.0
cycleSpeed: 0.3, rotationTwist: 0.5
blendMode: Screen, falloffType: Exponential
```

### Ghost Trail
```
layers: 4, depthCurve: Linear, depthScale: 1.5
cascade: true
blendMode: Additive, falloffType: Linear
```

## Implementation Notes

1. **Angular params**: `rotationTwist` follows standard conventions:
   - Stored as radians, ±ROTATION_OFFSET_MAX
   - Display as degrees in UI via `ModulatableSliderAngleDeg`

2. **Center point**: Make `centerX/Y` draggable via `DrawInteractiveHandle`.

3. **Cascade mode**: When enabled, iteration order matters—early layers influence later ones exponentially.

4. **Performance**: 16 layers = 16 texture samples. Profile on target hardware.

## UI Organization

```
── Layers ─────────────────────
   [Layers] 6        [Curve ▾] Exponential
   ── Depth Scale ──────────○──
   [ ] Cascade

── Transform ──────────────────
   ── Offset X ────────○──────
   ── Offset Y ────────○──────
   ── Rotation Twist ──○──────

── Blending ───────────────────
   [Blend ▾] Weighted Avg
   [Falloff ▾] Inverse
   ── Falloff Power ───○──────

── Animation ──────────────────
   ── Cycle Speed ─────○──────
   [Center] (drag handle)

── Bounds ─────────────────────
   [Mode ▾] Discard
```
