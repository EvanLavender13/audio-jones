---
plan: docs/plans/feedback-motion-scale.md
branch: feedback-motion-scale
mode: parallel
current_phase: 5
current_wave: 3
total_phases: 5
total_waves: 3
waves:
  1: [1]
  2: [2, 3, 4]
  3: [5]
started: 2026-01-24
last_updated: 2026-01-24
---

# Implementation Progress: Feedback Motion Scale

## Phase 1: Config + Registration
- Status: completed
- Wave: 1
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - src/config/effect_config.h
  - src/automation/param_registry.cpp
- Notes: Added motionScale field (default 1.0) to EffectConfig and registered for modulation with range [0.01, 1.0]

## Phase 2: Transform Scaling in SetupFeedback
- Status: completed
- Wave: 2
- Completed: 2026-01-24
- Files modified:
  - src/render/shader_setup.cpp
- Notes: Applied motionScale to all feedback transform uniforms. Identity-centered values (zoom, sx, sy) scale deviation from 1.0. Speed/displacement values (rotation, dx, dy, flow strength) multiply directly.

## Phase 3: WarpTime Accumulation Scaling
- Status: completed
- Wave: 2
- Completed: 2026-01-24
- Files modified:
  - src/render/render_pipeline.cpp
- Notes: Scaled warpTime accumulation by motionScale in RenderPipelineApplyFeedback

## Phase 4: Decay Compensation in SetupBlurV
- Status: completed
- Wave: 2
- Completed: 2026-01-24
- Files modified:
  - src/render/shader_setup.cpp
- Notes: Applied decay compensation formula: effectiveHalfLife = halfLife / safeMotionScale. Clamps motionScale to 0.01 minimum to prevent division by zero.

## Phase 5: UI + Serialization
- Status: completed
- Wave: 3
- Completed: 2026-01-24
- Files modified:
  - src/ui/ui_units.h
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
- Notes: Added ModulatableSliderLog helper to ui_units.h. Added Motion slider after Blur in FEEDBACK group. Added motionScale to preset JSON serialization.
