# Disco Ball (Phase 1: Sphere Reflection)

Faceted mirror sphere that reflects the input texture from each tile at a slightly different angle. The ball rotates, creating the classic disco effect with beveled tile edges.

## Current State

Cellular effects follow established patterns:
- `src/config/voronoi_config.h` - Similar config struct pattern
- `src/ui/imgui_effects_cellular.cpp:17-103` - UI panel pattern for cellular effects
- `shaders/voronoi.fs` - Reference for texture sampling with cell-based UV transforms

## Technical Implementation

**Source**: `docs/research/disco-ball.md`

### Ray-Sphere Intersection

Analytic intersection for centered sphere:

```glsl
// ro = ray origin (0,0,-2 for orthographic front view)
// rd = ray direction (0,0,1 for orthographic)
// For screen-space: ro = vec3(uv - 0.5, -2.0), rd = vec3(0,0,1)
float RaySphere(vec3 ro, vec3 rd, float radius) {
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return -1.0;  // miss
    return -b - sqrt(h);       // near intersection
}
```

### Spherical UV Tiling

Convert hit position to phi/theta, quantize into grid:

```glsl
vec3 pos = ro + rd * t;  // hit position on sphere
vec2 phiTheta = vec2(
    atan(pos.z, pos.x) + sphereAngle,  // longitude + rotation
    acos(clamp(pos.y / radius, -1.0, 1.0))  // latitude
);
vec2 tileId = floor(phiTheta / tileSize);
vec2 tilePos = fract(phiTheta / tileSize);  // [0,1] within tile
```

### Facet Normal Perturbation

Each tile has a flat face. Compute tile center, reconstruct normal:

```glsl
vec2 tileCenter = (tileId + 0.5) * tileSize;

vec3 facetPos;
facetPos.x = sin(tileCenter.y) * cos(tileCenter.x);
facetPos.y = cos(tileCenter.y);
facetPos.z = sin(tileCenter.y) * sin(tileCenter.x);

// Edge bump: darker at tile boundaries
vec2 edge = min(tilePos * 10.0, (1.0 - tilePos) * 10.0);
float bump = clamp(min(edge.x, edge.y), 0.0, 1.0);

// Perturb normal at edges for bevel effect
facetPos.y += (1.0 - bump) * bumpHeight;
vec3 normal = normalize(facetPos);
```

### Texture Reflection Sampling

```glsl
vec3 refl = reflect(rd, normal);

// Map 3D reflection to 2D texture coords
vec2 reflUV = refl.xy * 0.5 + 0.5;
vec3 color = texture(inputTex, reflUV).rgb;

// Darken edges, boost center
color *= bump * intensity;
```

## Parameters

| Parameter | Range | Default | Purpose |
|-----------|-------|---------|---------|
| sphereRadius | 0.2 - 1.5 | 0.8 | Size of ball (fraction of screen height) |
| tileSize | 0.05 - 0.3 | 0.12 | Facet grid density (smaller = more tiles) |
| rotationSpeed | -2.0 - 2.0 | 0.5 | Spin rate (radians/sec) |
| bumpHeight | 0.0 - 0.2 | 0.1 | Edge bevel depth |
| reflectIntensity | 0.5 - 5.0 | 2.0 | Brightness of reflected texture |

---

## Phase 1: Config Header

**Goal**: Define configuration struct for disco ball parameters.
**Files**: `src/config/disco_ball_config.h`

**Build**:
- Create `src/config/disco_ball_config.h` with `DiscoBallConfig` struct
- Fields: `enabled`, `sphereRadius`, `tileSize`, `rotationSpeed`, `bumpHeight`, `reflectIntensity`
- Use in-class defaults matching Parameters table

**Verify**: `cmake.exe --build build` → compiles without errors.

**Done when**: Config header exists with all parameter fields.

---

## Phase 2: Effect Registration

**Goal**: Register disco ball in the transform effect system.
**Files**: `src/config/effect_config.h`

**Build**:
- Add `#include "disco_ball_config.h"` with other config includes
- Add `TRANSFORM_DISCO_BALL` to `TransformEffectType` enum (before `TRANSFORM_EFFECT_COUNT`)
- Add `"Disco Ball"` to `TRANSFORM_EFFECT_NAMES` array at matching index
- Add `DiscoBallConfig discoBall;` member to `EffectConfig` struct
- Add case `TRANSFORM_DISCO_BALL: return e->discoBall.enabled;` to `IsTransformEnabled()`

**Verify**: `cmake.exe --build build` → compiles without errors.

**Done when**: `TRANSFORM_DISCO_BALL` exists and `IsTransformEnabled` handles it.

---

## Phase 3: Shader

**Goal**: Implement disco ball fragment shader.
**Files**: `shaders/disco_ball.fs`

**Build**:
- Create `shaders/disco_ball.fs` with `#version 330`
- Uniforms: `texture0`, `resolution`, `sphereRadius`, `tileSize`, `sphereAngle`, `bumpHeight`, `reflectIntensity`
- Implement ray-sphere intersection from Technical Implementation
- Implement spherical UV tiling with `sphereAngle` rotation
- Implement facet normal calculation with edge bump
- Sample input texture using reflection UV
- Pass through original texture for pixels outside sphere

**Verify**: Shader file exists, syntax valid (will test at runtime).

**Done when**: Shader implements full algorithm from research doc.

---

## Phase 4: PostEffect Integration

