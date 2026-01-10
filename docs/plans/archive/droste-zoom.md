# Droste Zoom

Continuous conformal mapping that creates seamless recursive self-similarity via complex logarithm space. Single texture read produces the "infinite corridor" or "Escher spiral" effect where the image contains scaled/rotated copies of itself spiraling infinitely.

## Current State

Existing effect infrastructure to hook into:
- `src/config/effect_config.h:22-38` - TransformEffectType enum and TransformOrderConfig
- `src/render/post_effect.h:13-194` - PostEffect struct with shader handles and uniform locations
- `src/render/post_effect.cpp:25-67` - LoadPostEffectShaders() and GetShaderUniformLocations()
- `src/render/shader_setup.cpp:10-44` - GetTransformEffect() switch dispatch
- `src/render/render_pipeline.cpp:141-145` - Time accumulation for animated effects
- `src/ui/imgui_effects_transforms.cpp:350-379` - Motion category with Infinite Zoom, Radial Blur
- `src/automation/param_registry.cpp:55-57` - Infinite zoom parameter bounds (pattern to follow)

Research documentation:
- `docs/research/droste-zoom.md` - Complete algorithm, GLSL code, parameter ranges

## Technical Implementation

### Source
- Primary: FabriceNeyret2's Shadertoy (docs/research/droste-zoom.md:59-72)
- Reference: Jos Leys math derivation, Reinder Nijhoff WebGL

### Core Algorithm

The droste transform maps UV through complex log space:

```glsl
vec2 droste(vec2 uv, float scale, float time, float spiralAngle, float twist, int branches) {
    float s = log(scale);           // log of scale ratio
    float a = s / TWO_PI;           // spiral shear: tan(alpha) = s/2pi

    // Center UV and get polar coords
    uv -= 0.5;
    float r = length(uv);
    float theta = atan(uv.y, uv.x);

    // Multi-arm spiral: multiply angle by branch count
    if (branches > 1) {
        theta *= float(branches);
    }

    // Conformal spiral mapping: rotate log-polar space
    vec2 z = mat2(1.0, -a, a, 1.0) * vec2(log(r), theta);

    // Additional spiral rotation
    z.y += spiralAngle;

    // Radius-dependent twist
    z.y += twist * z.x;

    // Droste: mod log-radius, animate
    z.x = mod(z.x - time, s);

    // Convert back to Cartesian
    vec2 result = exp(z.x) * vec2(cos(z.y), sin(z.y));

    // Undo branch multiplication in output
    if (branches > 1) {
        float outTheta = atan(result.y, result.x) / float(branches);
        float outR = length(result);
        result = outR * vec2(cos(outTheta), sin(outTheta));
    }

    return result + 0.5;
}
```

### UV Transformations

1. **Log-polar mapping**: `(r, theta) -> (log(r), theta)`
2. **Spiral shear**: `mat2(1, -a, a, 1)` rotates log-polar rectangle so diagonal aligns with vertical
3. **Modulo wrap**: `mod(z.x - time, s)` creates infinite recursion + animation
4. **Exponential back**: `exp(z.x) * (cos(z.y), sin(z.y))` returns to Cartesian

### Parameters

| Parameter | Uniform | Range | Effect |
|-----------|---------|-------|--------|
| `scale` | `scale` | [1.5, 10.0] | Ratio between recursive copies |
| `speed` | N/A (CPU time) | [-2.0, 2.0] | Animation speed, negative = zoom in |
| `spiralAngle` | `spiralAngle` | [-pi, pi] | Additional rotation per cycle |
| `twist` | `twist` | [-1.0, 1.0] | Radius-dependent twist beyond natural alpha |
| `innerRadius` | `innerRadius` | [0.0, 0.5] | Mask center singularity |
| `branches` | `branches` | [1, 8] | Number of spiral arms |

### Singularity Handling

```glsl
// Clamp minimum radius to avoid log(0)
r = max(r, 0.001);

// Optional fade to black at center
if (innerRadius > 0.0) {
    float fade = smoothstep(0.0, innerRadius, r);
    finalColor.rgb *= fade;
}
```

---

## Phase 1: Config and Shader

**Goal**: Create config struct and implement shader with all parameters.

**Build**:
- Create `src/config/droste_zoom_config.h` with DrosteZoomConfig struct
  - Fields: enabled, speed, scale, spiralAngle, twist, innerRadius, branches
  - Defaults: false, 1.0, 2.5, 0.0, 0.0, 0.0, 1
- Create `shaders/droste_zoom.fs` implementing the algorithm above
  - Uniforms: time, scale, spiralAngle, twist, innerRadius, branches
  - Handle singularity with clamp + optional innerRadius fade

**Done when**: Shader compiles, config struct defined with defaults.

---

## Phase 2: Integration

**Goal**: Wire shader into render pipeline and make it executable.

