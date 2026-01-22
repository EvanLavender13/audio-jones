---
plan: docs/plans/simulation-bounds-mode.md
branch: simulation-bounds-mode
current_phase: 6
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
- Notes: Created bounds_mode.h with PhysarumBoundsMode and BoidsBoundsMode enums. Added boundsMode field to both config structs. Added uniform locations to both simulation structs.

## Phase 2: Shader Implementation
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - shaders/physarum_agents.glsl
  - shaders/boids_agents.glsl
- Notes: Added boundsMode uniform to both shaders. Physarum: conditional bounds logic. Boids: modified wrapDelta(), added edge avoidance force, conditional position clamping.

## Phase 3: CPU Uniform Binding
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - src/simulation/physarum.cpp
  - src/simulation/boids.cpp
- Notes: Added uniform location lookups and rlSetUniform calls for boundsMode (and edgeMargin for boids).

## Phase 4: UI Controls
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - src/ui/imgui_effects.cpp
- Notes: Added bounds mode combo boxes for both Physarum and Boids sections.

## Phase 5: Serialization
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - src/config/preset.cpp
- Notes: Added boundsMode to NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for both configs.

## Phase 6: Testing & Iteration
- Status: completed
- Started: 2026-01-22
- Completed: 2026-01-22
- Files modified:
  - shaders/physarum_agents.glsl
  - shaders/boids_agents.glsl
  - src/simulation/bounds_mode.h
  - src/simulation/boids.h
  - src/simulation/boids.cpp
  - src/ui/imgui_effects.cpp
  - src/config/preset.cpp
- Notes: Significant iteration during testing:

### Physarum fixes
- **Sensing was toroidal regardless of boundsMode**: Out-of-bounds sensor samples in `sampleTrailAffinity` and `sampleAccumAffinity` now return 1.0 (repulsive) in bounded modes. This was the root cause of bounds modes having no visible effect.
- **Expanded to 5 modes**: TOROIDAL, REFLECT, REDIRECT (point toward center), SCATTER (reflect + ±45° random), RANDOM (pure random heading).
- **Emergent grid behavior**: At high step sizes, the discrete movement + repulsive boundary sensing creates self-organized lattice patterns. Not a bug; emergent from system dynamics.

### Boids fixes
- **Soft repulsion too aggressive**: Reduced from `edgeForce * maxSpeed` to `edgeForce * 0.5` with quadratic falloff for gentle steering.
- **Replaced wanderStrength with accumRepulsion**: Removed random wander behavior. Added gradient-based repulsion from accumulation texture - boids steer down the brightness gradient (away from bright areas). Samples at perceptionRadius distance for shape detection. Renamed field, uniform, UI label, and serialization throughout.

### Known limitations
- Trail diffusion/decay kernel still uses toroidal sampling (`mod()` wrapping). Trails at one edge bleed into the opposite edge in bounded modes. Not fixed due to complexity (shared system across all simulations).
