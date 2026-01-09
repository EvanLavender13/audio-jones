---
plan: docs/plans/standalone-warp-effects.md
branch: standalone-warp-effects
current_phase: 2
total_phases: 4
started: 2026-01-08
last_updated: 2026-01-08
---

# Implementation Progress: Standalone Warp Effects

## Phase 1: Rename Power Map
- Status: completed
- Started: 2026-01-08
- Completed: 2026-01-08
- Files modified:
  - shaders/conformal_warp.fs → shaders/power_map.fs
  - src/config/conformal_warp_config.h → src/config/power_map_config.h
  - src/config/effect_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
  - src/render/render_pipeline.cpp
  - src/ui/imgui_effects.cpp
  - src/automation/param_registry.cpp
  - src/config/preset.cpp
- Notes: Renamed all occurrences of "Conformal Warp" to "Power Map" including files, structs, enums, UI labels, and param IDs. Build verified successful.

## Phase 2: Poincare + Texture Warp
- Status: pending

## Phase 3: Joukowski + Exponential
- Status: pending

## Phase 4: Sine Map
- Status: pending
