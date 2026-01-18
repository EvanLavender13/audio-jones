---
plan: docs/plans/purposeful-terrain-warping.md
branch: purposeful-terrain-warping
current_phase: 3
total_phases: 3
started: 2026-01-18
last_updated: 2026-01-18
status: complete
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
- Status: completed
- Started: 2026-01-18
- Completed: 2026-01-18
- Files modified:
  - src/config/domain_warp_config.h (new file: enabled, strength, octaves, lacunarity, persistence, scale, driftSpeed)
  - shaders/domain_warp.fs (new file: fbm-based UV displacement with animated drift)
  - src/config/effect_config.h (added include, enum value, TransformOrderConfig entry, EffectConfig field, IsTransformEnabled case)
  - src/render/post_effect.h (added shader, 6 uniform locations, domainWarpDrift accumulator)
  - src/render/post_effect.cpp (load shader, get uniform locations, unload shader)
  - src/render/render_pipeline.cpp (accumulate driftSpeed per frame)
  - src/render/shader_setup.h (added SetupDomainWarp declaration)
  - src/render/shader_setup.cpp (added GetTransformEffect case, SetupDomainWarp implementation)
  - src/ui/imgui_effects.cpp (added TRANSFORM_DOMAIN_WARP to WARP category)
  - src/ui/imgui_effects_transforms.cpp (added sectionDomainWarp, DrawWarpDomainWarp, added to DrawWarpCategory)
  - src/config/preset.cpp (added serialization macro, to_json/from_json entries)
  - src/automation/param_registry.cpp (added strength, driftSpeed with targets)
- Notes: Full Domain Warp effect with fbm-based warping, fractal octave control, CPU-side drift accumulation for smooth animation.

## Phase 3: Validation
- Status: completed
- Started: 2026-01-18
- Completed: 2026-01-18
- Files modified:
  - shaders/domain_warp.fs (fixed drift animation to use circular motion instead of linear)
- Notes: Validated both effects. Fixed Domain Warp drift to use sin/cos circular motion through noise space instead of linear translation, eliminating directional bias.
