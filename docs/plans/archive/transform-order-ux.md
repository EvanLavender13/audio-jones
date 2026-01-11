# Transform Order List UX Improvement

Replace the unwieldy 17-item transform order list with a compact drag-drop pipeline showing only enabled effects.

## Current State

- `src/ui/imgui_effects.cpp:196-284` - Transform order list UI with Up/Down buttons
- `src/ui/imgui_effects_transforms.cpp` - Category sections with enable checkboxes
- `src/config/effect_config.h:25-67` - `TransformEffectType` enum and `TransformOrderConfig`

Current issues:
- Shows all 17 effects regardless of enabled state
- Small 80px listbox requires excessive scrolling
- Up/Down buttons require repeated clicks for large moves

## Design Decisions

- **Show only enabled effects** in the pipeline list
- **Drag-drop reordering** replaces Up/Down buttons
- **Append to end** when enabling an effect
- **Fixed 120px height** viewport
- **Category badges** on each row for context
- Keep existing `TransformOrderConfig` data structure (backward compatible)

---

## Phase 1: Add Helper Function

**Goal**: Consolidate the 17-case enabled check into a reusable function.

**Build**:
- Add `IsTransformEnabled(const EffectConfig*, TransformEffectType)` to `effect_config.h`
- Returns bool by switching on type and checking the corresponding config's `enabled` field
- Add `MoveTransformToEnd(TransformOrderConfig*, TransformEffectType)` helper
- Finds the effect in the array, shifts subsequent elements left, places effect at end

**Done when**: Both functions compile and can be called from UI code.

---

## Phase 2: Filtered Pipeline List

**Goal**: Display only enabled effects in a 120px fixed-height list.

**Build**:
- Replace the `ImGui::BeginListBox` section in `imgui_effects.cpp:221-283`
- Iterate `transformOrder` array, skip disabled effects using `IsTransformEnabled()`
- Render each enabled effect as a selectable row with:
  - Drag handle indicator (≡) on left
  - Effect name
  - Category badge on right (colored text: SYM/WARP/CELL/MOT/STY)
- Use `ImGui::BeginChild()` with fixed 120px height instead of `BeginListBox`
- Remove Up/Down buttons entirely

**Done when**: List shows only enabled effects with category badges. Up/Down buttons removed.

---

## Phase 3: Drag-Drop Reordering

**Goal**: Enable drag-drop to reorder effects within the pipeline.

**Build**:
- Add `ImGui::BeginDragDropSource()` on each row's selectable
- Set payload to the array index of the dragged effect
- Add `ImGui::BeginDragDropTarget()` on each row
- On drop: swap positions in `transformOrder` array
- Visual feedback: cyan highlight on drop target row

**Done when**: Dragging an effect row and dropping on another swaps their positions in the pipeline.

---

## Phase 4: Hook Enable Checkboxes

**Goal**: Move newly enabled effects to end of order array.

**Build**:
- In `imgui_effects_transforms.cpp`, after each effect's enable checkbox:
  - Check if checkbox was just toggled ON (was false, now true)
  - If so, call `MoveTransformToEnd(&e->transformOrder, TRANSFORM_XXX)`
- Apply to all 17 effect enable checkboxes across the category functions:
  - `DrawSymmetryCategory`: Kaleidoscope, KIFS, Poincare Disk
  - `DrawWarpCategory`: Sine Warp, Texture Warp, Gradient Flow, Wave Ripple, Mobius
  - `DrawCellularCategory`: Voronoi, Lattice Fold
  - `DrawMotionCategory`: Infinite Zoom, Radial Blur, Droste Zoom
  - `DrawStyleCategory`: Pixelation, Glitch, Toon, Heightfield Relief

**Done when**: Enabling any effect moves it to the end of the visible pipeline.

---

## Phase 5: Fix Section Header Colors

**Status**: SKIP - Already implemented correctly.

Category headers use `GetSectionGlow(index)` for alternating colors. Nested subsections already inherit `categoryGlow` from the parent. No changes needed.

---

## Phase 6: Polish

**Goal**: Visual refinement matching synthwave theme.

**Build**:
- Category badge colors use `GetSectionAccent(sectionIndex)` - derives from section headers automatically (not hard-coded, stays in sync)
- Alternating row backgrounds (subtle white tint on odd rows)
- Drag handle uses `Theme::TEXT_SECONDARY_U32`
- Selected row uses ImGui's built-in Selectable highlight (no custom overlay)
- Drag preview shows effect name floating near cursor
- Empty list shows "No effects enabled" in dimmed text

**Done when**: Pipeline list visually matches the rest of the effects panel.
