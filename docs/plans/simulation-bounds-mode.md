# Simulation Bounds Mode

Add configurable bounds behavior to Physarum and Boids simulations. Currently both use toroidal wrapping (agents exiting one edge appear at the opposite). This adds options for agents to respect window boundaries via reflection, random scattering, or soft repulsion.

## Current State

- `shaders/physarum_agents.glsl:247` - `pos = mod(pos, resolution);`
- `shaders/boids_agents.glsl:196` - `selfPos = mod(selfPos, resolution);`
- `shaders/boids_agents.glsl:58-63` - `wrapDelta()` helper for toroidal distance
- `src/simulation/physarum.h:19-39` - `PhysarumConfig` struct
- `src/simulation/boids.h:21-40` - `BoidsConfig` struct
- `src/ui/imgui_effects.cpp:206-238` - Physarum UI panel
- `src/ui/imgui_effects.cpp:321-347` - Boids UI panel

## Technical Implementation

### Physarum Bounds Modes

**Toroidal (current):**
```glsl
pos = mod(pos, resolution);
```

**Reflect:**
```glsl
// Reflect position and heading when outside bounds
if (pos.x < 0.0 || pos.x >= resolution.x) {
    pos.x = clamp(pos.x, 0.0, resolution.x - 1.0);
    agent.heading = 3.14159 - agent.heading;  // Mirror horizontally
}
if (pos.y < 0.0 || pos.y >= resolution.y) {
    pos.y = clamp(pos.y, 0.0, resolution.y - 1.0);
    agent.heading = -agent.heading;  // Mirror vertically
}
```

**Clamp + Random:**
```glsl
// Clamp and randomize heading pointing inward
bool hitEdge = false;
if (pos.x < 0.0) { pos.x = 0.0; hitEdge = true; }
if (pos.x >= resolution.x) { pos.x = resolution.x - 1.0; hitEdge = true; }
if (pos.y < 0.0) { pos.y = 0.0; hitEdge = true; }
if (pos.y >= resolution.y) { pos.y = resolution.y - 1.0; hitEdge = true; }

if (hitEdge) {
    // Random heading biased toward center
    vec2 toCenter = (resolution * 0.5) - pos;
    float baseAngle = atan(toCenter.y, toCenter.x);
    float randomOffset = (float(hash(hashState)) / 4294967295.0 - 0.5) * 1.57;  // +/- 45 degrees
    agent.heading = baseAngle + randomOffset;
}
```

### Boids Bounds Modes

**Toroidal (current):**
```glsl
selfPos = mod(selfPos, resolution);
```

**Soft Repulsion:**
```glsl
// Edge avoidance steering force
float edgeMargin = 50.0;  // Pixels from edge to start avoiding
vec2 edgeForce = vec2(0.0);

if (selfPos.x < edgeMargin) {
    edgeForce.x += (edgeMargin - selfPos.x) / edgeMargin;
}
if (selfPos.x > resolution.x - edgeMargin) {
    edgeForce.x -= (selfPos.x - (resolution.x - edgeMargin)) / edgeMargin;
}
if (selfPos.y < edgeMargin) {
    edgeForce.y += (edgeMargin - selfPos.y) / edgeMargin;
}
if (selfPos.y > resolution.y - edgeMargin) {
    edgeForce.y -= (selfPos.y - (resolution.y - edgeMargin)) / edgeMargin;
}

// Apply as strong steering force (before velocity clamp)
selfVel += edgeForce * edgeAvoidanceWeight;

// Hard clamp as fallback (prevents escape if velocity too high)
selfPos = clamp(selfPos, vec2(0.0), resolution - vec2(1.0));
```

### Boids wrapDelta Adjustment

When in bounded mode, `wrapDelta()` should return direct delta (no wrap consideration):

```glsl
vec2 wrapDelta(vec2 from, vec2 to)
{
    if (boundsMode == 0) {  // Toroidal
        vec2 delta = to - from;
        return mod(delta + resolution * 0.5, resolution) - resolution * 0.5;
    } else {  // Bounded
        return to - from;
    }
}
```

---

## Phase 1: Enum and Config Changes

**Goal**: Define bounds mode enums and add to config structs.

**Build**:
- Create `src/simulation/bounds_mode.h` with two enums:
  - `PhysarumBoundsMode`: TOROIDAL, REFLECT, CLAMP_RANDOM
  - `BoidsBoundsMode`: TOROIDAL, SOFT_REPULSION
- Add `boundsMode` field to `PhysarumConfig` in `src/simulation/physarum.h`
- Add `boundsMode` field to `BoidsConfig` in `src/simulation/boids.h`
- Add uniform location `boundModeLoc` to both `Physarum` and `Boids` structs

**Done when**: Builds without errors; new fields exist with defaults.

---

## Phase 2: Shader Implementation

**Goal**: Implement bounds logic in compute shaders.

**Build**:
- `shaders/physarum_agents.glsl`:
  - Add `uniform int boundsMode;`
  - Replace `pos = mod(pos, resolution);` with conditional logic for all three modes
- `shaders/boids_agents.glsl`:
  - Add `uniform int boundsMode;`
  - Add `uniform float edgeMargin;` (fixed at 50.0 initially, or derive from perceptionRadius)
  - Modify `wrapDelta()` to check `boundsMode`
  - Add edge avoidance force before velocity clamp
  - Replace `selfPos = mod(selfPos, resolution);` with conditional logic

**Done when**: Shader compiles; toroidal mode (0) behaves identically to current.

---

## Phase 3: CPU Uniform Binding

**Goal**: Pass bounds mode to shaders.

**Build**:
- `src/simulation/physarum.cpp`:
  - Get uniform location for `boundsMode` in `PhysarumInit`
  - Set uniform in `PhysarumUpdate` from `config.boundsMode`
- `src/simulation/boids.cpp`:
  - Get uniform locations for `boundsMode` and `edgeMargin` in `BoidsInit`
  - Set uniforms in `BoidsUpdate`

**Done when**: Changing config value changes shader behavior.

---

## Phase 4: UI Controls

**Goal**: Add combo boxes for bounds mode selection.

**Build**:
- `src/ui/imgui_effects.cpp`:
  - Add `PHYSARUM_BOUNDS_MODES` string array: "Toroidal", "Reflect", "Clamp Random"
  - Add `BOIDS_BOUNDS_MODES` string array: "Toroidal", "Soft Repulsion"
  - Add `ImGui::Combo("Bounds", ...)` to Physarum section (after "Enabled")
  - Add `ImGui::Combo("Bounds", ...)` to Boids section (after "Enabled")

**Done when**: UI shows combo; selection persists across frames.

---

## Phase 5: Serialization

**Goal**: Save/load bounds mode in presets.

**Build**:
- `src/config/preset.cpp`:
  - Add `boundsMode` to `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for both configs
  - Enum serializes as int automatically

**Done when**: Save preset with bounded mode; reload restores it correctly.

---

## Phase 6: Testing

**Goal**: Verify all modes work correctly.

**Build**:
- Test Physarum:
  - Toroidal: agents wrap edges (baseline)
  - Reflect: agents bounce off edges, no pile-up at corners
  - Clamp Random: agents scatter inward from edges
- Test Boids:
  - Toroidal: flock wraps edges (baseline)
  - Soft Repulsion: flock avoids edges, no wall-hugging

**Done when**: All modes produce distinct, correct visual behavior.
