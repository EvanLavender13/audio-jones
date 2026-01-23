# Physarum Boundary Modes

Extends the physarum bounds system with 6 attractor-based modes that produce structured emergent patterns (constellations, spirographs, Voronoi territories) when combined with Lévy flights. Adds a redirect-vs-respawn toggle and an independent gravity well parameter.

## Research

- **Source**: `docs/research/physarum-boundary-modes.md`
- **Key insight**: Interesting patterns require the boundary mode to change *where* agents are drawn to, not just what direction they face. Redirect produces distinct geometric structure because agents oscillate between boundary and attractor.

## Current State

- `src/simulation/bounds_mode.h:4-10` — `PhysarumBoundsMode` enum with 5 values (0-4)
- `shaders/physarum_agents.glsl:263-313` — 5-way if-chain for boundary handling
- `src/simulation/physarum.h:20-41` — `PhysarumConfig` struct with `boundsMode` field
- `src/simulation/physarum.cpp:65,179-180` — Uniform location lookup + upload for `boundsMode`
- `src/ui/imgui_effects.cpp:103,214-217` — Label array + Combo widget
- `src/config/preset.cpp:89-92` — Serialization macro includes `boundsMode`
- `src/automation/param_registry.cpp:12-20,226-236` — Physarum float param registration

## New Parameters

| Field | Type | Default | Uniform | Modulatable | UI |
|-------|------|---------|---------|-------------|-----|
| `boundsMode` | enum (extended) | 0 | `int boundsMode` | No | Combo (10 entries) |
| `respawnMode` | bool | false | `float respawnMode` | No | Checkbox |
| `gravityStrength` | float | 0.0 | `float gravityStrength` | Yes (0.0–1.0) | ModulatableSlider |
| `orbitOffset` | float | 0.0 | `float orbitOffset` | Yes (0.0–1.0) | ModulatableSlider |

Note: `attractorCount` from the research doc stays as a uniform int (not modulatable). Range 2–8.

---

## Phase 1: New Boundary Modes (Enum + Shader)

**Goal**: Add modes 5-9 to the bounds enum and implement their shader logic.

**Build**:

- `src/simulation/bounds_mode.h` — Extend enum with 5 new values:
  ```
  PHYSARUM_BOUNDS_FIXED_HOME = 5
  PHYSARUM_BOUNDS_ORBIT = 6
  PHYSARUM_BOUNDS_SPECIES_ORBIT = 7
  PHYSARUM_BOUNDS_MULTI_HOME = 8
  PHYSARUM_BOUNDS_ANTIPODAL = 9
  ```

- `src/simulation/physarum.h` — Add to PhysarumConfig:
  - `int attractorCount = 4;` (for multi-home mode)

- `src/simulation/physarum.cpp` — Add uniform location field `attractorCountLoc` to Physarum struct, lookup in LoadComputeProgram, upload in PhysarumUpdate as INT.

