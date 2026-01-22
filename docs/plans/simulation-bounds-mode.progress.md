---
plan: docs/plans/simulation-bounds-mode.md
branch: simulation-bounds-mode
current_phase: 2
total_phases: 6
started: 2026-01-22
last_updated: 2026-01-22
---

# Implementation Progress: Simulation Bounds Mode

## Phase 1: Enum and Config Changes
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - src/simulation/bounds_mode.h (created)
  - src/simulation/physarum.h
  - src/simulation/boids.h
- Notes: Created bounds_mode.h with PhysarumBoundsMode (TOROIDAL, REFLECT, CLAMP_RANDOM) and BoidsBoundsMode (TOROIDAL, SOFT_REPULSION) enums. Added boundsMode field to both config structs. Added boundsModeLoc uniform location to both simulation structs. Added edgeMarginLoc to Boids for soft repulsion mode.

## Phase 2: Shader Implementation
- Status: pending

## Phase 3: CPU Uniform Binding
- Status: pending

## Phase 4: UI Controls
- Status: pending

## Phase 5: Serialization
- Status: pending

## Phase 6: Testing
- Status: pending
