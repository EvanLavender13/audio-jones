---
plan: docs/plans/purposeful-terrain-warping.md
branch: purposeful-terrain-warping
current_phase: 2
total_phases: 3
started: 2026-01-18
last_updated: 2026-01-18
---

# Implementation Progress: Purposeful Terrain Warping

## Phase 1: Directional Texture Warp Enhancement
- Status: completed
- Started: 2026-01-18
- Completed: 2026-01-18
- Files modified:
  - src/config/texture_warp_config.h (added ridgeAngle, anisotropy, noiseAmount, noiseScale fields)
  - shaders/texture_warp.fs (added uniforms, hash/noise/fbm functions, directional bias and noise injection)
  - src/render/post_effect.h (added 4 uniform locations)
  - src/render/post_effect.cpp (get uniform locations)
  - src/render/shader_setup.cpp (pass new uniforms in SetupTextureWarp)
  - src/ui/imgui_effects_transforms.cpp (added Directional and Noise tree nodes with sliders)
  - src/config/preset.cpp (added new fields to serialization macro)
  - src/automation/param_registry.cpp (added ridgeAngle, anisotropy, noiseAmount with targets)
- Notes: Added directional anisotropy to suppress perpendicular warp component based on ridgeAngle. Added fbm-based procedural noise injection. All new parameters registered for modulation.

## Phase 2: Domain Warp Effect
- Status: pending

## Phase 3: Validation
- Status: pending
