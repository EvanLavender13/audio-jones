# Generator Blend Modes

Generators (constellation, plasma, interference) hardcode additive blending in their shaders. This refactor strips the additive blend, adds per-generator blend mode + intensity via the existing `BlendCompositor`, and promotes generators to reorderable `TransformEffectType` entries. Also adds a Solid Color generator for flat-color overlays with configurable blending.

**Research**: `docs/research/generator-blend-modes.md`

## Design

### Types

**New fields on existing generator configs** (`ConstellationConfig`, `PlasmaConfig`, `InterferenceConfig`):

```
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

Screen default approximates the current additive behavior for backward-compatible presets.

**New config struct** (`SolidColorConfig`):

```
bool enabled = false
float color[3] = {1.0f, 1.0f, 1.0f}   // RGB 0-1
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**New effect struct** (`SolidColorEffect`):

```
Shader shader
int colorLoc
```

No time accumulators — the shader is stateless.

**PostEffect additions**:

```
RenderTexture2D generatorScratch   // Shared scratch texture for generator blend rendering
SolidColorEffect solidColor        // Solid Color effect instance
bool constellationBlendActive      // Per-frame active flag (enabled && blendIntensity > 0)
bool plasmaBlendActive
bool interferenceBlendActive
bool solidColorBlendActive
```

### Rendering Approach

**Current flow**: Each generator shader reads `texture0` and additively blends: `finalColor = vec4(source + generated, 1.0)`. Generators run as hardcoded passes before the transform loop.

**New flow**: Two-step render per generator, integrated into the transform loop:

1. **Render to scratch**: `RenderPass(pe, src, &pe->generatorScratch, generatorShader, generatorSetup)` — the generator shader outputs raw content only (`finalColor = vec4(generated, 1.0)`), ignoring `texture0`
2. **Composite via BlendCompositor**: `RenderPass(pe, src, &pe->pingPong[writeIdx], blendCompositor->shader, blendSetup)` — `effect_blend.fs` composites `generatorScratch.texture` onto `src` using the configured blend mode and intensity

The transform loop gets a new helper `RenderGeneratorToScratch(pe, effectType)` that dispatches step 1. Step 2 uses the standard `RenderPass` path with the `BlendCompositor` shader.

This mirrors the bloom multi-pass pattern: `ApplyBloomPasses` → `RenderPass`.

### Shader Changes

Strip `texture0` read and additive blend from three generator shaders:

**constellation.fs** (lines 152-153):
- Before: `vec3 source = texture(texture0, fragTexCoord).rgb; finalColor = vec4(source + constellation, 1.0);`
- After: `finalColor = vec4(constellation, 1.0);`

**plasma.fs** (lines 135-136):
- Before: `vec3 source = texture(texture0, fragTexCoord).rgb; finalColor = vec4(source + total, 1.0);`
- After: `finalColor = vec4(total, 1.0);`

**interference.fs** (line 192):
- Before: `finalColor = vec4(srcColor.rgb + color * brightness, 1.0);`
- After: `finalColor = vec4(color * brightness, 1.0);`

Also remove the `texture0` sampler declaration from each shader if it becomes unused after the edit.

**solid_color.fs** — new, minimal:
- Reads `uniform vec3 color`
- Outputs `finalColor = vec4(color, 1.0)`
- No `texture0` read

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `constellation.blendIntensity` | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |
| `constellation.blendMode` | EffectBlendMode | 0-15 | SCREEN | No | "Blend Mode" |
| `plasma.blendIntensity` | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |
| `plasma.blendMode` | EffectBlendMode | 0-15 | SCREEN | No | "Blend Mode" |
| `interference.blendIntensity` | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |
| `interference.blendMode` | EffectBlendMode | 0-15 | SCREEN | No | "Blend Mode" |
| `solidColor.color` | vec3 | 0.0-1.0 per channel | (1,1,1) | Yes (R/G/B) | "Color" |
| `solidColor.blendIntensity` | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |
| `solidColor.blendMode` | EffectBlendMode | 0-15 | SCREEN | No | "Blend Mode" |

### Constants

