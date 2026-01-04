# Möbius Transform Effect

Add Möbius transformation as a toggleable post-process UV warp effect. Also add an explicit `enabled` toggle to kaleidoscope for UI consistency.

## Current State

Files to modify/create:
- `src/config/mobius_config.h` (new) - Config struct with 8 floats + enabled bool
- `src/config/effect_config.h:32` - Include mobius config
- `shaders/mobius.fs` (new) - GLSL fragment shader
- `src/render/post_effect.h:28` - Add shader and uniform location fields
- `src/render/post_effect.cpp:31` - Load shader, get uniforms, unload
- `src/render/render_pipeline.cpp:463` - Add render pass before kaleidoscope
- `src/ui/imgui_effects.cpp:42` - Add collapsible UI section
- `src/config/preset.cpp:93` - Add serialization macro
- `src/config/kaleidoscope_config.h:11` - Add `enabled` bool field

## Phase 1: Shader and Config

**Goal**: Create the Möbius shader and config struct.

**Build**:
- Create `src/config/mobius_config.h` with:
  - `bool enabled` (default false)
  - `float aReal, aImag` (default 1.0, 0.0 = identity scale)
  - `float bReal, bImag` (default 0.0, 0.0 = no translation)
  - `float cReal, cImag` (default 0.0, 0.0 = no pole)
  - `float dReal, dImag` (default 1.0, 0.0 = identity denominator)
- Include in `effect_config.h` and add `MobiusConfig mobius` field to `EffectConfig`
- Create `shaders/mobius.fs`:
  - Uniforms for all 8 floats
  - Complex multiply/divide helpers (from research doc)
  - Apply `w = (az + b) / (cz + d)` to centered UV
  - Handle singularity (clamp denominator magnitude)
  - Sample texture at warped UV

**Done when**: Files compile, shader syntax valid.

---

## Phase 2: PostEffect Integration

**Goal**: Load the shader and wire up uniform locations.

**Build**:
- In `post_effect.h`:
  - Add `Shader mobiusShader`
  - Add uniform location ints: `mobiusALoc`, `mobiusBLoc`, `mobiusCLoc`, `mobiusDLoc`
- In `post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Get uniform locations in `GetShaderUniformLocations()`
  - Add to shader validation check
  - Unload in `PostEffectUninit()`

**Done when**: App starts without shader load errors.

---

## Phase 3: Render Pipeline Pass

**Goal**: Apply Möbius as a render pass before kaleidoscope.

**Build**:
- In `render_pipeline.cpp`:
  - Add `SetupMobius()` function that packs params into vec2s and sets uniforms
  - Add conditional pass in `RenderPipelineApplyOutput()` before kaleidoscope pass
  - Enable condition: `pe->effects.mobius.enabled`

**Done when**: Setting `enabled = true` in code applies visible UV warping.

---

## Phase 4: UI and Preset Serialization

**Goal**: Add UI controls and preset save/load support.

**Build**:
- In `imgui_effects.cpp`:
  - Add `static bool sectionMobius = false`
  - Add collapsible section with `enabled` checkbox and 8 float sliders
  - Place before kaleidoscope section
  - Slider ranges: `a` components ±2.0, `b` components ±1.0, `c` components ±0.5, `d` components ±2.0
- In `preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MobiusConfig, ...)`
  - Add `mobius` to `EffectConfig` serialization list

**Done when**: Can toggle effect in UI, values persist across preset save/load.

---

## Phase 5: Kaleidoscope Consistency

**Goal**: Add explicit `enabled` toggle to kaleidoscope for UI consistency.

**Build**:
- In `kaleidoscope_config.h`:
  - Add `bool enabled = false` field
- In `imgui_effects.cpp`:
  - Add `Checkbox("Enabled##kaleido", ...)` at top of kaleidoscope section
  - Wrap existing controls in `if (e->kaleidoscope.enabled)` block
- In `render_pipeline.cpp`:
  - Change condition from `segments > 1` to `kaleidoscope.enabled`
- In `preset.cpp`:
  - Add `enabled` to `KaleidoscopeConfig` serialization macro

**Done when**: Kaleidoscope uses checkbox toggle, existing presets still load (defaults to false, segments value preserved).

---

## Phase 6: Iterated Möbius with Depth Accumulation

**Goal**: Replace simple Möbius transform with iterative depth-accumulated version from research.

The new technique iteratively applies animated Möbius transforms and accumulates weighted texture samples at each iteration depth. This creates heavy, psychedelic distortion without visible tiling.

**Build**:
- Update `src/config/mobius_config.h`:
  - Remove individual a/b/c/d components (8 floats)
  - Add `int iterations` (default 6, range 1-12)
  - Add `float animSpeed` (default 0.3, range 0.0-2.0)
  - Add `float poleMagnitude` (default 0.1, range 0.0-0.5) - controls c coefficient magnitude
  - Add `float rotationSpeed` (default 0.3, range 0.0-2.0) - controls a coefficient rotation
- Update `shaders/mobius.fs`:
  - Add `uniform float time`, `uniform int iterations`, `uniform float animSpeed`, `uniform float poleMagnitude`, `uniform float rotationSpeed`
  - Remove static a/b/c/d uniforms
  - Implement iterative loop with animated params per iteration
  - Accumulate weighted samples using `1.0 / float(i + 2)` weighting
  - Use `sin()` for smooth UV remapping to avoid hard edges
- Update `src/render/post_effect.h`:
  - Replace `mobiusALoc/B/C/D` with `mobiusTimeLoc`, `mobiusIterationsLoc`, `mobiusAnimSpeedLoc`, `mobiusPoleMagLoc`, `mobiusRotSpeedLoc`
  - Add `float mobiusTime` to PostEffect struct
- Update `src/render/post_effect.cpp`:
  - Update `GetShaderUniformLocations()` with new uniform names
- Update `src/render/render_pipeline.cpp`:
  - Update `SetupMobius()` to pass new uniforms
  - Accumulate `pe->mobiusTime` in `RenderPipelineApplyOutput()`
- Update `src/ui/imgui_effects.cpp`:
  - Replace 8 float sliders with: Iterations (int), Anim Speed, Pole Magnitude, Rotation Speed
- Update `src/config/preset.cpp`:
  - Update `MobiusConfig` serialization macro with new fields

**Done when**: Effect creates smooth, flowing distortion with visible animation and no hard edges. Old presets load with defaults.
