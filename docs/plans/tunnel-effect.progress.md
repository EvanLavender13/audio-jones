---
plan: docs/plans/tunnel-effect.md
branch: tunnel-effect
current_phase: 5
total_phases: 5
started: 2026-01-05
last_updated: 2026-01-05
---

# Implementation Progress: Tunnel Effect

## Phase 1: Config and Shader
- Status: completed
- Completed: 2026-01-05
- Files modified:
  - src/config/tunnel_config.h (new)
  - shaders/tunnel.fs (new)
  - src/config/effect_config.h
- Notes: Created TunnelConfig struct with all parameters. Added TRANSFORM_TUNNEL to enum, TransformEffectName switch, TransformOrderConfig default order, and TunnelConfig member to EffectConfig.

## Phase 2: PostEffect Integration
- Status: completed
- Completed: 2026-01-05
- Files modified:
  - src/render/post_effect.h
  - src/render/post_effect.cpp
- Notes: Added tunnelShader and 10 uniform locations. Added tunnelTime and tunnelFocal state variables. Shader loads in LoadPostEffectShaders, uniforms retrieved in GetShaderUniformLocations, time initialized to 0, shader unloaded in PostEffectUninit.

## Phase 3: Render Pipeline
- Status: completed
- Completed: 2026-01-05
- Files modified:
  - src/render/render_pipeline.cpp
- Notes: Added SetupTunnel forward declaration and implementation setting all 10 uniforms. Added TRANSFORM_TUNNEL case to GetTransformEffect. Added tunnelTime increment in RenderPipelineApplyFeedback. Computed tunnelFocal Lissajous in RenderPipelineApplyOutput.

## Phase 4: UI Panel
- Status: completed
- Completed: 2026-01-05
- Files modified:
  - src/ui/imgui_effects.cpp
- Notes: Added sectionTunnel state, TRANSFORM_TUNNEL case in effect order enabled-check. Added Tunnel section with checkbox, speed slider, modulatable rotation/twist/winding sliders, layers, depth spacing, winding frequencies, focal amplitude/frequencies, and anim speed.

## Phase 5: Serialization and Modulation
- Status: completed
- Completed: 2026-01-05
- Files modified:
  - src/config/preset.cpp
  - src/automation/param_registry.cpp
- Notes: Added NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT macro for TunnelConfig. Added conditional serialization in EffectConfig to_json/from_json. Registered 4 modulatable params: tunnel.speed, tunnel.rotationSpeed (uses ROTATION_SPEED_MAX), tunnel.windingAmplitude, tunnel.twist.

## Phase 6: Tuning - Winding Phase Accumulation
- Status: completed
- Completed: 2026-01-05
- Files modified:
  - shaders/tunnel.fs
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/render_pipeline.cpp
- Notes: Fixed winding frequency behavior. Original implementation used `time` directly in winding calculation, causing instant phase jumps when adjusting frequency sliders. Fix adds `tunnelWindingPhaseX` and `tunnelWindingPhaseY` state variables that accumulate based on frequency values each frame. Shader now uses accumulated phase instead of raw time: `sin(depth * windingFreqX + windingPhaseX)`. Frequency changes now smoothly affect animation speed without visual discontinuities. Winding effect offsets the tunnel center as a function of depth, creating warped distortion pattern.