**Build**:
- `src/config/effect_config.h`:
  - Add `#include "droste_zoom_config.h"`
  - Add `TRANSFORM_DROSTE_ZOOM` to enum (before TRANSFORM_EFFECT_COUNT)
  - Add `TransformEffectName()` case returning "Droste Zoom"
  - Add `TransformOrderConfig` default array entry
  - Add `DrosteZoomConfig drosteZoom;` member to EffectConfig
- `src/render/post_effect.h`:
  - Add `Shader drosteZoomShader;`
  - Add uniform location ints: `drosteZoomTimeLoc`, `drosteZoomScaleLoc`, etc.
  - Add `float drosteZoomTime;` accumulator
- `src/render/post_effect.cpp`:
  - LoadPostEffectShaders(): Load `shaders/droste_zoom.fs`
  - GetShaderUniformLocations(): Get all uniform locations
  - PostEffectInit(): Initialize `drosteZoomTime = 0.0f`
  - PostEffectUninit(): Unload shader
  - (Optional) SetResolutionUniforms() if shader needs resolution
- `src/render/shader_setup.h`:
  - Declare `SetupDrosteZoom(PostEffect* pe);`
- `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_DROSTE_ZOOM` case in GetTransformEffect()
  - Implement `SetupDrosteZoom()` binding all uniforms
- `src/render/render_pipeline.cpp`:
  - Add time accumulation: `pe->drosteZoomTime += deltaTime * pe->effects.drosteZoom.speed;`

**Done when**: Effect renders when enabled programmatically (no UI yet).

---

## Phase 3: UI Panel

**Goal**: Add controls to the Effects panel in Motion category.

**Build**:
- `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionDrosteZoom = false;`
  - In `DrawMotionCategory()`, add section after Infinite Zoom:
    - Checkbox: Enabled
    - SliderFloat: Speed [-2.0, 2.0]
    - SliderFloat: Scale [1.5, 10.0]
    - ModulatableSliderAngleDeg: Spiral Angle
    - ModulatableSlider: Twist [-1.0, 1.0]
    - TreeNode "Masking": innerRadius [0.0, 0.5]
    - TreeNode "Spiral": branches [1, 8]

**Done when**: All parameters adjustable via UI, effect visually working.

---

## Phase 4: Modulation and Serialization

**Goal**: Enable audio-reactive modulation and preset save/load.

**Build**:
- `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries for modulatable params:
    - `"drosteZoom.scale"` [1.5, 10.0]
    - `"drosteZoom.spiralAngle"` [-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX]
    - `"drosteZoom.twist"` [-1.0, 1.0]
    - `"drosteZoom.innerRadius"` [0.0, 0.5]
    - `"drosteZoom.branches"` [1.0, 8.0] (float for modulation)
  - Add targets[] pointers in ParamRegistryInit()
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrosteZoomConfig, enabled, speed, scale, spiralAngle, twist, innerRadius, branches)`
  - Add `if (e.drosteZoom.enabled) { j["drosteZoom"] = e.drosteZoom; }` in to_json
  - Add `e.drosteZoom = j.value("drosteZoom", e.drosteZoom);` in from_json

**Done when**: Presets save/load drosteZoom settings, modulation routes work.

---

## Phase 5: Verification

**Goal**: Verify full integration.

**Build**:
- Build project, fix any compilation errors
- Test effect with various parameter combinations
- Test preset save/load round-trip
- Test modulation routing (assign bass energy to scale)
- Test transform order reordering

**Done when**: Effect works correctly in all scenarios.

---

## Phase 6: Remove Droste Shear from Infinite Zoom

**Goal**: Remove the deprecated drosteShear parameter from Infinite Zoom now that proper Droste Zoom exists.

**Build**:
- `src/config/infinite_zoom_config.h`:
  - Remove `float drosteShear = 0.0f;` field
- `shaders/infinite_zoom.fs`:
  - Remove `uniform float drosteShear;`
  - Remove drosteShear usage from shader logic
- `src/render/post_effect.h`:
  - Remove `int infiniteZoomDrosteShearLoc;`
- `src/render/post_effect.cpp`:
  - Remove `GetShaderLocation()` call for drosteShear
- `src/render/shader_setup.cpp`:
  - Remove drosteShear uniform binding in `SetupInfiniteZoom()`
- `src/ui/imgui_effects_transforms.cpp`:
  - Remove `ModulatableSliderInt("Droste Shear##infzoom", ...)` line
- `src/automation/param_registry.cpp`:
  - Remove `{"infiniteZoom.drosteShear", {-8.0f, 8.0f}}` from PARAM_TABLE
  - Remove `&effects->infiniteZoom.drosteShear` from targets[]
- `src/config/preset.cpp`:
  - Remove `drosteShear` from NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for InfiniteZoomConfig

**Done when**: Infinite Zoom no longer has drosteShear parameter, builds clean, existing presets load without error (nlohmann handles missing fields gracefully).
