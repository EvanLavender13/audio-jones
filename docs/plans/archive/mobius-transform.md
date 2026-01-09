# Mobius Transform

Conformal UV warp via complex-plane fractional linear transformation. Two fixed points define the transform geometry; `spiralTightness` and `zoomFactor` control log-polar spiral character. Animating these over time creates flowing spiral, zoom, and rotation effects.

> **Reviewer Note**: The implemented algorithm differs from the plan below. Instead of separate hyperbolic/elliptic transforms, the shader uses Shane's two-point Mobius form followed by log-polar spiral animation (based on [ShaderToy 4sGXDK](https://www.shadertoy.com/view/4sGXDK)). Point2 is the pole (singularity).

## Current State

Files to modify/create:
- `src/config/mobius_config.h` (new) - Config struct
- `src/config/effect_config.h:15` - Add enum value and config field
- `shaders/mobius.fs` (new) - Fragment shader
- `src/render/post_effect.h:31` - Shader handle and uniform locations
- `src/render/post_effect.cpp:41` - Load shader, get uniforms
- `src/render/shader_setup.h:33` - Declare SetupMobius
- `src/render/shader_setup.cpp:25` - Implement SetupMobius, add to dispatch
- `src/render/render_pipeline.cpp:144` - Time accumulation, Lissajous computation
- `src/ui/imgui_effects.cpp:404` - UI section in Warp category
- `src/config/preset.cpp:118` - Serialization macro
- `src/automation/param_registry.cpp:59` - Register modulatable params

## Technical Implementation

