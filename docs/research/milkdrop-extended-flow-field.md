# MilkDrop Extended Flow Field Parameters

Extends the existing feedback flow field with four MilkDrop features: procedural warp animation, movable center pivot, directional stretch, and angular variation. Creates the characteristic psychedelic undulating motion of classic MilkDrop presets.

## Classification

- **Category**: FEEDBACK (extends existing Flow Field)
- **Core Operation**: UV displacement with time-animated procedural distortion
- **Pipeline Position**: Feedback stage, applied after basic zoom/rot/dx/dy transforms

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html) - Official parameter documentation
- [projectM Warp Shader](https://github.com/projectM-visualizer/projectm/blob/master/src/libprojectM/MilkdropPreset/Shaders/PresetWarpVertexShaderGlsl330.vert) - Reference GLSL implementation
- [Butterchurn](https://github.com/jberg/butterchurn) - WebGL MilkDrop implementation

## Algorithm

### Transformation Order

MilkDrop applies UV transforms in this specific order (from projectM shader):

1. **Zoom** (exponential, radius-dependent)
2. **Stretch** (relative to center)
3. **Procedural warp** (4 animated sin/cos terms)
4. **Rotation** (around center)
5. **Translation** (dx/dy)
6. Aspect correction

### Feature 1: Procedural Warp

Creates animated undulating distortion via four superimposed sinusoidal waves.

**Shader code** (from projectM):
```glsl
// warpFactors are time-varying oscillations computed per-frame on CPU:
// warpFactors.x = 11.68 + 4.0 * cos(warpTime * 1.413 + 10)
// warpFactors.y = 8.77  + 3.0 * cos(warpTime * 1.113 + 7)
// warpFactors.z = 10.54 + 3.0 * cos(warpTime * 1.233 + 3)
// warpFactors.w = 11.49 + 4.0 * cos(warpTime * 0.933 + 5)

u += warp * 0.0035 * sin(warpTime * 0.333 + warpScaleInverse * (pos.x * warpFactors.x - pos.y * warpFactors.w));
v += warp * 0.0035 * cos(warpTime * 0.375 - warpScaleInverse * (pos.x * warpFactors.z + pos.y * warpFactors.y));
u += warp * 0.0035 * cos(warpTime * 0.753 - warpScaleInverse * (pos.x * warpFactors.y - pos.y * warpFactors.z));
v += warp * 0.0035 * sin(warpTime * 0.825 + warpScaleInverse * (pos.x * warpFactors.x + pos.y * warpFactors.w));
```

**Key characteristics**:
- Four different time frequencies (0.333, 0.375, 0.753, 0.825) create non-repeating patterns
- `warpFactors` oscillate slowly with incommensurate frequencies (1.413, 1.113, 1.233, 0.933)
- Spatial modulation via `pos.x * factor - pos.y * factor` creates diagonal wavefronts
- Base amplitude 0.0035 per term (total ~0.014 at max constructive interference)

**Parameters**:
- `warp` (0-2+): Amplitude multiplier. 0=disabled, 1=normal, 2=major warping
- `warpTime`: Accumulated time (seconds) driving oscillation
- `warpScale` (0.1-100): Spatial frequency. Lower = larger waves. Passed as `1.0/warpScale`

### Feature 2: Movable Center (cx, cy)

The center point affects where zoom, rotation, and stretch pivot around.

**Current AudioJones** (hardcoded):
```glsl
vec2 center = vec2(0.5);
```

**MilkDrop behavior**:
- `cx` (0-1): 0=left edge, 0.5=center, 1=right edge
- `cy` (0-1): 0=top edge, 0.5=center, 1=bottom edge

**Transform application** (stretch example):
```glsl
u = (u - cx) / sx + cx;
v = (v - cy) / sy + cy;
```

**Rotation application**:
```glsl
float u2 = u - cx;
float v2 = v - cy;
u = u2 * cos(rot) - v2 * sin(rot) + cx;
v = u2 * sin(rot) + v2 * cos(rot) + cy;
```

### Feature 3: Directional Stretch (sx, sy)

Independent scaling on X and Y axes, creating anamorphic/squeeze effects.

**Parameters**:
- `sx` (>0): Horizontal stretch. 0.99=shrink 1%, 1.0=normal, 1.01=stretch 1%
- `sy` (>0): Vertical stretch. Same range as sx

**Differs from zoom**: Zoom is uniform scaling; stretch allows different X/Y ratios.

**Shader code**:
```glsl
u = (u - cx) / sx + cx;
v = (v - cy) / sy + cy;
```

### Feature 4: Angular Variation

MilkDrop's per-vertex equations can modulate any parameter based on polar angle from center.

**Currently AudioJones supports**:
```glsl
float zoom = zoomBase + rad * zoomRadial;  // radial only
```

**MilkDrop also allows**:
```glsl
zoom = zoomBase + rad * zoomRadial + sin(ang * N) * zoomAngular;
rot = rotBase + rad * rotRadial + sin(ang * M) * rotAngular;
```

**Common patterns**:
- `sin(ang * 2)`: 2-fold symmetry (opposite spirals)
- `sin(ang * 4)`: 4-fold symmetry (cross pattern)
- `cos(ang)`: Left-right asymmetry
- `sin(ang + time)`: Rotating pattern

**Polar coordinate calculation**:
```glsl
vec2 fromCenter = uv - center;
float rad = length(fromCenter);  // 0 at center, ~0.7 at corners
float ang = atan(fromCenter.y, fromCenter.x);  // -PI to PI
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `warp` | float | 0-2+ | 0 | Procedural warp amplitude |
| `warpSpeed` | float | 0.1-2.0 | 1.0 | Time multiplier for warp animation |
| `warpScale` | float | 0.1-100 | 1.0 | Spatial frequency (lower = larger waves) |
| `cx` | float | 0-1 | 0.5 | Horizontal pivot point |
| `cy` | float | 0-1 | 0.5 | Vertical pivot point |
| `sx` | float | 0.9-1.1 | 1.0 | Horizontal stretch |
| `sy` | float | 0.9-1.1 | 1.0 | Vertical stretch |
| `zoomAngular` | float | -0.1-0.1 | 0 | Angular modulation amplitude for zoom |
| `zoomAngularFreq` | int | 1-8 | 2 | Angular frequency (N in sin(ang*N)) |
| `rotAngular` | float | -0.05-0.05 | 0 | Angular modulation amplitude for rotation |
| `rotAngularFreq` | int | 1-8 | 2 | Angular frequency for rotation |
| `dxAngular` | float | -0.02-0.02 | 0 | Angular modulation for horizontal translation |
| `dyAngular` | float | -0.02-0.02 | 0 | Angular modulation for vertical translation |

## Implementation Considerations

### Transformation Order

Match MilkDrop's order for preset compatibility:
1. Zoom with radial/angular modulation
2. Stretch relative to center
3. Procedural warp
4. Rotation around center
5. Translation

Current AudioJones order: zoom -> rotate -> translate. Insert stretch after zoom, warp before rotation.

### warpFactors Computation

Option A: Compute on CPU, pass as uniform (matches projectM):
```cpp
float warpTime = accumulatedTime * warpSpeed;
Vector4 warpFactors = {
    11.68f + 4.0f * cosf(warpTime * 1.413f + 10.0f),
    8.77f  + 3.0f * cosf(warpTime * 1.113f + 7.0f),
    10.54f + 3.0f * cosf(warpTime * 1.233f + 3.0f),
    11.49f + 4.0f * cosf(warpTime * 0.933f + 5.0f)
};
```

Option B: Compute in shader (simpler, no CPU state):
```glsl
vec4 warpFactors = vec4(
    11.68 + 4.0 * cos(warpTime * 1.413 + 10.0),
    8.77  + 3.0 * cos(warpTime * 1.113 + 7.0),
    10.54 + 3.0 * cos(warpTime * 1.233 + 3.0),
    11.49 + 4.0 * cos(warpTime * 0.933 + 5.0)
);
```

Option B recommended for AudioJones to avoid adding CPU state for this.

### Parameter Sensitivity

All feedback parameters compound exponentially:
- `warp = 1.0` creates ~1.4% max UV offset per frame
- At 60fps, patterns evolve visibly but don't overwhelm

Suggested safe ranges:
- `warp`: 0-2 for typical use, up to 5 for extreme
- `sx/sy`: 0.95-1.05 for subtle, 0.9-1.1 max
- `angular` coefficients: ~10% of radial coefficients

### Aspect Ratio Handling

MilkDrop normalizes coordinates before angular calculation:
```glsl
vec2 normalized = fromCenter;
if (aspect > 1.0) normalized.x /= aspect;
else normalized.y *= aspect;
float ang = atan(normalized.y, normalized.x);
```

This ensures `ang` represents visual angle, not stretched coordinate angle.

## Audio Mapping Ideas

- `warp` modulated by `bass_att` for throbbing distortion
- `cx/cy` slowly drifting with `sin(time * 0.1)` creates wandering focus
- `sx/sy` inverse correlation creates breathing effect: `sx = 1 + bass * 0.02`, `sy = 1 - bass * 0.02`
- Angular frequency (`zoomAngularFreq`) randomized on beat creates pattern variation

## Notes

- The procedural warp's magic numbers (11.68, 8.77, etc.) are inherited from original MilkDrop. They create visually pleasing non-repeating patterns through incommensurate frequencies.
- MilkDrop's `warpScale` is passed as inverse (1.0/warpScale) to the shader. AudioJones can either follow this convention or expose it directly.
- Consider whether to add `warpTime` as accumulated CPU state (matches MilkDrop) or compute from global time (simpler but changes if preset restarts).
