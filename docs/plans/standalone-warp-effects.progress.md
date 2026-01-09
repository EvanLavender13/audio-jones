---
plan: docs/plans/standalone-warp-effects.md
branch: standalone-warp-effects
current_phase: 4
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

## Phase 2: Texture Warp (Poincare removed)
- Status: completed
- Started: 2026-01-08
- Completed: 2026-01-08
- Files modified:
  - shaders/texture_warp.fs (created)
  - src/config/texture_warp_config.h (created)
  - src/config/effect_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
  - src/ui/imgui_effects.cpp
  - src/automation/param_registry.cpp
  - src/config/preset.cpp
  - src/config/power_map_config.h (slider fix)
- Notes: Implemented Texture Warp (self-referential RG-offset distortion). Poincare was implemented then removed - the research doc oversold it as "hyperbolic infinite detail" but the simple tanh compression is just a boring radial stretch. Also fixed Power Map slider to use integer increments 1-8.

## Phase 3: Joukowski + Exponential
- Status: completed
- Started: 2026-01-08
- Completed: 2026-01-08
- Files modified:
  - src/config/joukowski_config.h (created)
  - src/config/exp_map_config.h (created)
  - shaders/joukowski.fs (created)
  - shaders/exp_map.fs (created)
  - src/config/effect_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
  - src/render/render_pipeline.cpp
  - src/ui/imgui_effects.cpp
  - src/automation/param_registry.cpp
  - src/config/preset.cpp
- Notes: Implemented Joukowski transform (z + c/z) and Exponential map (exp(z)) as conformal warp effects. Both effects have rotation speed, scale, and Lissajous focal offset parameters. Registered strength and rotationSpeed as modulatable params for both effects.

## Phase 4: Sine Map
- Status: pending