**Source**: [Research doc](../research/mobius-transform.md), [neozhaoliang ShaderToy](https://www.shadertoy.com/view/XsS3Dm)

### Core Algorithm

The Mobius transform maps UV coordinates through the complex plane. For animated elliptic/hyperbolic/loxodromic effects, apply log-polar transformations centered on the midpoint between two fixed points.

### Complex Arithmetic Helpers

```glsl
vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

vec2 cdiv(vec2 a, vec2 b) {
    return vec2(a.x*b.x + a.y*b.y, -a.x*b.y + a.y*b.x) / dot(b, b);
}
```

### Hyperbolic Transform (Scaling Flow)

Operates in log-polar space. `rho` controls expansion rate.

```glsl
vec2 trans_hyperbolic(vec2 p, float rho) {
    float r = length(p);
    if (r < 0.0001) return p;
    float logR = log(r) - rho;
    return normalize(p) * exp(logR);
}
```

### Elliptic Transform (Rotation)

Pure rotation by angle `alpha`.

```glsl
vec2 trans_elliptic(vec2 p, float alpha) {
    float c = cos(alpha), s = sin(alpha);
    return vec2(c*p.x - s*p.y, s*p.x + c*p.y);
}
```

### Loxodromic Transform (Spiral)

Combines hyperbolic and elliptic.

```glsl
vec2 trans_loxodromic(vec2 p, float rho, float alpha) {
    p = trans_hyperbolic(p, rho);
    p = trans_elliptic(p, alpha);
    return p;
}
```

### Full Shader Flow

1. Convert UV to centered coordinates `[-0.5, 0.5]`
2. Compute midpoint between fixed points
3. Translate to midpoint-centered space
4. Apply loxodromic transform with `time * animSpeed * rho` and `time * animSpeed * alpha`
5. Translate back
6. Sample texture at warped UV

### Singularity Handling

When UV approaches a fixed point, the transform can diverge. Clamp minimum radius:

```glsl
float r = max(length(p), 0.001);
```

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `enabled` | bool | - | false | Enable effect |
| `point1X` | float | 0.0-1.0 | 0.3 | Fixed point 1 X (UV space) |
| `point1Y` | float | 0.0-1.0 | 0.5 | Fixed point 1 Y |
| `point2X` | float | 0.0-1.0 | 0.7 | Fixed point 2 X |
| `point2Y` | float | 0.0-1.0 | 0.5 | Fixed point 2 Y |
| `rho` | float | -2.0-2.0 | 0.0 | Hyperbolic strength (expansion rate) |
| `alpha` | float | -PI-PI | 0.0 | Elliptic strength (rotation rate) |
| `animSpeed` | float | 0.0-2.0 | 1.0 | Time multiplier for rho/alpha |
| `pointAmplitude` | float | 0.0-0.3 | 0.0 | Lissajous motion amplitude |
| `pointFreq1` | float | 0.1-5.0 | 1.0 | Point 1 oscillation frequency |
| `pointFreq2` | float | 0.1-5.0 | 1.3 | Point 2 oscillation frequency |

---

## Phase 1: Config and Shader

**Goal**: Create config struct and shader with core transform logic.

**Build**:
- Create `src/config/mobius_config.h` with all parameters listed above
- Add `#include "mobius_config.h"` to `effect_config.h`
- Add `TRANSFORM_MOBIUS` to `TransformEffectType` enum (before `TRANSFORM_EFFECT_COUNT`)
- Add `"Mobius"` case to `TransformEffectName()`
- Add `TRANSFORM_MOBIUS` to `TransformOrderConfig::order` default array
- Add `MobiusConfig mobius` field to `EffectConfig` struct
- Create `shaders/mobius.fs` with:
  - Uniforms: `time`, `point1`, `point2`, `rho`, `alpha`
  - Complex arithmetic helpers
  - Hyperbolic/elliptic/loxodromic functions
  - Main: center UV, compute midpoint, apply transform, sample

**Done when**: Files compile, shader syntax validates.

---

## Phase 2: PostEffect and Pipeline Integration

**Goal**: Load shader, wire uniforms, add render pass.

**Build**:
- In `post_effect.h`:
  - Add `Shader mobiusShader`
  - Add uniform locations: `mobiusTimeLoc`, `mobiusPoint1Loc`, `mobiusPoint2Loc`, `mobiusRhoLoc`, `mobiusAlphaLoc`
  - Add `float mobiusTime` for time accumulation
- In `post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Get uniform locations in `GetShaderUniformLocations()`
  - Add to shader validation check
  - Initialize `mobiusTime = 0.0f`
  - Unload in `PostEffectUninit()`
- In `shader_setup.h`:
  - Declare `void SetupMobius(PostEffect* pe)`
- In `shader_setup.cpp`:
  - Add `TRANSFORM_MOBIUS` case to `GetTransformEffect()` returning `{&pe->mobiusShader, SetupMobius, &pe->effects.mobius.enabled}`
  - Implement `SetupMobius()`: set all uniforms from config
- In `render_pipeline.cpp`:
  - Add `pe->mobiusTime += deltaTime * pe->effects.mobius.animSpeed` in `RenderPipelineApplyFeedback()`
  - Compute Lissajous point offsets in `RenderPipelineApplyOutput()` before transform loop
  - Store computed points in `pe->currentMobiusPoint1/2` temporaries

**Done when**: App starts without errors, enabling Mobius in code shows visible UV warping.

---

## Phase 3: UI and Serialization

**Goal**: Add UI controls and preset save/load.

**Build**:
- In `imgui_effects.cpp`:
  - Add `static bool sectionMobius = false` with other section states
  - Add Mobius section in Warp category (after Wave Ripple):
    - Enabled checkbox
    - rho slider (-2.0 to 2.0)
    - alpha slider (use `SliderAngleDeg`, store radians, display degrees)
    - animSpeed slider (0.0 to 2.0)
    - TreeNode "Fixed Points" with point1X/Y, point2X/Y sliders (0.0 to 1.0)
    - TreeNode "Point Motion" with amplitude and freq sliders
  - Add `TRANSFORM_MOBIUS` case to effect order list's enabled check
- In `preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MobiusConfig, ...)`
  - Add `if (e.mobius.enabled) { j["mobius"] = e.mobius; }` in EffectConfig to_json
  - Add `e.mobius = j.value("mobius", e.mobius)` in EffectConfig from_json

**Done when**: Can toggle effect in UI, adjust all parameters, values persist across preset save/load.

---

## Phase 4: Modulation Registration

**Goal**: Register modulatable parameters for audio reactivity.

**Build**:
- In `param_registry.cpp`:
  - Add entries to `PARAM_TABLE`:
    - `{"mobius.rho", {-2.0f, 2.0f}}`
    - `{"mobius.alpha", {-3.14159f, 3.14159f}}`
    - `{"mobius.point1X", {0.0f, 1.0f}}`
    - `{"mobius.point1Y", {0.0f, 1.0f}}`
    - `{"mobius.point2X", {0.0f, 1.0f}}`
    - `{"mobius.point2Y", {0.0f, 1.0f}}`
  - Add corresponding pointers to targets array in `ParamRegistryRegisterEffects()`
- Update `ModulatableSlider` calls in UI for registered params

**Done when**: Can route audio/LFO to mobius parameters, modulation affects transform in real-time.
