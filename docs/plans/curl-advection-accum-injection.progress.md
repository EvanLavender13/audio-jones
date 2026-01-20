---
plan: docs/plans/curl-advection-accum-injection.md
branch: curl-advection-accum-injection
current_phase: 6
total_phases: 6
started: 2026-01-19
last_updated: 2026-01-19
---

# Implementation Progress: Curl Advection Accumulation-Based Injection

## Phase 1: Config and Struct Updates
- Status: completed
- Started: 2026-01-19
- Completed: 2026-01-19
- Files modified:
  - src/simulation/curl_advection.h
- Notes: Removed injectionAmplitude, injectionFreqX, injectionFreqY from config. Added injectionThreshold. Removed injectionCenterLoc and injectionTime from struct. Added injectionThresholdLoc.

## Phase 2: Shader Update
- Status: completed
- Started: 2026-01-19
- Completed: 2026-01-19
- Files modified:
  - shaders/curl_advection.glsl
- Notes: Added accumTexture sampler at binding 4. Replaced injectionCenter uniform with injectionThreshold. Replaced single-point Lissajous injection with distributed accum-based injection using luminance threshold.

## Phase 3: C++ Update Logic
- Status: completed
- Started: 2026-01-19
- Completed: 2026-01-19
- Files modified:
  - src/simulation/curl_advection.cpp
  - src/simulation/curl_advection.h
- Notes: Updated CurlAdvectionUpdate signature to accept accumTexture. Removed Lissajous time accumulation and center calculation. Added injectionThreshold uniform. Bind accumTexture to texture unit 4.

## Phase 4: Pipeline Integration
- Status: completed
- Started: 2026-01-19
- Completed: 2026-01-19
- Files modified:
  - src/render/render_pipeline.cpp
- Notes: Pass pe->accumTexture.texture to CurlAdvectionUpdate call.

## Phase 5: UI and Serialization
- Status: completed
- Started: 2026-01-19
- Completed: 2026-01-19
- Files modified:
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
- Notes: Replaced Lissajous sliders (amplitude, freqX, freqY) with injectionThreshold slider. Updated serialization macro.

## Phase 6: Testing
- Status: pending
