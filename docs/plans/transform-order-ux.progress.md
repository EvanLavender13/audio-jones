---
plan: docs/plans/transform-order-ux.md
branch: transform-order-ux
current_phase: 6
total_phases: 6
started: 2026-01-11
last_updated: 2026-01-11
---

# Implementation Progress: Transform Order List UX Improvement

## Phase 1: Add Helper Function
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/config/effect_config.h
- Notes: Added `IsTransformEnabled()` function with switch on all 21 transform types. Added `TransformOrderConfig::MoveToEnd()` method to shift effect to end of order array.

## Phase 2: Filtered Pipeline List
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/ui/imgui_effects.cpp
  - src/ui/theme.h
- Notes: Replaced listbox with 120px filtered list. Shows only enabled effects with drag handle (::), effect name, and colored category badge. Used full-width Selectable with AllowOverlap for proper highlight. Added `GetSectionAccent()` to theme.h.

## Phase 3: Drag-Drop Reordering
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/ui/imgui_effects.cpp
- Notes: Added BeginDragDropSource/Target on each row. Payload carries array index. On drop, inserts at target position and shifts other items.

## Phase 4: Hook Enable Checkboxes
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/ui/imgui_effects_transforms.cpp
- Notes: All 21 effect enable checkboxes now call MoveToEnd() when toggled on. Newly enabled effects appear at end of pipeline.

## Phase 5: Fix Section Header Colors
- Status: skipped
- Notes: Already implemented correctly. Category headers use alternating colors, subsections inherit parent color.

## Phase 6: Polish
- Status: pending
