---
plan: docs/plans/kaleidoscope-cleanup.md
branch: kaleidoscope-cleanup
current_phase: 2
total_phases: 3
started: 2026-01-08
last_updated: 2026-01-08
---

# Implementation Progress: Kaleidoscope Cleanup

## Phase 1: Remove from Kaleidoscope
- Status: completed
- Started: 2026-01-08
- Completed: 2026-01-08
- Files modified:
  - src/config/kaleidoscope_config.h
  - shaders/kaleidoscope.fs
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.cpp
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
  - src/automation/param_registry.cpp
- Notes: Removed drosteIntensity, drosteScale, powerMapIntensity, powerMapN fields from config. Removed sampleDroste() and samplePowerMap() shader functions. Removed uniform locations, SetShaderValue calls, UI toggles/sliders, serialization fields, and modulation param entries.

## Phase 2: Add Droste to Infinite Zoom
- Status: pending

## Phase 3: Create Conformal Warp Effect
- Status: pending
