---
plan: docs/plans/simulation-bounds-mode.md
branch: simulation-bounds-mode
current_phase: 5
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
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - shaders/physarum_agents.glsl
  - shaders/boids_agents.glsl
- Notes: Added boundsMode uniform to both shaders. Physarum: replaced mod() wrap with conditional logic for toroidal/reflect/clamp+random modes. Boids: modified wrapDelta() to return direct delta in bounded mode, added edge avoidance steering force before velocity clamp, replaced mod() wrap with conditional toroidal/clamp logic.

## Phase 3: CPU Uniform Binding
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - src/simulation/physarum.cpp
  - src/simulation/boids.cpp
- Notes: Added boundsModeLoc uniform location lookup in LoadComputeProgram for both simulations. Set boundsMode uniform in PhysarumUpdate and BoidsUpdate from config. Boids edgeMargin derives from perceptionRadius for natural avoidance scaling.

## Phase 4: UI Controls
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - src/ui/imgui_effects.cpp
- Notes: Added PHYSARUM_BOUNDS_MODES and BOIDS_BOUNDS_MODES string arrays. Added ImGui::Combo for bounds mode selection in both Physarum and Boids sections, placed after "Enabled" checkbox. Included bounds_mode.h for enum types.

## Phase 5: Serialization
- Status: pending

## Phase 6: Testing
- Status: pending
