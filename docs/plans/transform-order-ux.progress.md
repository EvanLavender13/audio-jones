---
plan: docs/plans/transform-order-ux.md
branch: transform-order-ux
current_phase: 3
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
- Notes: Replaced 80px listbox with 120px BeginChild. Shows only enabled effects. Added drag handle (≡), effect name, and category badge (SYM/WARP/CELL/MOT/STY). Added `GetSectionAccent()` to theme.h for consistent color cycling. Removed Up/Down buttons.

## Phase 3: Drag-Drop Reordering
- Status: pending

## Phase 4: Hook Enable Checkboxes
- Status: pending

## Phase 5: Fix Section Header Colors
- Status: skipped
- Notes: Already implemented correctly. Category headers use alternating colors, subsections inherit parent color.

## Phase 6: Polish
- Status: pending
