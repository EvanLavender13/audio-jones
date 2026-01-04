---
plan: docs/plans/turbulence-cascade.md
branch: turbulence-cascade
current_phase: 5
total_phases: 6
started: 2026-01-04
last_updated: 2026-01-04
---

# Implementation Progress: Turbulence Cascade

## Phase 1: Shader
- Status: completed
- Started: 2026-01-04
- Completed: 2026-01-04
- Files modified:
  - shaders/turbulence.fs
- Notes: Created turbulence cascade shader with depth-weighted accumulation. Uses sine-based coordinate shifts with rotation per octave. Uniforms: time, octaves, strength, animSpeed, rotationPerOctave.

## Phase 2: Config
- Status: completed
- Started: 2026-01-04
- Completed: 2026-01-04
- Files modified:
  - src/config/turbulence_config.h
  - src/config/effect_config.h
- Notes: Created TurbulenceConfig struct with enabled, octaves (1-8), strength (0-2), animSpeed (0-2), rotationPerOctave (0-2π). Added to EffectConfig.

## Phase 3: PostEffect Integration
- Status: completed
- Started: 2026-01-04
- Completed: 2026-01-04
- Files modified:
  - src/render/post_effect.h
  - src/render/post_effect.cpp
- Notes: Added turbulenceShader, uniform location fields, and turbulenceTime animation state. Shader loads in LoadPostEffectShaders, uniforms cached in GetShaderUniformLocations, shader unloaded in PostEffectUninit.

## Phase 4: Render Pipeline
- Status: completed
- Started: 2026-01-04
- Completed: 2026-01-04
- Files modified:
  - src/render/render_pipeline.cpp
- Notes: Added SetupTurbulence function to set shader uniforms. Update turbulenceTime in RenderPipelineApplyFeedback. Added conditional RenderPass after Möbius, before kaleidoscope.

## Phase 5: UI
- Status: pending

## Phase 6: Preset Serialization
- Status: pending
