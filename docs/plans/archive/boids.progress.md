---
plan: docs/plans/boids.md
branch: boids
current_phase: 8
total_phases: 8
started: 2026-01-11
last_updated: 2026-01-11
---

# Implementation Progress: Boids

## Phase 1: Core Infrastructure
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/simulation/boids.h
  - src/simulation/boids.cpp
  - shaders/boids_agents.glsl
- Notes: Created BoidAgent struct (32 bytes), BoidsConfig with all parameters, Boids struct with SSBO/compute/trailMap. Implemented Init/Uninit/Reset/Resize/ApplyConfig/Update. Agent initialization uses ColorConfig hue assignment from Physarum. Shader skeleton moves agents and deposits color without steering behaviors.

## Phase 2: Compute Shader - Steering Behaviors
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - shaders/boids_agents.glsl
- Notes: Implemented three steering rules: cohesion() with hue affinity weighting, separation() without hue weighting, alignment() for velocity matching. Main loop combines weighted forces, clamps velocity to min/max speed, wraps position. Deposit uses HSV->RGB conversion with proportional overflow scaling.

## Phase 3: Texture Reaction
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - shaders/boids_agents.glsl
  - src/simulation/boids.cpp
- Notes: Added feedbackTex sampler at binding 2. Implemented textureReaction() that samples luminance ahead of movement direction and steers toward/away from bright areas based on attractMode. Bound accumTexture in BoidsUpdate().

## Phase 4: PostEffect Integration
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
  - src/render/render_pipeline.cpp
  - CMakeLists.txt
- Notes: Added Boids* to PostEffect struct. Wired Init/Uninit/Resize/Clear lifecycle. Added SetupBoidsTrailBoost() for BlendCompositor. Added ApplyBoidsPass() in RenderPipelineApplyFeedback() after attractor flow. Added trail boost render pass in RenderPipelineApplyOutput(). Config accessed via pe->boids->config since EffectConfig not yet updated.

## Phase 5: Config and Serialization
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/config/effect_config.h
  - src/config/preset.cpp
  - src/render/render_pipeline.cpp
  - src/render/shader_setup.cpp
  - src/render/post_effect.cpp
- Notes: Added BoidsConfig to EffectConfig struct with include. Added NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for JSON serialization. Updated render pipeline to use pe->effects.boids instead of pe->boids->config. Added BoidsApplyConfig call to sync config from EffectConfig to internal state.

## Phase 6: UI Panel
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/ui/imgui_effects.cpp
- Notes: Added sectionBoids static bool and Boids section in SIMULATIONS group after Attractor Flow. Full UI controls for all parameters: enabled, agentCount, perception/separation radius, steering weights (cohesion, separation, alignment, hueAffinity), texture reaction (textureWeight, attractMode, sensorDistance), speed limits, trail settings (deposit, decay, diffusion), boost/blend mode, color config, and debug overlay.

## Phase 7: Modulation
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/automation/param_registry.cpp
  - src/ui/imgui_effects.cpp
- Notes: Added boids.cohesionWeight, boids.separationWeight, boids.alignmentWeight to PARAM_TABLE with 0-2 range. Added corresponding target pointers in ParamRegistryInit. Changed steering weight sliders in UI to ModulatableSlider with param keys.

## Phase 8: Remove Broken Texture Reaction
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - shaders/boids_agents.glsl
  - src/simulation/boids.h
  - src/simulation/boids.cpp
  - src/config/preset.cpp
  - src/ui/imgui_effects.cpp
- Notes: Removed textureReaction() function, feedbackTex sampler, and v4 from velocity calculation in shader. Removed textureWeight, attractMode, sensorDistance from BoidsConfig struct and corresponding uniform locations from Boids struct. Removed feedbackTex binding in BoidsUpdate(). Updated JSON serialization and UI sliders. Boids now relies on three steering rules (separation, alignment, cohesion) plus hue affinity.
