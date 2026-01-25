---
plan: docs/plans/ink-wash.md
branch: ink-wash
mode: parallel
current_phase: 5
current_wave: 3
total_phases: 4
total_waves: 2
waves:
  1: [1, 2]
  2: [3, 4]
started: 2026-01-24
last_updated: 2026-01-24
---

# Implementation Progress: Ink Wash

## Phase 1: Config + Registration
- Status: completed
- Wave: 1
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - src/config/ink_wash_config.h (created)
  - src/config/effect_config.h
- Notes: Added InkWashConfig struct, TRANSFORM_INK_WASH enum entry, name mapping, order array entry, EffectConfig member, and IsTransformEnabled case.

## Phase 2: Shader
- Status: completed
- Wave: 1
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - shaders/ink_wash.fs (created)
- Notes: Full ink wash shader with Sobel edge detection, FBM paper granulation (5-octave), and 5-tap directional color bleed along gradient.

## Phase 3: PostEffect + ShaderSetup
- Status: completed
- Wave: 2
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - src/render/post_effect.h
  - src/render/post_effect.cpp
  - src/render/shader_setup.h
  - src/render/shader_setup.cpp
- Notes: Added inkWashShader to PostEffect, uniform locations, shader load/unload, TRANSFORM_INK_WASH case in GetTransformEffect, SetupInkWash function.

## Phase 4: UI + Serialization + Modulation
- Status: completed
- Wave: 2
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - src/ui/imgui_effects.cpp
  - src/ui/imgui_effects_style.cpp
  - src/config/preset.cpp
  - src/automation/param_registry.cpp
- Notes: Added Ink Wash to Style category, DrawStyleInkWash UI section with 3 modulatable sliders, JSON serialization, param registry entries for modulation.
