# Laser Dance Camera

Add camera drift, rotation, and zoom to break the periodic 6.3-second cycle and eliminate the full-screen bright flash. The camera wanders through the cosine field on a Lissajous path while slowly rotating, so the viewer never sees the same view twice.

## Classification

- **Category**: GENERATORS > Filament (enhancement to existing Laser Dance)
- **Pipeline Position**: Same as Laser Dance — generator stage
- **Scope**: New config fields, uniforms, shader modifications. No new files.

## Attribution

- **Based on**: "Star Field Flight [351]" by diatribes (camera drift and rotation technique)
- **Source**: https://www.shadertoy.com/view/3ft3DS
- **License**: CC BY-NC-SA 3.0
- **Lineage**: Same source as the existing position warp enhancement. Drift and rotation lines were not ported during the warp pass.

## References

- [Star Field Flight [351]](https://www.shadertoy.com/view/3ft3DS) - Camera drift and rotation from diatribes' laser dance variant

## Reference Code

The full diatribes shader (camera drift and rotation lines marked):

```glsl
void mainImage(out vec4 o, vec2 u) {
    float i,d,s,t = iTime*3.;
    vec3  p = iResolution;
    u = ((u-p.xy/2.)/p.y);
    for(o*=i; i++<80.;o += (1.+cos(d+vec4(4,2,1,0))) / s)
        p = vec3(u * d, d + t),
        p.xy *= mat2(cos(tanh(sin(t*.1)*4.)*3.+vec4(0,33,11,0))),  // <-- rotation
        p.xy -= vec2(sin(t*.07)*16., sin(t*.05)*16.),               // <-- drift
        p += cos(t+p.y+p.x+p.yzx*.4)*.3,                           // <-- warp (already implemented)
        d += s = length(min( p = cos(p) + cos(p).yzx, p.zxy))*.3;
    o = tanh(o / d / 2e2);
}
```

## Algorithm

### Zoom

Change the ray direction from hardcoded `-1.0` focal length to a `zoom` uniform:

```glsl
// Current:
vec3 rayDir = normalize(vec3(uv, -1.0));
// New:
vec3 rayDir = normalize(vec3(uv, -zoom));
```

Larger zoom = narrower FOV (more zoomed in). Smaller = wider view.

### Drift

Compute camera position on CPU using the existing `DualLissajousConfig` infrastructure. Pass result as `vec2 cameraDrift` uniform. Apply in the raymarch loop:

```glsl
// After sample point, before warp:
vec3 p = z * rayDir;
p.xy += cameraDrift;  // <-- NEW: camera position offset
p += cos(p.yzx * warpFreq + warpTime * vec3(1.0, 1.3, 0.7)) * warpAmount;  // existing warp
```

The reference uses `sin(t*.07)*16.` and `sin(t*.05)*16.` — single-frequency sinusoids with irrational-ratio frequencies so the path never exactly repeats. `DualLissajousConfig` with dual frequencies per axis produces richer quasi-periodic motion, strictly better for our goal. CPU computes position via `DualLissajousUpdate()`, shader just receives the offset.

### Rotation

Accumulate rotation angle on CPU (`cameraAngle += rotationSpeed * deltaTime`). Pass as `float cameraAngle` uniform. Apply as 2D rotation in the shader:

```glsl
// After drift, before warp:
float ca = cos(cameraAngle), sa = sin(cameraAngle);
p.xy = mat2(ca, -sa, sa, ca) * p.xy;
```

The reference uses `tanh(sin(t*.1)*4.)` for a snapping oscillation between orientations. A constant rotation speed is simpler and more predictable for modulation. Users who want oscillation can route an LFO to `rotationSpeed`.

### Application order in raymarch loop

```glsl
vec3 p = z * rayDir;
p.xy += cameraDrift;                                                         // 1. drift
float ca = cos(cameraAngle), sa = sin(cameraAngle);                         // 2. rotation
p.xy = mat2(ca, -sa, sa, ca) * p.xy;
p += cos(p.yzx * warpFreq + warpTime * vec3(1.0, 1.3, 0.7)) * warpAmount;  // 3. warp (existing)
vec3 q = cos(p + time) + cos(p / freqRatio).yzx;                            // 4. distance field (existing)
```

### Flash elimination

The drift naturally breaks the flash. Currently all rays hit the cosine zero-crossing simultaneously because the camera is stationary and the field is periodic (period 2π ≈ 6.28s at speed=1.0). With drift, the camera is at a different position each cycle, so rays sample different field regions. The periodic alignment that caused the full-screen white flash no longer occurs.

### What to keep from reference

- `p.xy += offset` applied inside the loop (camera position offset)
- `mat2` rotation on `p.xy` (view rotation)
- Irrational frequency ratios for non-repeating drift

### What to replace

| Reference | Replacement |
|-----------|-------------|
| `sin(t*.07)*16., sin(t*.05)*16.` | `DualLissajousConfig` computed on CPU, passed as `vec2 cameraDrift` |
| `mat2(cos(tanh(sin(t*.1)*4.)*3.+vec4(0,33,11,0)))` | Simple `mat2(ca, -sa, sa, ca)` from CPU-accumulated `cameraAngle` |
| Hardcoded `-1.0` focal length | `zoom` uniform |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| zoom | float | 0.5-3.0 | 1.0 | Focal length / FOV. Higher = more zoomed in |
| rotationSpeed | float | -PI_F to PI_F | 0.0 | Camera view rotation rate in rad/s |
| drift (DualLissajousConfig) | struct | — | see below | Lissajous camera drift path |

DualLissajousConfig overridden defaults for camera drift:

| Field | Default | Rationale |
|-------|---------|-----------|
| amplitude | 16.0 | Matches reference `sin()*16.` — drifts through ~5 periods of the cosine field |
| motionSpeed | 1.0 | Standard rate |
| freqX1 | 0.07 | Matches reference X frequency |
| freqY1 | 0.05 | Matches reference Y frequency |
| freqX2 | 0.0 | Disabled by default (single-frequency like reference) |
| freqY2 | 0.0 | Disabled by default |

## Modulation Candidates

- **zoom**: Pulsing zoom creates a breathing effect — zooming in on bass hits, widening on release
- **rotationSpeed**: Speed changes create acceleration/deceleration feel
- **drift.amplitude**: How far the camera wanders — low for intimate, high for epic sweeps
- **drift.motionSpeed**: Faster = frantic wandering, slower = gentle exploration

### Interaction Patterns

- **drift.amplitude × zoom** (competing forces): Wide drift explores more of the field while high zoom narrows the view. Together they create a "peephole on a moving train" — tight detail that constantly changes. Modulating zoom with bass while drift amplitude follows energy creates verse/chorus dynamics.
- **rotationSpeed × drift** (resonance): When rotation and drift align, the visual sweeps dramatically in one direction. When they oppose, the view feels turbulent and disorienting. The interplay creates natural visual rhythm without explicit timing.

## Notes

- Default `rotationSpeed = 0.0` and `drift.amplitude = 16.0` — drift is on by default (that's the whole point), rotation is opt-in.
- `DualLissajousUpdate()` returns values in `[-amplitude, +amplitude]`, so amplitude 16.0 gives ±16 offset, matching the reference's `sin()*16.` range.
- Drift is applied before warp so the warp perturbation acts on the drifted position — the organic filaments shift as the camera moves.
- The `zoom` uniform replaces only the `-1.0` in the ray direction. All other shader math is unchanged.
- Three new uniforms: `cameraDrift` (vec2), `cameraAngle` (float), `zoom` (float). Three new loc fields + one DualLissajousConfig embed + one float accumulator.
