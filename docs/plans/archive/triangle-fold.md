# Triangle Fold

Iterated Sierpinski-style UV transform that folds space into equilateral triangles. Creates 3-fold/6-fold gasket patterns with fractal self-similarity when iterated with scale and offset.

## Current State

- `src/config/mandelbox_config.h` - Reference for iterated fold config structure
- `shaders/mandelbox.fs` - Reference for fold shader iteration pattern
- `docs/research/folding-techniques.md:129-158` - Triangle fold algorithm specification
- Effect registration pattern established across 31 effects

## Technical Implementation

**Source**: `docs/research/folding-techniques.md` (Triangle/Sierpinski Fold section)

### Core Algorithm

Folds UV into equilateral triangle wedge (60° sector), then iterates with scale and translation:

```glsl
vec2 triangleFold(vec2 p) {
    const float SQRT3 = 1.7320508;

    // Fold 1: Reflect across Y axis (creates left-right symmetry)
    p.x = abs(p.x);

    // Fold 2: Reflect across 60° line (y = sqrt(3) * x)
    // Normal to 60° line: n = normalize(sqrt(3), -1) = (sqrt(3)/2, -0.5)
    vec2 n = vec2(SQRT3, -1.0) * 0.5;
    float d = dot(p, n);
    if (d > 0.0) {
        p -= 2.0 * d * n;  // Reflect across line
    }

    return p;
}
```

### UV Transformations

1. Center UV: `p = (fragTexCoord - 0.5) * 2.0`
2. Apply global rotation (CPU-accumulated)
3. For each iteration:
   - `p = triangleFold(p)`
   - `p = p * scale - offset`
   - `p = rotate2d(p, twistAngle)`
4. Mirror for seamless tiling: `abs(fract(p / 2.0) - 0.5) * 2.0`
5. Sample texture at transformed UV

### Parameters

| Uniform | Type | Range | Default | Effect |
|---------|------|-------|---------|--------|
| iterations | int | 1-8 | 3 | Recursion depth |
| scale | float | 1.5-3.0 | 2.0 | Expansion per iteration |
| offsetX | float | 0.0-2.0 | 0.5 | X translation after fold |
| offsetY | float | 0.0-2.0 | 0.5 | Y translation after fold |
| rotation | float | - | accumulated | Global rotation (radians) |
| twistAngle | float | - | accumulated | Per-iteration rotation (radians) |

---

## Phase 1: Config and Shader

**Goal**: Create the effect's data structure and GLSL shader.

**Build**:
- Create `src/config/triangle_fold_config.h`:
  - `enabled` (bool, default false)
  - `iterations` (int, default 3, range 1-8)
  - `scale` (float, default 2.0)
  - `offsetX`, `offsetY` (float, default 0.5 each)
  - `rotationSpeed` (float, radians/frame, default 0.002)
  - `twistSpeed` (float, radians/frame, default 0.0)
- Create `shaders/triangle_fold.fs` implementing algorithm above

**Done when**: Config header compiles, shader file exists.

---

## Phase 2: Effect Registration

**Goal**: Wire the effect into the transform system.

**Build**:
- `src/config/effect_config.h`:
  - Add `#include "triangle_fold_config.h"`
  - Add `TRANSFORM_TRIANGLE_FOLD` to enum (before TRANSFORM_EFFECT_COUNT)
  - Add case in `TransformEffectName()` returning "Triangle Fold"
  - Add `TRANSFORM_TRIANGLE_FOLD` to `TransformOrderConfig::order` array
  - Add `TriangleFoldConfig triangleFold;` member to EffectConfig
  - Add case in `IsTransformEnabled()`

**Done when**: Effect enum recognized, order array updated.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- `src/render/post_effect.h`:
  - Add `Shader triangleFoldShader;`
  - Add uniform locations: `triangleFoldIterationsLoc`, `triangleFoldScaleLoc`, `triangleFoldOffsetLoc`, `triangleFoldRotationLoc`, `triangleFoldTwistAngleLoc`
  - Add accumulators: `float currentTriangleFoldRotation;`, `float currentTriangleFoldTwist;`
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
- `src/render/shader_setup.h`: Declare `void SetupTriangleFold(PostEffect* pe);`
- `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_TRIANGLE_FOLD` case in `GetTransformEffect()`
  - Implement `SetupTriangleFold()` writing all uniforms
- `src/render/render_pipeline.cpp`:
  - Add rotation accumulation: `pe->currentTriangleFoldRotation += pe->effects.triangleFold.rotationSpeed;`
  - Add twist accumulation: `pe->currentTriangleFoldTwist += pe->effects.triangleFold.twistSpeed;`

**Done when**: Effect renders when enabled.

---

## Phase 5: UI Panel

**Goal**: Add controls to the Effects panel.

**Build**:
- `src/ui/imgui_effects.cpp`: Add `TRANSFORM_TRIANGLE_FOLD` case in `GetTransformCategory()` returning `CATEGORY_SYMMETRY`
- `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionTriangleFold = false;`
  - Add `DrawSymmetryTriangleFold()` helper with all parameter controls
  - Call helper from `DrawSymmetryCategory()` with spacing after Mandelbox
  - Use `ModulatableSliderAngleDeg` for rotationSpeed and twistSpeed

**Done when**: UI controls appear and modify effect in real-time.

---

## Phase 6: Serialization and Modulation

**Goal**: Enable preset save/load and audio reactivity.

**Build**:
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriangleFoldConfig, enabled, iterations, scale, offsetX, offsetY, rotationSpeed, twistSpeed)`
  - Add to_json entry
  - Add from_json entry
- `src/automation/param_registry.cpp`:
  - Add `{"triangleFold.rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
  - Add `{"triangleFold.twistSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
  - Add corresponding pointer entries to targets array (must match indices)

**Done when**: Preset save/load works, modulation routes to angular params.

---

## Verification Checklist

- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Symmetry" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to pipeline list
- [ ] Triangle fold creates 3-fold/6-fold symmetry
- [ ] Increasing iterations deepens fractal detail
- [ ] Scale > 2.0 zooms into fractal structure
- [ ] Rotation/twist animate when speed > 0
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to rotationSpeed and twistSpeed
- [ ] Build succeeds with no warnings
