---
plan: docs/plans/glitch.md
branch: glitch
current_phase: 4
total_phases: 4
started: 2026-01-09
last_updated: 2026-01-09
---

# Implementation Progress: Glitch Effect

## Phase 1: Config and Shader
- Status: completed
- Started: 2026-01-09
- Completed: 2026-01-09
- Files modified:
  - src/config/glitch_config.h (created)
  - shaders/glitch.fs (created)
- Notes: Created GlitchConfig struct with all 4 sub-modes (CRT, Analog, Digital, VHS) plus overlay params. Shader implements gradient noise, CRT barrel distortion, analog horizontal distortion with chromatic aberration, digital block displacement, VHS tracking bars, and overlay scanlines/noise.

## Phase 2: Pipeline Integration
- Status: completed
- Started: 2026-01-09
- Completed: 2026-01-09
- Files modified:
  - src/config/effect_config.h
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
- Notes: Added TRANSFORM_GLITCH enum, GlitchConfig to EffectConfig, shader loading, 18 uniform locations, SetupGlitch function with CPU time accumulation, and transform effect dispatch entry.

## Phase 3: UI and Serialization
- Status: completed
- Started: 2026-01-09
- Completed: 2026-01-09
- Files modified:
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
- Notes: Added Glitch section to Effects panel with collapsible CRT/Analog/Digital/VHS sub-sections. Used ModulatableSlider for analogIntensity, aberration, blockThreshold, blockOffset. Added NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT macro and to_json/from_json entries.

## Phase 4: Modulation Registration
- Status: completed
- Started: 2026-01-09
- Completed: 2026-01-09
- Files modified:
  - src/automation/param_registry.cpp
- Notes: Added 4 glitch params to PARAM_TABLE (analogIntensity, blockThreshold, aberration, blockOffset) with corresponding pointers in ParamRegistryInit targets array.
