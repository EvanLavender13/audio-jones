# Config Serialization Refactor

Wrap `EffectConfig::transformOrder` in a dedicated struct so it can use nlohmann macros. Keep custom `EffectConfig` serialization to skip disabled effects, reducing preset file size from ~400 lines to ~100.

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

## Phase 2: Add TransformOrderConfig Serialization

**Goal**: Enable macro-based serialization for `TransformOrderConfig`.

**Build**:
- Add `to_json(json&, const TransformOrderConfig&)` in `preset.cpp` - serialize as flat int array for backward compatibility
- Add `from_json(const json&, TransformOrderConfig&)` in `preset.cpp` - parse flat int array, clamp values

**Done when**: `TransformOrderConfig` serializes to/from flat int array.

---

## Phase 3: Conditional Effect Serialization

**Goal**: Skip disabled effects in JSON output to reduce file size.

**Build**:
- Keep custom `to_json` for `EffectConfig` - only serialize sub-configs when `enabled == true`
- Keep custom `from_json` for `EffectConfig` - use `j.value()` for defaults on missing fields
- Sub-config macros (`MobiusConfig`, `VoronoiConfig`, etc.) handle their own field serialization

**Done when**: Save preset with 1 enabled effect, verify JSON excludes disabled effect configs. Load preset, verify disabled effects use defaults.
