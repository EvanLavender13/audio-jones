# Reorderable Trail Boosts

Add trail boost effects (Physarum, Curl Flow, Curl Advection, Attractor Flow, Boids, Cymatics) to the existing transform effect ordering system. Trail boosts become fully interleaved with other transforms.

## Current State

- `src/config/effect_config.h:48-85` - `TransformEffectType` enum (34 effects)
- `src/config/effect_config.h:134-175` - `TransformOrderConfig` with default order array
- `src/render/shader_setup.cpp:14-90` - `GetTransformEffect()` dispatch table
- `src/render/render_pipeline.cpp:340-380` - Hardcoded trail boost if-blocks (to remove)
- `src/render/render_pipeline.cpp:387-398` - Transform effect loop (trail boosts will join this)
- `src/config/preset.cpp:216-232` - Serialization saves entire order array (to fix)

## Phase 1: Unify Simulation Defaults

**Goal**: Consistent boost intensity range and blend mode across all simulations.

**Build**:
- In each simulation config header (`physarum.h`, `curl_flow.h`, `curl_advection.h`, `attractor_flow.h`, `boids.h`, `cymatics.h`):
  - Change `blendMode` default from `EFFECT_BLEND_BOOST` to `EFFECT_BLEND_SCREEN`
  - Change `boostIntensity` default to `1.0f`
  - Update `boostIntensity` comment to say `0.0-5.0` range

**Done when**: All 6 simulation configs default to `EFFECT_BLEND_SCREEN` and `boostIntensity = 1.0f`.

---

## Phase 2: Extend Enum and Order Config

**Goal**: Add 6 trail boost entries to existing transform system.

**Build**:
- Add to `TransformEffectType` enum in `effect_config.h` (before `TRANSFORM_EFFECT_COUNT`):
  - `TRANSFORM_PHYSARUM_BOOST`
  - `TRANSFORM_CURL_FLOW_BOOST`
  - `TRANSFORM_CURL_ADVECTION_BOOST`
  - `TRANSFORM_ATTRACTOR_FLOW_BOOST`
  - `TRANSFORM_BOIDS_BOOST`
  - `TRANSFORM_CYMATICS_BOOST`
- Add display names to `TransformEffectName()` switch
- Extend `TransformOrderConfig::order` default initializer with 6 new entries at end

**Done when**: Project compiles, enum has 40 entries.

---

## Phase 3: Fix Transform Order Serialization

**Goal**: Only save enabled effects in preset, not entire 40-entry array.

**Build**:
- Modify `to_json(TransformOrderConfig)` in `preset.cpp`:
  - Only push effects where `IsTransformEnabled(e, type)` is true
  - Requires passing `EffectConfig*` to the serializer (change signature or inline into `EffectConfig` serialization)
- Modify `from_json(TransformOrderConfig)` in `preset.cpp`:
  - Load saved order into a temporary list
  - Build final order: saved effects first (in saved order), then remaining effects in default order
  - This ensures new effects appear at end, and disabled effects maintain default positions when re-enabled

**Done when**: Preset JSON contains only enabled effect indices, loads correctly.

---

## Phase 4: Extend Dispatch Table

**Goal**: `GetTransformEffect()` returns valid entries for trail boosts.

**Build**:
- Add 6 cases to `GetTransformEffect()` switch in `shader_setup.cpp`
- Each returns `{ &pe->blendCompositor->shader, SetupXxxTrailBoost, enabled_ptr }`
- Add 6 `bool` fields to `PostEffect` struct for trail boost active state
- Update these bools at start of `RenderPipelineApplyOutput()`: `pe->physarumBoostActive = (pe->physarum != NULL && pe->effects.physarum.enabled && pe->effects.physarum.boostIntensity > 0.0f)`

**Done when**: `GetTransformEffect(TRANSFORM_PHYSARUM_BOOST)` returns valid entry.

---

## Phase 5: Update IsTransformEnabled

**Goal**: UI correctly shows trail boosts as enabled/disabled.

**Build**:
- Add 6 cases to `IsTransformEnabled()` in `effect_config.h`
- Return `e->physarum.enabled && e->physarum.boostIntensity > 0.0f`

**Done when**: Enabled trail boosts appear in pipeline list UI.

---

## Phase 6: Remove Hardcoded Blocks

**Goal**: Trail boosts execute via the transform loop instead of dedicated if-blocks.

**Build**:
- Delete lines 340-380 in `render_pipeline.cpp` (6 hardcoded trail boost blocks)
- Trail boosts now execute at their position in `transformOrder` array

**Done when**: Trail boosts render correctly via the unified loop.

---

## Phase 7: UI Category

**Goal**: Trail boosts appear in pipeline list with appropriate badge.

**Build**:
- Add trail boost cases to `GetTransformCategory()` in `imgui_effects.cpp`
- Use "SIM" badge or similar

**Done when**: Trail boosts visible in pipeline list, draggable to reorder.

---

## Notes

- Chromatic aberration currently runs after trail boosts. With this change, its position depends on `transformOrder`. If chromatic needs to be reorderable too, add it to the enum in a separate task.
- Old presets will load with trail boosts at end of order (default positions for new enum values).