**TransformEffectType enum entries** (before `TRANSFORM_EFFECT_COUNT`):

| Enum | Display Name | Category Badge |
|------|-------------|---------------|
| `TRANSFORM_CONSTELLATION_BLEND` | "Constellation Blend" | `{"GEN", 10}` |
| `TRANSFORM_PLASMA_BLEND` | "Plasma Blend" | `{"GEN", 10}` |
| `TRANSFORM_INTERFERENCE_BLEND` | "Interference Blend" | `{"GEN", 10}` |
| `TRANSFORM_SOLID_COLOR` | "Solid Color" | `{"GEN", 10}` |

Section index 10 follows the existing 0-9 range (SYM, WARP, CELL, MOT, ART, GFX, RET, OPT, COL, SIM).

---

## Tasks

### Wave 1: Foundation (Config + Shader + New Effect Module)

#### Task 1.1: Add blend fields to generator configs

**Files**: `src/effects/constellation.h`, `src/effects/plasma.h`, `src/effects/interference.h`

**Creates**: `blendMode` and `blendIntensity` fields that all subsequent tasks reference

**Do**:
- Add `#include "render/blend_mode.h"` to each header
- Add `EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;` and `float blendIntensity = 1.0f;` to each config struct (after existing fields, before closing brace)

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Create Solid Color effect module

**Files**: `src/effects/solid_color.h` (create), `src/effects/solid_color.cpp` (create)

**Creates**: `SolidColorConfig`, `SolidColorEffect`, lifecycle functions

**Do**:
- Follow the standard effect module pattern (same structure as `constellation.h`)
- `SolidColorConfig`: `enabled`, `color[3]`, `blendMode`, `blendIntensity` (see Design > Types)
- `SolidColorEffect`: `shader`, `colorLoc`
- `SolidColorEffectInit`: load `shaders/solid_color.fs`, cache `colorLoc`
- `SolidColorEffectSetup`: bind `color` uniform (no time accumulation)
- `SolidColorEffectUninit`: unload shader
- `SolidColorConfigDefault`: return default config
- `SolidColorRegisterParams`: register `solidColor.color` (3 channels), `solidColor.blendIntensity`

**Verify**: File exists, compiles when added to CMake (Wave 2).

#### Task 1.3: Create Solid Color shader

**Files**: `shaders/solid_color.fs` (create)

**Creates**: Fragment shader for flat color output

**Do**:
- Standard `#version 330` header with `fragTexCoord`, `finalColor`
- Uniform: `vec3 color`
- Output: `finalColor = vec4(color, 1.0);`
- No `texture0` read

**Verify**: File exists, valid GLSL.

#### Task 1.4: Strip additive blend from generator shaders

**Files**: `shaders/constellation.fs`, `shaders/plasma.fs`, `shaders/interference.fs`

**Creates**: Raw-output shaders that no longer read `texture0`

**Do**:
- See Design > Shader Changes for exact before/after per shader
- Remove the `vec3 source = texture(texture0, ...)` lines
- Change output to raw generated content only
- Remove `uniform sampler2D texture0;` if no longer referenced

**Verify**: Files exist, valid GLSL (visual verification deferred to final).

---

### Wave 2: Integration (all parallel — no file overlap)

#### Task 2.1: Register transform entries

**Files**: `src/config/effect_config.h`

**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/solid_color.h"` with other includes
- Add 4 enum entries before `TRANSFORM_EFFECT_COUNT`: `TRANSFORM_CONSTELLATION_BLEND`, `TRANSFORM_PLASMA_BLEND`, `TRANSFORM_INTERFERENCE_BLEND`, `TRANSFORM_SOLID_COLOR`
- Add 4 entries to `TRANSFORM_EFFECT_NAMES` array (matching enum order)
- Add `SolidColorConfig solidColor;` to `EffectConfig` struct (with comment)
- Add 4 cases to `IsTransformEnabled()`:
  - Constellation/Plasma/Interference: `return e->xxx.enabled && e->xxx.blendIntensity > 0.0f;` (same pattern as simulation boosts)
  - SolidColor: `return e->solidColor.enabled && e->solidColor.blendIntensity > 0.0f;`
