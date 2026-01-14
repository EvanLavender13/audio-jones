---
plan: docs/plans/voronoi-smooth-rework.md
branch: voronoi-smooth-rework
current_phase: 6
total_phases: 6
started: 2026-01-14
last_updated: 2026-01-14
---

# Implementation Progress: Voronoi Smooth Rework

## Phase 1: Shader Core
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - shaders/voronoi.fs
- Notes: Added `uniform bool smooth`, smooth Voronoi algorithm using inverse distance power accumulation, gradient-based anti-aliased contours for Edge Iso and Center Iso effects.

## Phase 2: Effect Swap
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - shaders/voronoi.fs
- Notes: Replaced `edgeDarkenIntensity` and `angleShadeIntensity` with `organicFlowIntensity` and `edgeGlowIntensity`. Organic Flow applies smooth displacement with center-falloff. Edge Glow adds brightness falloff from edges toward centers.

## Phase 3: Fix Transparency
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - shaders/voronoi.fs
- Notes: Changed Edge Iso, Center Iso, and Edge Detect from additive (`color +=`) to mix-blend (`color = mix(color, cellColor, mask * intensity)`). Effects now show solid voronoi pattern instead of transparent overlay.

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
- Notes: Added `smooth` bool and replaced `edgeDarkenIntensity`/`angleShadeIntensity` with `organicFlowIntensity`/`edgeGlowIntensity` in config, pipeline, param registry, UI, and serialization. Had to include UI and serialization changes (from Phase 5) to maintain build integrity.

## Phase 5: UI and Serialization
- Status: completed
- Completed: 2026-01-14
- Notes: Merged into Phase 4 to maintain build integrity. UI button labels changed from "Darken"/"Angle" to "Organic"/"Glow". Preset serialization updated to include `smooth` field.

## Phase 6: Preset Migration and Docs
- Status: completed
- Completed: 2026-01-14
- Files modified:
  - presets/GRAYBOB.json
  - presets/BOIDIUS.json
  - presets/WINNY.json
  - presets/STAYINNIT.json
  - presets/SOLO.json
- Notes: Updated 5 preset files with old voronoi field names. Replaced angleShadeIntensity/edgeDarkenIntensity with organicFlowIntensity/edgeGlowIntensity, added smooth field. Docs/effects.md description still accurate - no changes needed.
