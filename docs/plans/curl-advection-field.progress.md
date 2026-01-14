---
plan: docs/plans/curl-advection-field.md
branch: curl-advection-field
current_phase: 3
total_phases: 4
started: 2026-01-13
last_updated: 2026-01-13
---

# Implementation Progress: Curl-Advection Field

## Phase 1: Core Algorithm
- Status: completed
- Started: 2026-01-13
- Completed: 2026-01-13
- Files modified:
  - src/simulation/curl_advection.h (created)
  - src/simulation/curl_advection.cpp (created)
  - shaders/curl_advection.glsl (created)
- Notes: Created CurlAdvectionConfig struct with all parameters from plan. Implemented CurlAdvection struct with ping-pong state textures (RGBA16F). Compute shader implements 3x3 stencil differential operators (curl, divergence, laplacian, blur), multi-step self-advection loop, field update equation with competing forces, and color output via ColorLUT.

## Phase 2: Integration
- Status: completed
- Started: 2026-01-13
- Completed: 2026-01-13
- Files modified:
  - CMakeLists.txt
  - src/config/effect_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/render_pipeline.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
  - src/ui/imgui_effects.cpp
- Notes: Added CurlAdvectionConfig to EffectConfig, CurlAdvection pointer to PostEffect struct. Integrated Init/Uninit/Resize/Reset calls. Added ApplyCurlAdvectionPass to simulation passes and SetupCurlAdvectionTrailBoost for trail compositing. Created UI section with sliders for all parameters including ModulatableSlider for advectionCurl, curlScale, selfAmp, injectionIntensity.

## Phase 3: Preset & Modulation
- Status: pending

## Phase 4: Polish
- Status: pending
