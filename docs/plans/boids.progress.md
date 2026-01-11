---
plan: docs/plans/boids.md
branch: boids
current_phase: 4
total_phases: 7
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
- Status: pending

## Phase 5: Config and Serialization
- Status: pending

## Phase 6: UI Panel
- Status: pending

## Phase 7: Modulation
- Status: pending
