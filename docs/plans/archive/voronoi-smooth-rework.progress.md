---
plan: docs/plans/voronoi-smooth-rework.md
branch: voronoi-smooth-rework
current_phase: 6
total_phases: 6
started: 2026-01-14
last_updated: 2026-01-14
status: complete
---

# Implementation Progress: Voronoi Smooth Rework

## Phase 1: Shader Core
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - shaders/voronoi.fs
- Notes: Added `uniform bool smoothMode`, smooth Voronoi algorithm using inverse distance power accumulation.

## Phase 2: Effect Swap
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - shaders/voronoi.fs
- Notes: Replaced `edgeDarkenIntensity` and `angleShadeIntensity` with `organicFlowIntensity` and `edgeGlowIntensity`.

## Phase 3: Fix Transparency
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - shaders/voronoi.fs
- Notes: Changed Edge Iso, Center Iso, and Edge Detect from additive to mix-blend.

## Phase 4: Config and Pipeline
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - src/config/voronoi_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.cpp
  - src/automation/param_registry.cpp
  - src/ui/imgui_effects_transforms.cpp
  - src/config/preset.cpp
- Notes: Added `smoothMode` bool (renamed from `smooth` due to GLSL reserved keyword). Updated uniform bindings and UI checkbox.

## Phase 5: UI and Serialization
- Status: completed
- Completed: 2026-01-14
- Notes: Merged into Phase 4. UI button labels changed to "Organic"/"Glow". Added Smooth checkbox.

## Phase 6: Preset Migration and Docs
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - presets/GRAYBOB.json
  - presets/BOIDIUS.json
  - presets/WINNY.json
  - presets/STAYINNIT.json
  - presets/SOLO.json
- Notes: Updated 5 preset files with new field names. Docs unchanged (description still accurate).

## Post-Implementation Fixes
- Renamed `smooth` to `smoothMode` (GLSL reserved keyword)
- Fixed smooth mode to use separate vectors: `smoothMr` for displacement, `mr` for border calc
- Added `fwidth()` anti-aliasing for iso effects in smooth mode