- `shaders/physarum_agents.glsl` — Add `uniform int attractorCount;` declaration. Extend the bounds if-chain with modes 5-9:

  **Mode 5 — Fixed Home**: On boundary hit, compute deterministic home position from `hash(id)`, redirect heading toward home.
  ```glsl
  } else if (boundsMode == 5) {
      bool hitEdge = false;
      if (pos.x < 0.0) { pos.x = 0.0; hitEdge = true; }
      if (pos.x >= resolution.x) { pos.x = resolution.x - 1.0; hitEdge = true; }
      if (pos.y < 0.0) { pos.y = 0.0; hitEdge = true; }
      if (pos.y >= resolution.y) { pos.y = resolution.y - 1.0; hitEdge = true; }
      if (hitEdge) {
          uint homeHash = hash(id * 3u + 12345u);
          float homeX = float(homeHash) / 4294967295.0 * resolution.x;
          homeHash = hash(homeHash);
          float homeY = float(homeHash) / 4294967295.0 * resolution.y;
          vec2 toHome = vec2(homeX, homeY) - pos;
          agent.heading = atan(toHome.y, toHome.x);
      }
  }
  ```

  **Mode 6 — Orbit**: On boundary hit, redirect perpendicular to center vector (tangent to circle).
  ```glsl
  } else if (boundsMode == 6) {
      bool hitEdge = false;
      if (pos.x < 0.0) { pos.x = 0.0; hitEdge = true; }
      if (pos.x >= resolution.x) { pos.x = resolution.x - 1.0; hitEdge = true; }
      if (pos.y < 0.0) { pos.y = 0.0; hitEdge = true; }
      if (pos.y >= resolution.y) { pos.y = resolution.y - 1.0; hitEdge = true; }
      if (hitEdge) {
          vec2 toCenter = (resolution * 0.5) - pos;
          agent.heading = atan(toCenter.y, toCenter.x) + PI * 0.5;
      }
  }
  ```

  **Mode 7 — Per-Species Orbit**: Same as orbit but offset tangent angle by agent hue, scaled by `orbitOffset` uniform (added in Phase 3, use 1.0 constant here as placeholder until then).
  ```glsl
  } else if (boundsMode == 7) {
      bool hitEdge = false;
      if (pos.x < 0.0) { pos.x = 0.0; hitEdge = true; }
      if (pos.x >= resolution.x) { pos.x = resolution.x - 1.0; hitEdge = true; }
      if (pos.y < 0.0) { pos.y = 0.0; hitEdge = true; }
      if (pos.y >= resolution.y) { pos.y = resolution.y - 1.0; hitEdge = true; }
      if (hitEdge) {
          vec2 toCenter = (resolution * 0.5) - pos;
          float speciesOffset = agent.hue * TWO_PI * orbitOffset;
          agent.heading = atan(toCenter.y, toCenter.x) + PI * 0.5 + speciesOffset;
      }
  }
  ```

  **Mode 8 — Multi-Home**: Assign agent to one of K attractors via `hash(id) % attractorCount`, redirect toward it.
  ```glsl
  } else if (boundsMode == 8) {
      bool hitEdge = false;
      if (pos.x < 0.0) { pos.x = 0.0; hitEdge = true; }
      if (pos.x >= resolution.x) { pos.x = resolution.x - 1.0; hitEdge = true; }
      if (pos.y < 0.0) { pos.y = 0.0; hitEdge = true; }
      if (pos.y >= resolution.y) { pos.y = resolution.y - 1.0; hitEdge = true; }
      if (hitEdge) {
          uint attractorIdx = hash(id) % uint(attractorCount);
          uint attractorSeed = hash(attractorIdx * 7u + 99u);
          float ax = float(attractorSeed) / 4294967295.0 * resolution.x;
          attractorSeed = hash(attractorSeed);
          float ay = float(attractorSeed) / 4294967295.0 * resolution.y;
          vec2 toAttractor = vec2(ax, ay) - pos;
          agent.heading = atan(toAttractor.y, toAttractor.x);
      }
  }
  ```

  **Mode 9 — Antipodal Respawn**: On boundary hit, teleport to diametrically opposite position.
  ```glsl
  } else if (boundsMode == 9) {
      bool hitEdge = false;
      if (pos.x < 0.0) { pos.x = 0.0; hitEdge = true; }
      if (pos.x >= resolution.x) { pos.x = resolution.x - 1.0; hitEdge = true; }
      if (pos.y < 0.0) { pos.y = 0.0; hitEdge = true; }
      if (pos.y >= resolution.y) { pos.y = resolution.y - 1.0; hitEdge = true; }
      if (hitEdge) {
          vec2 center = resolution * 0.5;
          pos = 2.0 * center - pos;
          pos = clamp(pos, vec2(0.0), resolution - 1.0);
      }
  }
  ```

- `src/ui/imgui_effects.cpp` — Update label array to 10 entries:
  ```
  "Toroidal", "Reflect", "Redirect", "Scatter", "Random",
  "Fixed Home", "Orbit", "Species Orbit", "Multi-Home", "Antipodal"
  ```
  Update Combo count from 5 to 10.

- `src/config/preset.cpp` — Add `attractorCount` to PhysarumConfig serialization macro.

**Done when**: Modes 5-9 selectable via UI, each produces distinct visual behavior with Lévy flights enabled.

---

## Phase 2: Redirect vs Respawn Toggle

**Goal**: Add a global bool that changes attractor-based modes from steering toward targets to teleporting to targets.

**Build**:

- `src/simulation/physarum.h` — Add `bool respawnMode = false;` to PhysarumConfig. Add `int respawnModeLoc;` to Physarum struct.

