---
plan: docs/plans/reorderable-trail-boosts.md
branch: reorderable-trail-boosts
current_phase: 4
total_phases: 7
started: 2026-01-20
last_updated: 2026-01-20
---

# Implementation Progress: Reorderable Trail Boosts

## Phase 1: Unify Simulation Defaults
- Status: completed
- Started: 2026-01-20
- Completed: 2026-01-20
- Files modified:
  - src/simulation/physarum.h
  - src/simulation/curl_flow.h
  - src/simulation/curl_advection.h
  - src/simulation/attractor_flow.h
  - src/simulation/boids.h
  - src/simulation/cymatics.h
- Notes: Changed blendMode default from EFFECT_BLEND_BOOST to EFFECT_BLEND_SCREEN in all 6 simulation configs. Updated boostIntensity default to 1.0f where needed (physarum, boids). Updated boostIntensity comment to 0.0-5.0 range in all files.

## Phase 2: Extend Enum and Order Config
- Status: completed
- Started: 2026-01-20
- Completed: 2026-01-20
- Files modified:
  - src/config/effect_config.h
- Notes: Added 6 trail boost enum values (TRANSFORM_PHYSARUM_BOOST through TRANSFORM_CYMATICS_BOOST) before TRANSFORM_EFFECT_COUNT. Added display names to TransformEffectName() switch. Extended TransformOrderConfig::order default initializer with 6 new entries at end. Enum now has 41 entries (TRANSFORM_EFFECT_COUNT = 41).

## Phase 3: Fix Transform Order Serialization
- Status: completed
- Started: 2026-01-20
- Completed: 2026-01-20
- Files modified:
  - src/config/preset.cpp
- Notes: Replaced TransformOrderConfig to_json/from_json with TransformOrderToJson/TransformOrderFromJson helpers. to_json now only saves enabled effects (via IsTransformEnabled). from_json merges saved order with defaults: saved effects first in saved order, then remaining effects in default order. This ensures new effects appear at end and old presets load correctly.

## Phase 4: Extend Dispatch Table
- Status: pending

## Phase 5: Update IsTransformEnabled
- Status: pending

## Phase 6: Remove Hardcoded Blocks
- Status: pending

## Phase 7: UI Category
- Status: pending
