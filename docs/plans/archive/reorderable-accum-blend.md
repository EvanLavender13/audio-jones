# Reorderable Accum Blend

The accumulated feedback+drawable texture becomes a positioned, blend-controlled entry in the reorderable transform pipeline list. The output chain starts from a black canvas, and the accum texture blends in at the user's chosen position with a selectable blend mode and intensity. Default: Screen at 1.0 at position 0, which produces identical visuals to the current pipeline.

**Research**: `docs/research/reorderable-accum-blend.md`

## Design

### Types

**New enum entry** (in `TransformEffectType`, before `TRANSFORM_EFFECT_COUNT`):
```c
TRANSFORM_ACCUM_COMPOSITE,
TRANSFORM_EFFECT_COUNT
```

**New fields in `EffectConfig`** (top-level, alongside `halfLife`, `gamma`, etc.):
```c
EffectBlendMode accumBlendMode = EFFECT_BLEND_SCREEN;
float accumBlendIntensity = 1.0f;
bool accumCompositeEnabled = true; // Always true; never serialized
```

`accumCompositeEnabled` exists solely so `IsDescriptorEnabled()` can read it via `enabledOffset`. It defaults to `true` and is never toggled or serialized, ensuring the accum entry is always visible in the pipeline list.

### Algorithm

#### Output chain dispatch change

`RenderPipelineApplyOutput()` currently starts with:
```c
RenderTexture2D *src = &pe->accumTexture;
int writeIdx = 0;
```

New behavior:
```c
BeginTextureMode(pe->pingPong[0]);
ClearBackground(BLACK);
EndTextureMode();
RenderTexture2D *src = &pe->pingPong[0];
int writeIdx = 1;
```

The accum texture is no longer the implicit base image. Instead, it enters via the `TRANSFORM_ACCUM_COMPOSITE` entry in the dispatch loop, which falls through to the standard single-pass `else` branch (same as sim-boosts):
```c
} else {
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
}
```

The setup function calls `BlendCompositorApply(pe->blendCompositor, pe->accumTexture.texture, accumBlendIntensity, accumBlendMode)`. The blend compositor shader reads `texture0` (the current `src` — black if accum is first) and `effectMap` (the accum texture), compositing via the selected blend mode.

**Backward compatibility**: With Screen at 1.0 from a black canvas: `1 - (1 - black) * (1 - accum) = accum`. This is a mathematically exact passthrough of the accum texture, identical to the current pipeline where `src = &pe->accumTexture`.

#### Descriptor registration

