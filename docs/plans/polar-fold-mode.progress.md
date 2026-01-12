---
plan: docs/plans/polar-fold-mode.md
branch: polar-fold-mode
current_phase: 5
total_phases: 6
started: 2026-01-12
last_updated: 2026-01-12
---

# Implementation Progress: Polar Fold Mode

## Phase 1: Config Layer
- Status: completed
- Started: 2026-01-12
- Completed: 2026-01-12
- Files modified:
  - src/config/kifs_config.h
  - src/config/sine_warp_config.h
- Notes: Added `polarFold` (bool) and `polarFoldSegments` (int, default 6) fields to both KifsConfig and SineWarpConfig structs.

## Phase 2: Shader Updates
- Status: completed
- Started: 2026-01-12
- Completed: 2026-01-12
- Files modified:
  - shaders/kifs.fs
  - shaders/sine_warp.fs
- Notes: Added `polarFold` and `polarFoldSegments` uniforms plus `doPolarFold()` helper function to both shaders. Polar fold inserted after initial rotation (KIFS) and after centering (Sine Warp), before iteration loops.

## Phase 3: Render Integration
- Status: completed
- Started: 2026-01-12
- Completed: 2026-01-12
- Files modified:
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.cpp
- Notes: Added uniform location fields (`kifsPolarFoldLoc`, `kifsPolarFoldSegmentsLoc`, `sineWarpPolarFoldLoc`, `sineWarpPolarFoldSegmentsLoc`) to PostEffect struct. Cached locations in `GetShaderUniformLocations()`. Added `SetShaderValue()` calls in `SetupKifs()` and `SetupSineWarp()` to send config values to shaders.

## Phase 4: UI
- Status: completed
- Started: 2026-01-12
- Completed: 2026-01-12
- Files modified:
  - src/ui/imgui_effects_transforms.cpp
- Notes: Added "Polar Fold" checkbox and "Segments" slider (2-12) to both KIFS and Sine Warp sections. Segments slider only appears when polar fold is enabled.

## Phase 5: Serialization & Registration
- Status: pending

## Phase 6: Verification
- Status: pending
