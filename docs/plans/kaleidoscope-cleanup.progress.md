---
plan: docs/plans/kaleidoscope-cleanup.md
branch: kaleidoscope-cleanup
current_phase: 3
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
- Status: completed
- Started: 2026-01-08
- Completed: 2026-01-08
- Files modified:
  - src/config/infinite_zoom_config.h
  - shaders/infinite_zoom.fs
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.cpp
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
  - src/automation/param_registry.cpp
- Notes: Added drosteIntensity (0-1 blend) and drosteScale (2-256 logarithmic) fields. Shader refactored to extract sampleLayeredZoom() and add sampleDroste() functions. Main blends between modes based on drosteIntensity with early-outs at 0 and 1. UI shows Droste slider with conditional Scale slider when intensity > 0. Modulation entries registered for both fields.

## Phase 3: Create Conformal Warp Effect
- Status: completed
- Started: 2026-01-08
- Completed: 2026-01-08
- Files modified:
  - src/config/conformal_warp_config.h (new)
  - shaders/conformal_warp.fs (new)
  - src/config/effect_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/render_pipeline.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
  - src/automation/param_registry.cpp
- Notes: Created new TRANSFORM_CONFORMAL_WARP effect with Power Map (z^n conformal transformation). Config includes powerMapN (0.5-8.0), rotationSpeed (per-frame accumulation), and Lissajous focal offset. Shader applies complex power transformation with rotation. Full integration: transform order list, UI section, preset serialization, modulation entries.
