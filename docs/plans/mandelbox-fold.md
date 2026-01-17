# Mandelbox Fold

Add a Mandelbox-style UV transform combining box fold (conditional reflection at boundaries) and sphere fold (inversion through nested spheres). Creates crystalline-yet-organic fractal patterns distinct from existing KIFS.

## Current State

- `src/config/kifs_config.h` - Similar fractal fold config to reference
- `shaders/kifs.fs` - Similar shader structure to reference
- `docs/research/folding-techniques.md:16-127` - Complete algorithm specification
- Effect registration pattern established across 30+ effects

## Technical Implementation

**Source**: `docs/research/folding-techniques.md` (Box Fold, Sphere Fold, Mandelbox 2D sections)

### Core Algorithm

Each iteration applies box fold, then sphere fold, then scale:

```glsl
vec2 mandelboxFold(vec2 p, float boxLimit, float sphereMin, float sphereMax,
                   float scale, float boxMix, float sphereMix) {
    // Box fold: conditional reflection at +-boxLimit
    vec2 boxed = clamp(p, -boxLimit, boxLimit) * 2.0 - p;
    p = mix(p, boxed, boxMix);

    // Sphere fold: inversion through nested spheres
    float r2 = dot(p, p);
    float sphereMin2 = sphereMin * sphereMin;
    float sphereMax2 = sphereMax * sphereMax;

    vec2 sphered = p;
    if (r2 < sphereMin2) {
        sphered *= sphereMax2 / sphereMin2;  // Strong inversion
    } else if (r2 < sphereMax2) {
        sphered *= sphereMax2 / r2;          // Smooth falloff
    }
    p = mix(p, sphered, sphereMix);

    // Scale
    p *= scale;

    return p;
}
```

### UV Transformations

1. Center UV: `p = (fragTexCoord - 0.5) * 2.0`
2. Apply global rotation (CPU-accumulated)
3. Iterate: boxFold -> sphereFold -> scale -> translate -> twist
4. Mirror for tiling: `abs(fract(p / 2.0) - 0.5) * 2.0`
5. Sample texture at transformed UV

### Parameters

| Uniform | Type | Range | Default | Effect |
|---------|------|-------|---------|--------|
| iterations | int | 1-12 | 4 | Recursion depth |
| boxLimit | float | 0.5-2.0 | 1.0 | Box fold boundary |
| sphereMin | float | 0.1-0.5 | 0.25 | Inner sphere radius (strong inversion) |
| sphereMax | float | 0.5-2.0 | 1.0 | Outer sphere radius |
| scale | float | 1.5-3.0 | 2.0 | Expansion per iteration |
| offset | vec2 | 0.0-2.0 | (1.0, 1.0) | Translation after fold |
| rotation | float | - | accumulated | Global rotation (radians) |
| twistAngle | float | - | accumulated | Per-iteration rotation (radians) |
| boxIntensity | float | 0.0-1.0 | 1.0 | Box fold contribution |
| sphereIntensity | float | 0.0-1.0 | 1.0 | Sphere fold contribution |

---

## Phase 1: Config and Shader

**Goal**: Create the effect's data structure and GLSL shader.

**Build**:
- Create `src/config/mandelbox_config.h` with MandelboxConfig struct containing all parameters
- Create `shaders/mandelbox.fs` implementing the algorithm above

**Done when**: Config header compiles, shader file exists.

---

## Phase 2: Effect Registration

**Goal**: Wire the effect into the transform system.

**Build**:
- `src/config/effect_config.h`:
  - Add `#include "mandelbox_config.h"`
  - Add `TRANSFORM_MANDELBOX` to enum (before TRANSFORM_EFFECT_COUNT)
  - Add case in `TransformEffectName()` returning "Mandelbox"
  - Add `TRANSFORM_MANDELBOX` to `TransformOrderConfig::order` array (commonly missed)
  - Add `MandelboxConfig mandelbox;` member to EffectConfig
  - Add case in `IsTransformEnabled()`

**Done when**: Effect enum recognized, order array updated.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- `src/render/post_effect.h`:
  - Add `Shader mandelboxShader;`
  - Add uniform location ints: `mandelboxIterationsLoc`, `mandelboxBoxLimitLoc`, `mandelboxSphereMinLoc`, `mandelboxSphereMaxLoc`, `mandelboxScaleLoc`, `mandelboxOffsetLoc`, `mandelboxRotationLoc`, `mandelboxTwistAngleLoc`, `mandelboxBoxIntensityLoc`, `mandelboxSphereIntensityLoc`
  - Add accumulator: `float currentMandelboxRotation;`, `float currentMandelboxTwist;`
- `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get all uniform locations in `GetShaderUniformLocations()`
  - Unload in `PostEffectUninit()`

**Done when**: Shader loads without errors, uniform locations cached.

---

## Phase 4: Shader Setup and Accumulation

**Goal**: Write uniforms and accumulate rotation on CPU.

**Build**:
- `src/render/shader_setup.h`: Declare `void SetupMandelbox(PostEffect* pe);`
- `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_MANDELBOX` case in `GetTransformEffect()`
  - Implement `SetupMandelbox()` writing all uniforms
- `src/render/render_pipeline.cpp`:
  - Add rotation accumulation: `pe->currentMandelboxRotation += pe->effects.mandelbox.rotationSpeed;`
  - Add twist accumulation: `pe->currentMandelboxTwist += pe->effects.mandelbox.twistSpeed;`

**Done when**: Effect renders when enabled.

---

## Phase 5: UI Panel

**Goal**: Add controls to the Effects panel.

**Build**:
- `src/ui/imgui_effects.cpp`: Add `TRANSFORM_MANDELBOX` case in `GetTransformCategory()` returning `CATEGORY_SYMMETRY`
- `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionMandelbox = false;`
  - Add `DrawSymmetryMandelbox()` helper with all parameter controls
  - Call helper from `DrawSymmetryCategory()` with spacing
  - Use `ModulatableSliderAngleDeg` for rotationSpeed and twistSpeed

**Done when**: UI controls appear and modify effect in real-time.

---

## Phase 6: Serialization and Modulation

**Goal**: Enable preset save/load and audio reactivity.

**Build**:
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MandelboxConfig, ...)` macro
  - Add to_json entry
  - Add from_json entry
- `src/automation/param_registry.cpp`:
  - Add `{"mandelbox.rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
  - Add `{"mandelbox.twistSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
  - Add corresponding pointer entries to targets array (must match indices)

**Done when**: Preset save/load works, modulation routes to angular params.

---

## Verification Checklist

- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Symmetry" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to pipeline list
- [ ] Box fold creates hard-edged reflections
- [ ] Sphere fold creates bulging distortions
- [ ] Mix sliders blend fold contributions smoothly
- [ ] Rotation/twist animate when speed > 0
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to rotationSpeed and twistSpeed
- [ ] Build succeeds with no warnings