**Goal**: Load shader and cache uniform locations.
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
- Add `Shader discoBallShader;` to `PostEffect` struct in header
- Add uniform location ints: `discoBallResolutionLoc`, `discoBallSphereRadiusLoc`, `discoBallTileSizeLoc`, `discoBallSphereAngleLoc`, `discoBallBumpHeightLoc`, `discoBallReflectIntensityLoc`
- Add `float discoBallAngle;` for accumulated rotation
- In `post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to shader success check
  - Get uniform locations in `GetShaderUniformLocations()`
  - Set resolution uniform in `SetResolutionUniforms()`
  - Unload shader in `PostEffectUninit()`

**Verify**: `cmake.exe --build build` → compiles, shader loads at runtime.

**Done when**: Shader loads successfully and uniform locations are cached.

---

<!-- CHECKPOINT: Compiles, shader loads but no rendering -->

## Phase 5: Shader Setup

**Goal**: Bind uniforms each frame and handle rotation accumulation.
**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`

**Build**:
- Declare `void SetupDiscoBall(PostEffect* pe);` in header
- Add dispatch case in `GetTransformEffect()` for `TRANSFORM_DISCO_BALL`
- Implement `SetupDiscoBall()`:
  - Accumulate `pe->discoBallAngle += cfg->rotationSpeed * pe->currentDeltaTime`
  - Set all uniforms from config

**Verify**: `cmake.exe --build build` → compiles without errors.

**Done when**: Setup function binds all uniforms with rotation accumulation.

---

## Phase 6: UI Panel

**Goal**: Add disco ball controls to Cellular effects category.
**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_cellular.cpp`

**Build**:
- Add `case TRANSFORM_DISCO_BALL: return 2;` to `GetTransformCategory()` in `imgui_effects.cpp` (category 2 = Cellular)
- In `imgui_effects_cellular.cpp`:
  - Add `#include "config/disco_ball_config.h"`
  - Add `static bool sectionDiscoBall = false;`
  - Add `DrawCellularDiscoBall()` helper with:
    - Enable checkbox with `MoveTransformToEnd` on enable
    - `ModulatableSlider` for sphereRadius, tileSize, bumpHeight, reflectIntensity
    - `ModulatableSliderAngleDeg` for rotationSpeed (shows °/s)
  - Call from `DrawCellularCategory()` with spacing

**Verify**: Build, run, disco ball section appears in Cellular category.

**Done when**: UI controls modify effect in real-time.

---

## Phase 7: Preset Serialization

**Goal**: Save/load disco ball settings in presets.
**Files**: `src/config/preset.cpp`

**Build**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DiscoBallConfig, enabled, sphereRadius, tileSize, rotationSpeed, bumpHeight, reflectIntensity)` with other macros
- Add `if (e.discoBall.enabled) { j["discoBall"] = e.discoBall; }` to `to_json`
- Add `e.discoBall = j.value("discoBall", e.discoBall);` to `from_json`

**Verify**: Save preset with disco ball enabled, reload, settings preserved.

**Done when**: Presets correctly serialize all disco ball parameters.

---

## Phase 8: Parameter Registration

**Goal**: Enable LFO/audio modulation of disco ball parameters.
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add to `PARAM_TABLE`:
  ```cpp
  {"discoBall.sphereRadius",     {0.2f, 1.5f}},
  {"discoBall.tileSize",         {0.05f, 0.3f}},
  {"discoBall.rotationSpeed",    {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  {"discoBall.bumpHeight",       {0.0f, 0.2f}},
  {"discoBall.reflectIntensity", {0.5f, 5.0f}},
  ```
- Add matching pointers to `targets[]` array:
  ```cpp
  &effects->discoBall.sphereRadius,
  &effects->discoBall.tileSize,
  &effects->discoBall.rotationSpeed,
  &effects->discoBall.bumpHeight,
  &effects->discoBall.reflectIntensity,
  ```

**Verify**: Create LFO route to `discoBall.rotationSpeed`, ball spin responds to LFO.

**Done when**: All parameters respond to modulation sources.

---

<!-- CHECKPOINT: Feature complete, ready for testing -->

## Verification Checklist

- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Cellular" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to the pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to registered parameters
- [ ] Build succeeds with no warnings
- [ ] Ball rotates smoothly with `rotationSpeed`
- [ ] Facet edges visible with `bumpHeight > 0`
- [ ] Reflection shows input texture, not black

---

## Post-Implementation Notes

### Changed: Perspective ray casting (2026-01-28)

**Reason**: Orthographic rays (`rd = vec3(0,0,1)`) produced flat-colored facets because reflection only depended on facet normal. Perspective rays vary across screen, giving each facet unique reflections.

**Changes**:
- `shaders/disco_ball.fs`: Changed from `rd = vec3(0,0,1)` to `rd = normalize(vec3(p, 2.0))`
- Ray origin moved to `vec3(0, 0, -2.5)` to match reference shader

### Changed: Black background (2026-01-28)

**Reason**: User requested ball-only rendering without background texture pass-through.

**Changes**:
- `shaders/disco_ball.fs`: Miss case outputs `vec4(0.0)` instead of sampling `texture0`

### Added: Contrast and gamma (2026-01-28)

**Reason**: Reference shader uses color squaring and gamma correction for punchier reflections.

**Changes**:
- `shaders/disco_ball.fs`: Added `color = color * color` and `pow(color, vec3(0.6))`