Manual registration via `EffectDescriptorRegister()` (like bloom). The descriptor has:
- `name`: `"Accum Composite"`
- `categoryBadge`: `"TRL"` (Trails)
- `categorySectionIndex`: `-1` (no category section — won't appear in any collapsible section)
- `enabledOffset`: `offsetof(EffectConfig, accumCompositeEnabled)`
- `flags`: `EFFECT_FLAG_NONE`
- `init/uninit/resize`: all `NULL` (no GPU resources to manage)
- `registerParams`: `NULL` (modulation registered via `PARAM_TABLE`)
- `getShader`: returns `&pe->blendCompositor->shader`
- `setup`: `SetupAccumComposite` (calls `BlendCompositorApply`)
- `drawParams`: `NULL` (UI is inline below pipeline list, not in a category section)

#### TransformOrderFromJson prepend logic

When deserializing old presets that don't contain `TRANSFORM_ACCUM_COMPOSITE`, the entry must be **prepended** (not appended) to preserve current visual behavior. After the existing two-pass merge in `TransformOrderFromJson`, add:

```
if accum_composite was not in the saved JSON:
    find its current position in order[]
    if it's not already at index 0:
        shift everything from [0..pos-1] right by 1
        place accum_composite at index 0
```

This ensures old presets get the accum entry first, matching current behavior where `src = &pe->accumTexture` is the starting point.

#### TransformOrderToJson always-save

`TransformOrderToJson` currently only saves enabled effects. Since `accumCompositeEnabled` is always `true`, the accum entry will always be saved. No change needed to the save logic.

#### Pipeline list UI

The accum entry appears in the drag-and-drop pipeline list like any other entry (badge `"TRL"`, section color from index `-1` which cycles via `GetSectionAccent(-1 % 3)`). It participates in drag-and-drop reordering. No enable checkbox (it has no collapsible category section).

Below the pipeline list box, draw the accum blend controls inline:
- `ImGui::SeparatorText("Trails Blend")`
- Blend intensity slider: `ModulatableSlider("Blend Intensity##accumComposite", &e->accumBlendIntensity, "effects.accumBlendIntensity", "%.2f", modSources)`
- Blend mode combo: `ImGui::Combo("Blend Mode##accumComposite", ...)`

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| accumBlendMode | EffectBlendMode | enum | EFFECT_BLEND_SCREEN | No | Blend Mode |
| accumBlendIntensity | float | 0.0-1.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_ACCUM_COMPOSITE`
- Display name: `"Accum Composite"`
- Badge: `"TRL"` (Trails)
- Category section index: `-1` (no section)
- Flags: `EFFECT_FLAG_NONE`

---

## Tasks

### Wave 1: Foundation (types and enum)

#### Task 1.1: Add enum entry and config fields

**Files**: `src/config/effect_config.h`
**Creates**: `TRANSFORM_ACCUM_COMPOSITE` enum value and `accumBlendMode` / `accumBlendIntensity` / `accumCompositeEnabled` fields that all other tasks depend on.

**Do**:
1. Add `TRANSFORM_ACCUM_COMPOSITE` immediately before `TRANSFORM_EFFECT_COUNT` in `TransformEffectType` enum
2. Add to `EffectConfig` struct, in the top-level section after `clarity` and before the first effect config (before the `// Kaleidoscope` comment):
   - `EffectBlendMode accumBlendMode = EFFECT_BLEND_SCREEN;`
   - `float accumBlendIntensity = 1.0f;`
   - `bool accumCompositeEnabled = true;`
3. This requires `#include "render/blend_mode.h"` at the top of `effect_config.h` — check if it's already included, add if not.
4. Modify `TransformOrderConfig` constructor to place `TRANSFORM_ACCUM_COMPOSITE` at index 0. After the existing identity fill loop (`order[i] = (TransformEffectType)i`), find `TRANSFORM_ACCUM_COMPOSITE` in the array, shift everything before it right by one, and place it at index 0. This ensures fresh configs (no preset loaded) have the accum entry first, matching current visual behavior. Use the same shift pattern as `MoveTransformToEnd` but in reverse — a `MoveTransformToFront` operation.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: Parallel implementation

#### Task 2.1: Serialization and preset compat

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `to_json(json &j, const EffectConfig &e)`: add two lines after the existing top-level fields (after `j["clarity"] = e.clarity;`):
   - `j["accumBlendMode"] = (int)e.accumBlendMode;`
   - `j["accumBlendIntensity"] = e.accumBlendIntensity;`
2. In `from_json(const json &j, EffectConfig &e)`: add two lines after the existing top-level fields (after `e.clarity = j.value(...)`):
   - `e.accumBlendMode = (EffectBlendMode)j.value("accumBlendMode", (int)e.accumBlendMode);`
   - `e.accumBlendIntensity = j.value("accumBlendIntensity", e.accumBlendIntensity);`
3. In `TransformOrderFromJson`: after the existing two-pass merge (after the second `for` loop), add prepend logic for `TRANSFORM_ACCUM_COMPOSITE`. Check if it was present in the saved JSON by inspecting the `seen[]` array at index `TRANSFORM_ACCUM_COMPOSITE`. If not seen, find its current position in `t.order[]`, and if not already at index 0, shift elements right and insert it at position 0.

**Verify**: `cmake.exe --build build` compiles. Load an existing preset file — the accum entry should appear first in the deserialized transform order.

---

#### Task 2.2: Descriptor registration and setup function

**Files**: `src/render/shader_setup.cpp`, `src/render/shader_setup.h`
**Depends on**: Wave 1 complete

**Do**:
1. In `shader_setup.cpp`, add a new non-static setup function `SetupAccumComposite(PostEffect *pe)` that calls:
   ```
   BlendCompositorApply(pe->blendCompositor, pe->accumTexture.texture,
                        pe->effects.accumBlendIntensity, pe->effects.accumBlendMode);
   ```
2. After the function, add manual descriptor registration (follow the bloom pattern in `bloom.cpp` lines 143-152). Use `static bool reg_accumComposite = EffectDescriptorRegister(...)` with:
   - type: `TRANSFORM_ACCUM_COMPOSITE`
   - name: `"Accum Composite"`
   - badge: `"TRL"`
   - section: `-1`
   - enabledOffset: `offsetof(EffectConfig, accumCompositeEnabled)`
   - flags: `EFFECT_FLAG_NONE`
   - init/uninit/resize/registerParams: all `NULL`
   - getShader: static helper returning `&pe->blendCompositor->shader`
   - setup: `SetupAccumComposite`
   - All remaining pointers: `nullptr`
3. In `shader_setup.h`, add declaration: `void SetupAccumComposite(PostEffect *pe);`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Render pipeline dispatch change

**Files**: `src/render/render_pipeline.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `RenderPipelineApplyOutput()`, replace the current starting point:
   ```c
   RenderTexture2D *src = &pe->accumTexture;
   int writeIdx = 0;
   ```
   With:
   ```c
   BeginTextureMode(pe->pingPong[0]);
   ClearBackground(BLACK);
   EndTextureMode();
   RenderTexture2D *src = &pe->pingPong[0];
   int writeIdx = 1;
   ```
2. No other changes needed — the accum composite entry dispatches through the existing `else` branch (standard single-pass) via its registered descriptor.

**Verify**: `cmake.exe --build build` compiles. Run the app — visuals should be identical to before (accum composite is first in default order, Screen at 1.0 from black = exact passthrough).

---

#### Task 2.4: Param registry modulation

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. Add one entry to `PARAM_TABLE[]`:
   ```c
   {"effects.accumBlendIntensity", {0.0f, 1.0f}, offsetof(EffectConfig, accumBlendIntensity)},
   ```
   Place it near the other top-level entries (after `effects.motionScale`).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Pipeline list UI — inline blend controls

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. After the `ImGui::EndListBox()` call (end of the pipeline list), add the accum blend controls section:
   - `ImGui::SeparatorText("Trails Blend");`
   - Blend intensity slider: `ModulatableSlider("Blend Intensity##accumComposite", &e->accumBlendIntensity, "effects.accumBlendIntensity", "%.2f", modSources);`
   - Blend mode combo: standard `ImGui::Combo("Blend Mode##accumComposite", &blendModeInt, BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)` pattern with cast to/from `EffectBlendMode`.
2. Ensure necessary includes are present: `"render/blend_mode.h"`, `"ui/modulatable_slider.h"`, `"automation/mod_sources.h"`. Check what's already included in the file and add any missing ones.
3. The function signature needs access to `EffectConfig*` and `const ModSources*`. Check if `ImGuiDrawEffectsPanel()` already receives these — it likely receives `EffectConfig*` (as `e`) and `ModSources*` (as `modSources`). Use whatever parameters are already available.

**Verify**: `cmake.exe --build build` compiles. Run the app — "Trails Blend" section appears below the pipeline list with a slider and combo box.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Default visuals are identical to before (accum composite first, Screen at 1.0)
- [ ] Accum entry appears in pipeline list with "TRL" badge
- [ ] Drag-and-drop reordering works — moving accum entry changes visual output
- [ ] Blend mode combo and intensity slider appear below pipeline list
- [ ] Changing blend mode/intensity produces expected visual changes
- [ ] Old presets load correctly (accum entry prepended at position 0)
- [ ] New presets save and reload the accum entry position and blend settings
- [ ] accumBlendIntensity shows up in modulation engine parameter list
