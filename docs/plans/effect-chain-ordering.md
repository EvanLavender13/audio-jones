# Effect Chain Ordering

Configurable execution order for transformation effects (Mobius, Turbulence, Kaleidoscope, Infinite Zoom) via UI reorder controls. Saves to presets with backward compatibility.

## Current State

Effects execute in hardcoded order within `RenderPipelineApplyOutput`:
- `src/render/render_pipeline.cpp:485-507` - Sequential if-blocks for each effect
- `src/config/effect_config.h:24-56` - `EffectConfig` aggregates all effect configs
- `src/config/preset.cpp:104-106` - Serialization via nlohmann macro
- `src/ui/imgui_drawables.cpp:115-139` - Existing Up/Down reorder pattern to reuse

## Phase 1: Data Structures

**Goal**: Define enum and order array without changing behavior.

**Build**:
- Add `TransformEffectType` enum to `src/config/effect_config.h` (before `EffectConfig` struct):
  - `TRANSFORM_MOBIUS = 0`
  - `TRANSFORM_TURBULENCE`
  - `TRANSFORM_KALEIDOSCOPE`
  - `TRANSFORM_INFINITE_ZOOM`
  - `TRANSFORM_EFFECT_COUNT`
- Add `transformOrder[TRANSFORM_EFFECT_COUNT]` array to `EffectConfig` with default: `{TRANSFORM_MOBIUS, TRANSFORM_TURBULENCE, TRANSFORM_KALEIDOSCOPE, TRANSFORM_INFINITE_ZOOM}`
- Add `TransformEffectName()` helper function returning display strings ("Mobius", "Turbulence", "Kaleidoscope", "Infinite Zoom")

**Done when**: Project compiles, existing behavior unchanged.

---

## Phase 2: Render Dispatch

**Goal**: Replace hardcoded effect sequence with configurable loop.

**Build**:
- Add `TransformEffectEntry` struct in `render_pipeline.cpp` containing shader pointer, setup function, enabled flag pointer
- Add `GetTransformEffect(PostEffect* pe, TransformEffectType type)` function with switch returning entry for each effect type
- Replace lines 485-507 with loop iterating `pe->effects.transformOrder[i]`, calling `GetTransformEffect()` and `RenderPass()` for enabled effects
- Preserve ping-pong buffer swap pattern

**Done when**: Effects render in default order, visual output identical to before.

---

## Phase 3: Preset Serialization

**Goal**: Save/load effect order with backward compatibility.

**Build**:
- Add `transformOrder` to `EffectConfig` serialization in `preset.cpp`
- Serialize as int array `[0, 1, 2, 3]`
- Use `j.value()` with default array for backward compatibility when loading old presets
- Validate loaded values: clamp to valid enum range

**Done when**: Save preset with custom order, reload, order persists. Load old preset without `transformOrder`, default order applies.

---

## Phase 4: UI

**Goal**: Add effect chain reorder section to Effects panel.

**Build**:
- Add "Effect Order" section at top of Effects panel in `imgui_effects.cpp`, before existing sections
- Use `DrawSectionBegin`/`DrawSectionEnd` with `Theme::GLOW_CYAN` accent
- Add `ImGui::BeginListBox` showing 4 effects using `TransformEffectName()`
- Dim disabled effects using `ImGuiCol_TextDisabled` (check each effect's `.enabled` flag)
- Add Up/Down buttons following `imgui_drawables.cpp:115-139` pattern:
  - `canMoveUp` = selected > 0
  - `canMoveDown` = selected < count - 1
  - Swap via temp variable, update selection index
- Add static `selectedTransformEffect` for selection tracking

**Done when**: Reorder via UI, effects execute in new order immediately, preset save/load preserves order.

---

## Notes

**Extensibility**: Add future effects (trail boosts) by extending enum and switch. Keep trail boost effects separate initially since they have different dispatch pattern (check simulation pointer + enabled + boost intensity > 0).

**Error handling**: Invalid enum values from malformed presets clamp to valid range. Duplicate effects in array not prevented but harmless (second instance skipped since already rendered).
