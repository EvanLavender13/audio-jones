---
plan: docs/plans/attractor-curl-flow-optimization.md
branch: attractor-curl-flow-optimization
current_phase: done
total_phases: 4
started: 2026-01-02
last_updated: 2026-01-02
---

# Implementation Progress: Attractor & Curl Flow Shader Optimization

## Phase 1: Attractor Rotation Precomputation
- Status: completed
- Completed: 2026-01-02
- Files modified:
  - src/simulation/attractor_flow.h
  - src/simulation/attractor_flow.cpp
  - shaders/attractor_agents.glsl
- Notes: Replaced vec3 rotation uniform with mat3 rotationMatrix. Compute rotation matrix on CPU with 6 trig ops once per frame instead of per agent. Deleted shader rotationMatrix() function.

## Phase 2: NoiseTexture3D Module
- Status: FAILED AND REVERTED
- Files deleted:
  - src/simulation/noise_texture3d.h
  - src/simulation/noise_texture3d.cpp
- Notes: 3D tiling texture cannot replace procedural noise. With noiseFrequency=0.005, texture repeats ~10 times across screen with only ~25 texels per tile, causing visible tiling. A screen-sized 2D texture would compute MORE noise ops (2M pixels) than per-agent (100K agents). Procedural noise is the only viable approach.

## Phase 3: Curl Flow Gradient Pass
- Status: completed
- Completed: 2026-01-02
- Files modified:
  - shaders/curl_gradient.glsl
  - src/simulation/curl_flow.h
  - src/simulation/curl_flow.cpp
- Notes: Created compute shader for gradient precomputation. Samples density at +/- gradientRadius, computes central differences, writes to RG16F texture. Dispatched when trailInfluence >= 0.001f. Gradient texture recreated on resize. This optimization WORKS - reduces 5 samples per agent to 1.

## Phase 4: Curl Flow Shader Integration
- Status: FAILED AND REVERTED
- Notes: Attempted to integrate 3D noise texture. Caused visible tiling artifacts. Reverted to procedural simplex noise in shader. Gradient texture integration remains and works correctly.

## Final Result
- Rotation precomputation: WORKS
- Gradient texture: WORKS
- Noise texture: FAILED, procedural noise restored
- Files deleted: noise_texture3d.h, noise_texture3d.cpp
- CMakeLists.txt: noise_texture3d.cpp removed
