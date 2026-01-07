---
plan: docs/plans/voronoi-flexible-effects.md
branch: voronoi-flexible-effects
current_phase: 6
total_phases: 6
started: 2026-01-06
last_updated: 2026-01-07
status: complete
---

# Implementation Progress: Voronoi Flexible Effects

## Phase 1: Config and Shader Foundation
- Status: completed
- Started: 2026-01-06
- Completed: 2026-01-06
- Files modified:
  - src/config/voronoi_config.h
  - shaders/voronoi.fs
- Notes: Removed VoronoiMode enum, added 9 intensity floats and isoFrequency param. Shader restructured to compute voronoi data once and apply all effects based on intensity.

## Phase 2: Uniform Locations and Setup
- Status: completed
- Completed: 2026-01-06
- Files modified:
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.cpp
- Notes: Added uniform locations for all 9 intensity params plus isoFrequency. Updated SetupVoronoi to pass all new uniforms.

## Phase 3: UI Sliders
- Status: completed
- Completed: 2026-01-06
- Files modified:
  - src/ui/imgui_effects.cpp
- Notes: Replaced mode combo with 9 intensity sliders. Added isoFrequency slider.

## Phase 4: Param Registry
- Status: completed
- Completed: 2026-01-06
- Files modified:
  - src/automation/param_registry.cpp
  - src/ui/imgui_effects.cpp
- Notes: Added all 9 intensity params + isoFrequency to PARAM_TABLE and targets array. Updated UI to use ModulatableSlider for all new params.

## Phase 5: Preset Serialization
- Status: completed
- Completed: 2026-01-06
- Files modified:
  - src/config/preset.cpp
- Notes: Updated JSON serialization macro with all new fields. Old presets with mode/strength ignored (uses defaults).

## Phase 6: Polish and Defaults
- Status: completed
- Completed: 2026-01-07
- Files modified:
  - shaders/voronoi.fs
  - docs/plans/voronoi-flexible-effects.md
  - docs/research/voronoi-shaders.md
- Notes: Fixed border vector calculation (was storing midpoint instead of projection). Corrected effect formulas to match reference. Scaled up Determinant effect 4x for visibility. Documented algorithm deviations and known issues in plan. Added effect summary and recommended combinations to research doc.
