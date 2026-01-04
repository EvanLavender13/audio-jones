# Rotation Standardization

Standardize all rotation/spin parameters: bidirectional (±), consistent max speed (45°/frame), all modulatable.

## Current State

Rotation params scattered with inconsistent bounds and some missing from PARAM_TABLE:

- `src/automation/param_registry.cpp:20-21` - flowField.rotBase/rotRadial at ±0.01 rad
- `src/automation/param_registry.cpp:30-33` - kaleidoscope/attractor rotation speeds at ±0.1 rad
- `src/automation/drawable_params.cpp:19-20` - drawable.*.rotationSpeed registered at ±0.05 rad but NOT in PARAM_TABLE (bug: slider uses 0-1 rad default)
- `src/ui/drawable_type_controls.cpp:18-19` - drawable rotation/offset sliders
- `src/ui/imgui_effects.cpp:176-184` - attractor rotation offset sliders (non-modulatable)

## Phase 1: Define Rotation Constants

**Goal**: Single source of truth for rotation bounds.

**Modify**:
- `src/ui/ui_units.h` - Add `ROTATION_SPEED_MAX` (0.785398f = 45°/frame) and `ROTATION_OFFSET_MAX` (PI)

**Done when**: Constants defined and included where needed.

---

## Phase 2: Fix Drawable Param Bounds

**Goal**: Remove broken fallback, define drawable field bounds properly.

**Modify**:
- `src/automation/param_registry.cpp`:
  - Add drawable field definitions to PARAM_TABLE (or a dedicated DRAWABLE_FIELD_TABLE):
    - `drawable.*.x` → 0.0 to 1.0
    - `drawable.*.y` → 0.0 to 1.0
    - `drawable.*.rotationSpeed` → ±ROTATION_SPEED_MAX
    - `drawable.*.rotationOffset` → ±ROTATION_OFFSET_MAX
  - Update `ParamRegistryGetDynamic` to match `drawable.<id>.<field>` against these patterns
  - Remove the blind fallback that accepts any drawable.* with caller defaults

**Done when**: Drawable sliders use defined bounds; undefined drawable fields fail loudly.

---

## Phase 3: Standardize Static PARAM_TABLE

**Goal**: All rotation speeds use ±0.785 rad (45°/frame).

**Modify**:
- `src/automation/param_registry.cpp` - Update bounds for:
  - `flowField.rotBase` → ±0.785 (was ±0.01)
  - `flowField.rotRadial` → ±0.785 (was ±0.01)
  - `kaleidoscope.rotationSpeed` → ±0.785 (was ±0.1)
  - `attractorFlow.rotationSpeedX/Y/Z` → ±0.785 (was ±0.1)

**Done when**: All rotation speed sliders allow ±45°/frame.

---

## Phase 4: Add Rotation Offset Params

**Goal**: Make rotation offsets modulatable.

**Modify**:
- `src/automation/param_registry.cpp` - Add to PARAM_TABLE:
  - `attractorFlow.rotationX` → ±PI
  - `attractorFlow.rotationY` → ±PI
  - `attractorFlow.rotationZ` → ±PI
- `src/automation/drawable_params.cpp` - Register `rotationOffset` in `DrawableParamsRegister()`
  (bounds come from DRAWABLE_FIELD_TABLE added in Phase 2)

**Done when**: Offset params registered and queryable.

---

## Phase 5: Update UI to Modulatable Sliders

**Goal**: All rotation controls use modulatable slider variants.

**Modify**:
- `src/ui/drawable_type_controls.cpp:19` - Change `SliderAngleDeg("Offset", ...)` to `ModulatableDrawableSliderAngleDeg`
- `src/ui/imgui_effects.cpp:176-178` - Change attractor `SliderAngleDeg("Rot X/Y/Z", ...)` to `ModulatableSliderAngleDeg`
- `src/ui/imgui_effects.cpp:107` - Check shape `texAngle` - make modulatable if rotation-related

**Done when**: All rotation sliders show modulation indicator diamond.
