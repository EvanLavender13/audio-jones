# Laser Dance Camera

Add camera drift, rotation, and zoom to Laser Dance to break the periodic 6.3-second flash cycle. The camera wanders through the cosine field on a Lissajous path while slowly rotating, so the viewer never sees the same view twice. No new files — extends the existing generator with new config fields, uniforms, and shader lines.

**Research**: `docs/research/laser_dance_camera.md`

## Design

### Types

**LaserDanceConfig** — add after the `// Warp` section:

```
// Camera
float zoom = 1.0f;            // Focal length / FOV (0.5-3.0)
float rotationSpeed = 0.0f;   // Camera rotation rate rad/s (-PI_F to PI_F)
DualLissajousConfig drift = {
    .amplitude = 16.0f,
    .motionSpeed = 1.0f,
    .freqX1 = 0.07f,
    .freqY1 = 0.05f,
    .freqX2 = 0.0f,
    .freqY2 = 0.0f,
};
```

Requires `#include "config/dual_lissajous_config.h"` in the header.

**LaserDanceEffect** — add three loc fields and one accumulator:

```
int cameraDriftLoc;
int cameraAngleLoc;
int zoomLoc;
float cameraAngle;  // accumulated rotation
```

**LASER_DANCE_CONFIG_FIELDS** — append: `zoom, rotationSpeed, drift`

### Algorithm

Three changes to the shader's raymarch loop, applied in order: drift → rotation → warp.

**Zoom** — replace hardcoded focal length:

```glsl
// Current:
vec3 rayDir = normalize(vec3(uv, -1.0));
// New:
vec3 rayDir = normalize(vec3(uv, -zoom));
```

**Drift + Rotation** — insert after `vec3 p = z * rayDir;`, before the existing warp line:

```glsl
vec3 p = z * rayDir;
p.xy += cameraDrift;                                // camera position offset
float ca = cos(cameraAngle), sa = sin(cameraAngle); // camera view rotation
p.xy = mat2(ca, -sa, sa, ca) * p.xy;
p += cos(p.yzx * warpFreq + warpTime * vec3(1.0, 1.3, 0.7)) * warpAmount; // existing warp (unchanged)
```

Three new uniforms in the shader:

```glsl
uniform vec2 cameraDrift;
uniform float cameraAngle;
uniform float zoom;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| zoom | float | 0.5-3.0 | 1.0 | Yes | `"Zoom##laserDance"` |
| rotationSpeed | float | -PI_F to PI_F | 0.0 | Yes | `ModulatableSliderSpeedDeg` `"Rotation##laserDanceCamera"` |
| drift.amplitude | float | 0.0-50.0 | 16.0 | Yes | via DrawLissajousControls |
| drift.motionSpeed | float | 0.0-5.0 | 1.0 | Yes | via DrawLissajousControls |
| drift.freqX1 | float | 0.0-0.2 | 0.07 | No | via DrawLissajousControls |
| drift.freqY1 | float | 0.0-0.2 | 0.05 | No | via DrawLissajousControls |
| drift.freqX2 | float | 0.0-0.2 | 0.0 | No | via DrawLissajousControls |
| drift.freqY2 | float | 0.0-0.2 | 0.0 | No | via DrawLissajousControls |

---

## Tasks

### Wave 1: Header

#### Task 1.1: Config and effect struct updates

**Files**: `src/effects/laser_dance.h`
**Creates**: New config fields and loc fields that .cpp and .fs depend on

**Do**:
- Add `#include "config/dual_lissajous_config.h"`
- Add `// Camera` section to `LaserDanceConfig` after `// Warp` section with `zoom`, `rotationSpeed`, and `drift` fields as specified in Design > Types
- Add `cameraDriftLoc`, `cameraAngleLoc`, `zoomLoc`, and `cameraAngle` to `LaserDanceEffect`
- Append `zoom, rotationSpeed, drift` to `LASER_DANCE_CONFIG_FIELDS`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Setup, registration, and UI

**Files**: `src/effects/laser_dance.cpp`
**Depends on**: Wave 1 complete

**Do**:

*Init* — cache three new uniform locations (`cameraDrift`, `cameraAngle`, `zoom`). Initialize `e->cameraAngle = 0.0f`.

*Setup* — add before existing uniform binding:
1. Accumulate `e->cameraAngle += cfg->rotationSpeed * deltaTime`
2. Compute drift via `DualLissajousUpdate` on `&cfg->drift`:
   - Guard with `if (cfg->drift.amplitude > 0.0f)` like `infinite_zoom.cpp:60-65`
   - Pass `deltaTime`, `0.0f` perSourceOffset
   - Store result in `float cameraDrift[2]`
   - Default `cameraDrift` to `{0.0f, 0.0f}` before the guard
3. Bind three new uniforms: `cameraDrift` (VEC2), `cameraAngle` (FLOAT), `zoom` (FLOAT — bind `&cfg->zoom`)

*RegisterParams* — add:
- `"laserDance.zoom"` — range 0.5-3.0
- `"laserDance.rotationSpeed"` — range `-ROTATION_SPEED_MAX` to `ROTATION_SPEED_MAX`
- `"laserDance.drift.amplitude"` — range 0.0-50.0
- `"laserDance.drift.motionSpeed"` — range 0.0-5.0

*UI* — in `DrawLaserDanceParams`, add a `// Camera` section after the Warp section (before Audio):
- `ImGui::SeparatorText("Camera")`
- `ModulatableSlider` for zoom: `"Zoom##laserDance"`, range implicit from registered param, format `"%.2f"`
- `ModulatableSliderSpeedDeg` for rotationSpeed: `"Rotation##laserDanceCamera"`, range `-ROTATION_SPEED_MAX` to `ROTATION_SPEED_MAX`
- `DrawLissajousControls(&c->drift, "ld_drift", "laserDance.drift", modSources, 0.2f)` — freqMax 0.2 since these are slow sub-Hz drift frequencies

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Shader modifications

**Files**: `shaders/laser_dance.fs`
**Depends on**: Wave 1 complete

**Do**:

*Uniforms* — add three new uniforms after `uniform float warpFreq;`:
```glsl
uniform vec2 cameraDrift;
uniform float cameraAngle;
uniform float zoom;
```

*Ray direction* — change `vec3 rayDir = normalize(vec3(uv, -1.0));` to `vec3 rayDir = normalize(vec3(uv, -zoom));`

*Raymarch loop* — insert drift and rotation between `vec3 p = z * rayDir;` and the existing warp line. Follow the exact order from Design > Algorithm: drift → rotation → warp.

*Attribution* — add to the shader header comment block:
```
// Camera drift and rotation from "Star Field Flight [351]" by diatribes (https://www.shadertoy.com/view/3ft3DS)
```

**Verify**: `cmake.exe --build build` compiles. Visual: drift active by default (amplitude 16.0), rotation off by default.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Default preset: drift visibly moves camera through field (amplitude 16.0 active by default)
- [ ] Default preset: no periodic full-screen white flash (drift breaks the cycle)
- [ ] Rotation slider at nonzero: field slowly rotates
- [ ] Zoom slider: changing from 1.0 narrows/widens FOV
- [ ] Warp still functions correctly (drift applied before warp)
- [ ] Preset save/load round-trips all new fields (zoom, rotationSpeed, drift)
