# Physarum Competitive Species

Add explicit repulsion between different-hue agents, transforming soft color clustering into territorial competition with a tunable `repulsionStrength` parameter.

## Current State

Agents prefer trails of similar hue but still follow dissimilar trails over empty space:
- `shaders/physarum_agents.glsl:77-92` — `computeAffinity()` returns lower values for similar hues (more attractive), but all trails remain more attractive than empty pixels
- `src/simulation/physarum.h:19-34` — `PhysarumConfig` struct
- `src/simulation/physarum.cpp:50-62` — uniform location pattern
- `src/ui/imgui_effects.cpp:183-208` — Physarum UI section
- `src/automation/param_registry.cpp:13-16` — physarum param entries

Current affinity math:
```glsl
// hueDiff ranges 0.0 (same) to 0.5 (opposite)
// Empty space returns 1.0, opposite-hue trail returns ~0.5-0.8
return hueDiff + (1.0 - intensity) * 0.3;
```

## Technical Implementation

**Goal**: Opposite-hue trails score WORSE than empty space (repulsion).

**Core algorithm** (to be refined in Phase 1):
```glsl
uniform float repulsionStrength;  // 0 = current behavior, 1 = strong repulsion

float computeAffinity(vec3 color, float agentHue)
{
    float intensity = dot(color, LUMA_WEIGHTS);
    if (intensity < 0.001) {
        return 0.5;  // Empty space = neutral
    }

    vec3 hsv = rgb2hsv(color);
    float hueDiff = hueDifference(agentHue, hsv.x);

    // Map hueDiff (0-0.5) to similarity (-1 to +1)
    float similarity = 1.0 - 2.0 * hueDiff;

    // Blend from attraction-only (current) to full repulsion
    // When repulsionStrength=0: opposite hue still attracts (legacy)
    // When repulsionStrength=1: opposite hue repels (score > 0.5)
    float attraction = intensity * similarity;
    float baseAffinity = 0.5 - attraction * 0.4;  // Center at neutral

    // Lerp between old behavior and new
    float oldAffinity = hueDiff + (1.0 - intensity) * 0.3;
    return mix(oldAffinity, baseAffinity, repulsionStrength);
}
```

**Visual impact**:
| Scenario | repulsion=0 | repulsion=1 |
|----------|-------------|-------------|
| Agent near same-hue trail | Follows | Follows |
| Agent near opposite-hue trail | Follows (slower) | **Flees** |
| Agent between two species | Drifts toward brighter | **Pushed to boundary** |
| Long-term pattern | Soft gradients | Hard territories |

---

## Phase 1: Core Algorithm

**Goal**: Implement repulsion logic with hardcoded strength for visual validation.

**Build**:
- `shaders/physarum_agents.glsl` — Replace `computeAffinity()` with new repulsion-aware version. Hardcode `repulsionStrength = 0.4` temporarily (no uniform yet). Key changes:
  - Empty space returns `0.5` (neutral) instead of `1.0`
  - `similarity = 1.0 - 2.0 * hueDiff` maps hue difference to signed range
  - `baseAffinity = 0.5 - intensity * similarity * 0.4` centers at neutral, scales by intensity

**Done when**: Enabling physarum shows agents fleeing from opposite-hue trails and forming distinct territories instead of blending gradients.

---

## Phase 2: Parameter Integration

**Goal**: Expose `repulsionStrength` as a modulatable parameter.

**Build**:
- `src/simulation/physarum.h` — Add `float repulsionStrength = 0.4f;` to `PhysarumConfig`
- `src/simulation/physarum.h` — Add `int repulsionStrengthLoc;` to `Physarum` struct
- `src/simulation/physarum.cpp` — Get uniform location in `LoadComputeProgram()`:
  ```cpp
  p->repulsionStrengthLoc = rlGetLocationUniform(program, "repulsionStrength");
  ```
- `src/simulation/physarum.cpp` — Set uniform in `PhysarumUpdate()`:
  ```cpp
  rlSetUniform(p->repulsionStrengthLoc, &p->config.repulsionStrength, RL_SHADER_UNIFORM_FLOAT, 1);
  ```
- `shaders/physarum_agents.glsl` — Change hardcoded `0.4` to `uniform float repulsionStrength;`
- `src/ui/imgui_effects.cpp` — Add slider in Physarum section after "Sense Blend":
  ```cpp
  ModulatableSlider("Repulsion", &e->physarum.repulsionStrength,
                    "physarum.repulsionStrength", "%.2f", modSources);
  ```
- `src/automation/param_registry.cpp` — Add to PARAM_TABLE:
  ```cpp
  {"physarum.repulsionStrength", {0.0f, 1.0f}},
  ```
- `src/automation/param_registry.cpp` — Add pointer to targets array:
  ```cpp
  &effects->physarum.repulsionStrength,
  ```

**Done when**: Repulsion slider appears in Physarum UI, responds to audio modulation, and smoothly transitions between soft clustering (0) and hard territories (1).

---

## Implementation Notes

The original plan's affinity-only approach failed. Standard physarum uses discrete steering: sample 3 forward sensors, turn toward best option. This only selects the "least bad" direction — it doesn't actively flee from repulsive things. If front and left are same-hue (attractive) and right is different-hue (repulsive), the agent ignores the repulsive sensor entirely.

**Solution**: Added optional vector-based steering (boids-style). Each sensor contributes a force vector — attractive sensors pull toward, repulsive sensors push away. The net steering vector determines turn direction, so repulsion on one side creates a push even when other directions look fine.

**Final affinity formula**:
```glsl
float attraction = intensity * (1.0 - hueDiff * 2.0);  // 1 at same hue, 0 at opposite
float repulsion = intensity * hueDiff * 2.0;          // 0 at same hue, 1 at opposite
return 0.5 - attraction * 0.5 + repulsion * 2.0 * repulsionStrength;
```

Key difference from plan: ANY hue difference causes repulsion, not just "opposite" hues. Red flees from blue (120° apart), not just cyan (180° apart).

**Added parameters**:
- `repulsionStrength` (0-1): Scales repulsion intensity. Default 0 for preset compatibility.
- `vectorSteering` (bool): Toggles between original discrete steering (organic branching) and vector steering (smoother paths, effective repulsion).

Vector steering changes the overall feel — more streamlined veins vs organic jittery branching. Toggle allows choosing per-preset.