- `src/simulation/physarum.cpp` — Lookup `respawnMode` uniform location. Upload as float (0.0/1.0 pattern matching `vectorSteering`).

- `shaders/physarum_agents.glsl` — Add `uniform float respawnMode;` declaration. Modify modes 2 (redirect), 5 (fixed home), 6 (orbit), 7 (species orbit), 8 (multi-home) to branch on `respawnMode > 0.5`:
  - **Redirect path** (existing): Set `agent.heading = atan(...)` toward target.
  - **Respawn path** (new): Set `pos = target` (teleport to the computed target position), keep heading unchanged or randomize.

  Example for mode 2 (center redirect):
  ```glsl
  if (hitEdge) {
      vec2 target = resolution * 0.5;
      if (respawnMode > 0.5) {
          pos = target;
      } else {
          vec2 toTarget = target - pos;
          agent.heading = atan(toTarget.y, toTarget.x);
      }
  }
  ```

  Apply same pattern to modes 5, 6, 7, 8. Mode 9 (antipodal) already teleports — respawnMode has no effect on it.

- `src/ui/imgui_effects.cpp` — Add checkbox: `ImGui::Checkbox("Respawn", &e->physarum.respawnMode);` after the Bounds combo. Only show when `boundsMode >= 2` (modes that use attractor targets).

- `src/config/preset.cpp` — Add `respawnMode` to serialization macro.

**Done when**: Toggling respawn produces visually distinct starburst patterns (teleport) vs connected web structure (redirect) on modes 2, 5-8.

---

## Phase 3: Gravity Well + Species Orbit Offset

**Goal**: Add `gravityStrength` as a continuous force (independent of bounds mode) and `orbitOffset` for per-species angular separation.

**Build**:

- `src/simulation/physarum.h` — Add to PhysarumConfig:
  - `float gravityStrength = 0.0f;`
  - `float orbitOffset = 0.0f;`
  Add location fields to Physarum struct: `gravityStrengthLoc`, `orbitOffsetLoc`.

- `src/simulation/physarum.cpp` — Lookup + upload both uniforms as FLOAT.

- `shaders/physarum_agents.glsl` — Add uniform declarations. Insert gravity force application **before** the bounds if-chain (it applies every frame regardless of mode):
  ```glsl
  // Gravity well: continuous inward acceleration
  if (gravityStrength > 0.001) {
      vec2 toCenter = (resolution * 0.5) - pos;
      float dist = length(toCenter);
      float force = gravityStrength * (dist / length(resolution));
      float forceAngle = atan(toCenter.y, toCenter.x);
      agent.heading = mix(agent.heading, forceAngle, force);
  }
  ```
  Remove the `1.0` placeholder from mode 7 and use the `orbitOffset` uniform.

- `src/ui/imgui_effects.cpp` — Add ModulatableSliders for both params:
  ```
  ModulatableSlider("Gravity", &e->physarum.gravityStrength,
                    "physarum.gravityStrength", "%.2f", modSources);
  ModulatableSlider("Orbit Offset", &e->physarum.orbitOffset,
                    "physarum.orbitOffset", "%.2f", modSources);
  ```
  Place after the Respawn checkbox.

- `src/automation/param_registry.cpp` — Register both:
  - `{"physarum.gravityStrength", {0.0f, 1.0f}}` + target pointer
  - `{"physarum.orbitOffset", {0.0f, 1.0f}}` + target pointer

- `src/config/preset.cpp` — Add `gravityStrength`, `orbitOffset` to serialization macro.

**Done when**: Gravity creates smooth orbital arcs that combine with any bounds mode. Orbit offset separates species into distinct rotation bands. Both respond to audio modulation.

---

## Phase 4: Conditional Visibility

**Goal**: Show only relevant controls for the active bounds mode.

**Build**:

- `src/ui/imgui_effects.cpp` — Add conditional visibility:
  - `attractorCount` slider: Show only when `boundsMode == 8` (Multi-Home). Range 2-8.
  - `orbitOffset` slider: Show only when `boundsMode == 7` (Species Orbit).
  - `respawnMode` checkbox: Show only when `boundsMode` is 2, 5, 6, 7, or 8 (attractor-based modes).
  - `gravityStrength` slider: Always visible (independent of bounds mode).

**Done when**: UI hides controls irrelevant to the currently selected bounds mode.