- Add 4 entries to `TransformOrderConfig::order` initialization (they're appended by the sequential default init, so no manual change needed — the `TRANSFORM_EFFECT_COUNT` increase handles it)

**Verify**: Compiles.

#### Task 2.2: PostEffect integration

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Depends on**: Wave 1 complete

**Do**:

`post_effect.h`:
- Add `#include "effects/solid_color.h"` (if not auto-included via effect_config.h)
- Add to `PostEffect` struct: `RenderTexture2D generatorScratch;`, `SolidColorEffect solidColor;`, and 4 `bool xxxBlendActive;` flags (see Design > Types)

`post_effect.cpp`:
- In `PostEffectInit`: allocate `generatorScratch` via `LoadRenderTexture(screenWidth, screenHeight)`, init `SolidColorEffect`
- In `PostEffectUninit`: unload `generatorScratch`, uninit `SolidColorEffect`
- In `PostEffectResize`: unload and reload `generatorScratch` at new dimensions
- In `PostEffectRegisterParams`: call `SolidColorRegisterParams(&pe->effects.solidColor)`

**Verify**: Compiles.

#### Task 2.3: Shader setup and transform dispatch

**Files**: `src/render/shader_setup_generators.h`, `src/render/shader_setup_generators.cpp`, `src/render/shader_setup.cpp`

**Depends on**: Wave 1 complete

**Do**:

`shader_setup_generators.h`:
- Add declarations: `void SetupSolidColor(PostEffect *pe);`
- Add declarations for 4 blend setup functions: `void SetupConstellationBlend(PostEffect *pe);`, `void SetupPlasmaBlend(PostEffect *pe);`, `void SetupInterferenceBlend(PostEffect *pe);`, `void SetupSolidColorBlend(PostEffect *pe);`
- Add declaration: `void RenderGeneratorToScratch(PostEffect *pe, TransformEffectType type);`

`shader_setup_generators.cpp`:
- Add `#include "render/blend_compositor.h"`
- Add `SetupSolidColor`: calls `SolidColorEffectSetup`
- Add 4 blend setup functions: each calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, config.blendIntensity, config.blendMode)` — same pattern as `SetupTrailBoost` but using `generatorScratch.texture` instead of `TrailMapGetTexture`
- Add `RenderGeneratorToScratch`: switch on `effectType`, call `RenderPass(pe, src, &pe->generatorScratch, shader, setup)` with the generator's own shader + setup function. The `src` parameter is unused by the generator shader (raw output) but required by `RenderPass` signature. Accepts a `RenderTexture2D *src` parameter.

`shader_setup.cpp`:
- Add 4 `GetTransformEffect` cases returning `{&pe->blendCompositor->shader, SetupXxxBlend, &pe->xxxBlendActive}` — same pattern as simulation boost cases

**Verify**: Compiles.

#### Task 2.4: Render pipeline changes

**Files**: `src/render/render_pipeline.cpp`

**Depends on**: Wave 1 complete

**Do**:
- Remove the 3 hardcoded generator pass blocks (lines 332-354: constellation, plasma, interference `if (enabled)` blocks)
- Add per-frame active flag computation for the 4 generator blend entries (same pattern as `pe->physarumBoostActive` at lines 302-321): `pe->constellationBlendActive = (pe->effects.constellation.enabled && pe->effects.constellation.blendIntensity > 0.0f);` (and same for plasma, interference, solidColor)
- Add helper `IsGeneratorBlendEffect(TransformEffectType)` — returns true for the 4 generator transform types
- In the transform loop, add an `else if (IsGeneratorBlendEffect(effectType))` branch (alongside bloom, anamorphic, oil paint special cases): call `RenderGeneratorToScratch(pe, effectType, src)` then `RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup)`

**Verify**: Compiles.

#### Task 2.5: UI panel updates

**Files**: `src/ui/imgui_effects_generators.cpp`, `src/ui/imgui_effects_generators.h`, `src/ui/imgui_effects.cpp`

**Depends on**: Wave 1 complete

**Do**:

`imgui_effects_generators.cpp`:
- Add includes: `"render/blend_mode.h"`, `"effects/solid_color.h"`
- Add blend mode names array (same as in `imgui_effects.cpp`): `BLEND_MODES[]` and `BLEND_MODE_COUNT`
- For each existing generator section (Constellation, Plasma, Interference): add an "Output" separator + blend intensity `ModulatableSlider` + blend mode `ImGui::Combo` after the existing controls. Follow the simulation panel pattern from `imgui_effects.cpp`. Also add `MoveTransformToEnd(&e->transformOrder, TRANSFORM_XXX_BLEND)` on first enable.
- Add `static bool sectionSolidColor = false;` and `DrawGeneratorsSolidColor` function: enable checkbox with `MoveTransformToEnd`, `ImGui::ColorEdit3` for color, blend intensity slider, blend mode combo
- Add `DrawGeneratorsSolidColor` call in `DrawGeneratorsCategory`

`imgui_effects_generators.h`:
- No changes needed (function signature unchanged)

`imgui_effects.cpp`:
- Add 4 cases to `GetTransformCategory()` returning `{"GEN", 10}` for the 4 generator blend transforms

**Verify**: Compiles.

#### Task 2.6: Preset serialization

**Files**: `src/config/preset.cpp`

**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SolidColorConfig, enabled, color, blendMode, blendIntensity)` macro
- Update existing `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macros for `ConstellationConfig`, `PlasmaConfig`, `InterferenceConfig` to include `blendMode, blendIntensity` fields
- Add `to_json` entry: `if (e.solidColor.enabled) { j["solidColor"] = e.solidColor; }`
- Add `from_json` entry: `e.solidColor = j.value("solidColor", e.solidColor);`
- Existing generator entries already use `j.value()` with defaults, so old presets missing `blendMode`/`blendIntensity` automatically get the defaults (SCREEN / 1.0)

**Verify**: Compiles.

#### Task 2.7: Parameter registration

**Files**: `src/automation/param_registry.cpp`

**Depends on**: Wave 1 complete

**Do**:
- Add `PARAM_TABLE` entries for the new modulatable fields:
  - `constellation.blendIntensity` (0.0-5.0)
  - `plasma.blendIntensity` (0.0-5.0)
  - `interference.blendIntensity` (0.0-5.0)
  - `solidColor.blendIntensity` (0.0-5.0)
  - `solidColor.color` — 3 entries: `solidColor.colorR`, `solidColor.colorG`, `solidColor.colorB` (0.0-1.0 each), using `offsetof(EffectConfig, solidColor.color[0])`, `[1]`, `[2]`

**Verify**: Compiles.

#### Task 2.8: Build system

**Files**: `CMakeLists.txt`

**Depends on**: Wave 1 complete

**Do**:
- Add `src/effects/solid_color.cpp` to `EFFECTS_SOURCES`

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Constellation/Plasma/Interference appear in the transform pipeline list with "GEN" badge
- [ ] Generators can be reordered via drag-drop relative to other transforms
- [ ] Blend mode combo and intensity slider appear in each generator's UI section
- [ ] Default blend (Screen, 1.0) produces visuals nearly identical to the old additive blend
- [ ] Solid Color generator works: flat color composited via selected blend mode
- [ ] Enabling a generator calls `MoveTransformToEnd` (appears at end of pipeline)
- [ ] Preset save/load preserves blend mode + intensity for all generators
- [ ] Old presets without blend fields load with Screen/1.0 defaults (no visual change)
- [ ] Modulation routes to `blendIntensity` and `solidColor.color` params
- [ ] Multiple generators enabled simultaneously render correctly (sequential scratch reuse)

<!-- Intentional deviation: Research doc recommends reusing ping-pong buffers (Option 1). Plan uses a shared scratch texture instead, per user decision during planning — avoids double-swap complexity and matches the simulation TrailMap pattern more closely. -->
<!-- blendIntensity range 0.0-5.0 matches existing simulation boostIntensity ranges; research doc omits an upper bound. -->
