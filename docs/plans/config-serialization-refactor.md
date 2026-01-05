# Config Serialization Refactor

Wrap `EffectConfig::transformOrder` in a dedicated struct so `EffectConfig` can use `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT`. Reduces maintenance from editing two serialization functions to updating one macro field list.

## Current State

- `src/config/effect_config.h:90` - `transformOrder[TRANSFORM_EFFECT_COUNT]` raw array in `EffectConfig`
- `src/config/preset.cpp:108-160` - Manual `to_json`/`from_json` for `EffectConfig` (53 lines)
- `src/render/render_pipeline.cpp:560` - Accesses `transformOrder[i]`
- `src/ui/imgui_effects.cpp:44-68` - Accesses `transformOrder[i]` for reordering UI

## Phase 1: Create TransformOrderConfig Wrapper

**Goal**: Define wrapper struct with `operator[]` to preserve existing access patterns.

**Build**:
- Add `TransformOrderConfig` struct in `effect_config.h` before `EffectConfig`
- Contains `TransformEffectType order[TRANSFORM_EFFECT_COUNT]` with current default values
- Add `operator[](int i)` and `const operator[](int i)` returning `order[i]`
- Change `EffectConfig::transformOrder` type from raw array to `TransformOrderConfig`

**Done when**: Project compiles. `render_pipeline.cpp` and `imgui_effects.cpp` unchanged, still access `transformOrder[i]`.

---

## Phase 2: Add Serialization and Convert to Macro

**Goal**: Replace manual `EffectConfig` serialization with macro.

**Build**:
- Add `to_json(json&, const TransformOrderConfig&)` in `preset.cpp` - serialize as flat int array for backward compatibility
- Add `from_json(const json&, TransformOrderConfig&)` in `preset.cpp` - parse flat int array, clamp values
- Remove manual `to_json`/`from_json` for `EffectConfig` (lines 108-160)
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EffectConfig, halfLife, blurScale, chromaticOffset, feedbackDesaturate, flowField, gamma, clarity, mobius, turbulence, kaleidoscope, voronoi, physarum, curlFlow, attractorFlow, infiniteZoom, radialStreak, multiInversion, transformOrder)`

**Done when**: Load existing preset (e.g., `SOLO.json`), verify `transformOrder` loads correctly. Save preset, verify JSON output matches expected format.
