# Modulatable Slider Polish

Visual cleanup for the modulatable slider: fix tick/handle alignment, add limit tick mark, fix edge case when modulation exceeds bounds.

## Current State

- `src/ui/modulatable_slider.cpp:96-131` - `DrawModulationTrack()` draws highlight bar and base tick
- `src/ui/modulatable_slider.cpp:318-322` - Base value incorrectly calculated as `*value - offset` (fails when clamped)
- `src/automation/modulation_engine.cpp:12` - Base stored in `ParamMeta.base` but no getter exists
- `src/automation/modulation_engine.h:32` - API declarations

## Phase 1: Add ModEngineGetBase API

**Goal**: Expose stored base value directly instead of back-calculating from offset.

**Build**:
- Add `float ModEngineGetBase(const char* paramId);` declaration to `modulation_engine.h`
- Implement in `modulation_engine.cpp`: lookup `sParams[id]`, return `meta.base` or 0.0f if not found

**Done when**: Can call `ModEngineGetBase("test.param")` and get the stored base value.

---

## Phase 2: Update DrawModulationTrack

**Goal**: Draw both base tick and limit tick with correct positioning.

**Build**:
- Extend `DrawModulationTrack` signature to accept `float limitValue` parameter
- Add limit tick drawing after base tick:
  - Calculate `limitRatio` and `limitXCenter` using same formula as base
  - Draw 2px vertical rect at `limitXCenter` with `SetColorAlpha(sourceColor, 90)`
- Keep base tick at alpha 180 (existing)

**Done when**: Function accepts limit value and draws two distinct tick marks.

---

## Phase 3: Fix Call Site

**Goal**: Use correct base value and calculate limit for DrawModulationTrack.

**Build**:
- Replace `modulatable_slider.cpp:319-320`:
  ```
  const float offset = ModEngineGetOffset(paramId);
  const float baseValue = (*value - offset) * displayScale;
  ```
  With:
  ```
  const float baseValue = ModEngineGetBase(paramId) * displayScale;
  ```
- Calculate limit value:
  ```
  const float range = displayMax - displayMin;
  const float limitValue = std::clamp(baseValue + route.amount * range, displayMin, displayMax);
  ```
- Pass `limitValue` to `DrawModulationTrack`

**Done when**: Base tick stays fixed when modulation hits bounds; limit tick shows furthest modulation point.

---

## Phase 4: Visual Verification

**Goal**: Confirm all edge cases render correctly.

**Build**:
- Test with LFO source (predictable sweep)
- Verify: base at max + positive amount → limit tick at max boundary
- Verify: base at min + negative amount → limit tick at min boundary
- Verify: mid-range base → both ticks visible, correctly positioned
- Verify: handle and base tick align when no modulation active

**Done when**: All edge cases display correctly; no visual artifacts.
