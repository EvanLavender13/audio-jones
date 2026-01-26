# Physarum Walk Modes

Replace the single Levy flight parameter with a family of step-size strategies. Each mode produces distinct emergent network topologies by applying different distributions, correlations, or state-dependent scaling to agent movement.

## Current State

- `src/simulation/physarum.h:29` - `levyAlpha` parameter in PhysarumConfig
- `src/simulation/physarum.cpp:56` - `levyAlphaLoc` uniform location
- `shaders/physarum_agents.glsl:264-274` - Levy flight step calculation
- `src/ui/imgui_effects.cpp:262` - "Levy Alpha" slider in Movement section
- `src/automation/param_registry.cpp:18` - `physarum.levyAlpha` bounds {0.0f, 3.0f}
- `src/config/preset.cpp:91` - `levyAlpha` in PhysarumConfig serialization

## Technical Implementation

**Source**: [docs/research/physarum-walk-modes.md](../research/physarum-walk-modes.md)

### Walk Mode Enum

```cpp
typedef enum {
    PHYSARUM_WALK_NORMAL = 0,      // Fixed step = stepSize
    PHYSARUM_WALK_LEVY = 1,        // Power-law: stepSize * pow(u, -1/alpha)
    PHYSARUM_WALK_PERSISTENT = 2,  // Heading blends toward previous
    PHYSARUM_WALK_RUN_TUMBLE = 3,  // Two-state run/tumble switching
    PHYSARUM_WALK_ANTIPERSISTENT = 4, // Heading biases away from previous
    PHYSARUM_WALK_BALLISTIC = 5,   // Straight lines until density threshold
    PHYSARUM_WALK_ADAPTIVE = 6,    // Step scales with local density
} PhysarumWalkMode;
```

### Shader Step Computation

Replace lines 264-274 of `physarum_agents.glsl` with mode switch. Note: per-agent state variables (`_pad1`, `_pad2`) are read inside each branch to avoid aliasing.

```glsl
// Walk mode step computation
float agentStep = stepSize;
bool skipChemotaxis = false;  // Ballistic mode disables sensor-based turning

if (walkMode == 0) {
    // Normal: fixed step
    agentStep = stepSize;
}
else if (walkMode == 1) {
    // Levy: power-law distribution
    if (levyAlpha > 0.001) {
        float u = max(float(hash(hashState)) / 4294967295.0, 0.001);
        hashState = hash(hashState);
        agentStep = stepSize * pow(u, -1.0 / levyAlpha);
        agentStep = min(agentStep, stepSize * 50.0);
    }
}
else if (walkMode == 2) {
    // Persistent: blend toward previous heading
    float prevHeading = agent._pad1;
    agent.heading = mix(agent.heading, prevHeading, persistence);
    agent._pad1 = agent.heading;  // Store for next frame
    agentStep = stepSize;
}
else if (walkMode == 3) {
    // Run & Tumble: two-state mode
    float timer = agent._pad1;
    float modeState = agent._pad2;
    timer -= 1.0;
    if (timer <= 0.0) {
        bool wasRunning = modeState > 0.5;
        modeState = wasRunning ? 0.0 : 1.0;
        timer = wasRunning ? tumbleDuration : runDuration;
        if (wasRunning) {
            float rndAngle = (float(hash(hashState)) / 4294967295.0 - 0.5) * TWO_PI;
            hashState = hash(hashState);
            agent.heading += rndAngle;
        }
    }
    agent._pad1 = timer;
    agent._pad2 = modeState;
    bool running = modeState > 0.5;
    agentStep = running ? stepSize * runMultiplier : stepSize * 0.2;
}
else if (walkMode == 4) {
    // Antipersistent: bias away from previous direction
    float prevHeading = agent._pad1;
    float diff = agent.heading - prevHeading;
    agent.heading += diff * antiPersistence;
    agent._pad1 = agent.heading;  // Store for next frame
    agentStep = stepSize;
}
else if (walkMode == 5) {
    // Ballistic: straight lines until density threshold (DLA-like)
    ivec2 coord = ivec2(pos);
    float localDensity = dot(imageLoad(trailMap, coord).rgb, LUMA_WEIGHTS);
    bool stuck = localDensity > stickThreshold;
    agentStep = stuck ? stepSize * 0.05 : stepSize;
    skipChemotaxis = !stuck;  // Unstuck agents ignore sensors, move straight
}
else if (walkMode == 6) {
    // Adaptive: step scales with local density
    ivec2 coord = ivec2(pos);
    float localDensity = dot(imageLoad(trailMap, coord).rgb, LUMA_WEIGHTS);
    float scale = mix(1.0, densityResponse, localDensity);
    agentStep = stepSize * scale;
}

pos += moveDir * agentStep;
```

**Chemotaxis bypass**: The `skipChemotaxis` flag must be checked earlier in the shader (before the sensor sampling block at lines 189-259). When `true`, skip the heading adjustment logic—agents maintain their current heading.

### Parameters

| Parameter | Type | Range | Default | Mode | Effect |
|-----------|------|-------|---------|------|--------|
| walkMode | int | 0-6 | 0 | All | Selects active strategy |
| persistence | float | 0.0-1.0 | 0.5 | Persistent | Directional memory strength |
| antiPersistence | float | 0.0-1.0 | 0.3 | Antipersistent | Reversal tendency |
| runDuration | float | 10-100 | 30 | Run & Tumble | Frames per run phase |
| tumbleDuration | float | 5-30 | 10 | Run & Tumble | Frames per tumble phase |
| runMultiplier | float | 1.0-5.0 | 2.0 | Run & Tumble | Speed boost during run |
| stickThreshold | float | 0.0-1.0 | 0.5 | Ballistic | Density to trigger sticking |
| densityResponse | float | 0.1-5.0 | 1.5 | Adaptive | Step scale factor |

### Per-Agent State Usage

| Mode | _pad1 | _pad2 | _pad3 | _pad4 |
|------|-------|-------|-------|-------|
| Persistent | prev_heading | — | — | — |
| Run & Tumble | timer | mode (0/1) | — | — |
| Antipersistent | prev_heading | — | — | — |
| Others | unused | unused | unused | unused |

### State Reset Logic

When `walkMode` changes, zero `_pad1` through `_pad4` for all agents. Implement via CPU-side buffer update in `PhysarumApplyConfig()`.

---

## Phase 1: Config and Enum

**Goal**: Add walk mode enum and parameters to PhysarumConfig.
**Depends on**: —
**Files**: `src/simulation/physarum.h`

**Build**:
- Add `PhysarumWalkMode` enum with 7 values (NORMAL through ADAPTIVE)
- Add `walkMode` field to PhysarumConfig (default: PHYSARUM_WALK_NORMAL)
- Add 7 new parameters: `persistence`, `antiPersistence`, `runDuration`, `tumbleDuration`, `runMultiplier`, `stickThreshold`, `densityResponse`
- Keep `levyAlpha` unchanged (used when walkMode=1)

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: PhysarumConfig contains walkMode enum and all mode-specific parameters.

---

## Phase 2: Shader Uniforms

**Goal**: Bind walk mode parameters as shader uniforms.
**Depends on**: Phase 1
**Files**: `src/simulation/physarum.h`, `src/simulation/physarum.cpp`

**Build**:
- Add uniform location fields to Physarum struct: `walkModeLoc`, `persistenceLoc`, `antiPersistenceLoc`, `runDurationLoc`, `tumbleDurationLoc`, `runMultiplierLoc`, `stickThresholdLoc`, `densityResponseLoc`
- Get uniform locations in `LoadComputeProgram()`
- Set uniform values in `PhysarumUpdate()`

**Verify**: `cmake.exe --build build` compiles. No runtime errors when physarum enabled.

**Done when**: All walk mode uniforms bind to shader.

---

## Phase 3: Shader Walk Modes

**Goal**: Implement walk mode logic in compute shader.
**Depends on**: Phase 2
**Files**: `shaders/physarum_agents.glsl`

**Build**:
- Add uniform declarations for new parameters
- Add `bool skipChemotaxis = false;` before the sensor sampling block (~line 189)
- Wrap existing heading adjustment logic (vectorSteering and discrete steering blocks, lines 205-259) in `if (!skipChemotaxis) { ... }`
- Replace Levy-only step calculation (lines 264-274) with mode switch per Technical Implementation section
- Ballistic mode sets `skipChemotaxis = true` for unstuck agents
- Preserve existing `levyAlpha` behavior in mode 1

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Physarum renders. Default mode 0 behaves identically to current fixed-step behavior. Ballistic mode (5) produces fractal tree-like structures.

**Done when**: All 7 walk modes produce distinct visual patterns.

---

## Phase 4: State Reset on Mode Change

**Goal**: Zero per-agent state when walkMode changes.
**Depends on**: Phase 3
**Files**: `src/simulation/physarum.cpp`

**Build**:
- Track previous walkMode in Physarum struct
- In `PhysarumApplyConfig()`: detect walkMode change, download agent buffer, zero `_pad1`-`_pad4` fields, re-upload buffer
- Alternative: add `prevWalkMode` uniform, zero state in shader on first frame after change

**Verify**: Switch from Run & Tumble to Persistent mid-simulation → no visual glitches from stale timer values.

**Done when**: Mode transitions produce clean visual changes without artifacts.

---

## Phase 5: UI Controls

**Goal**: Add walk mode dropdown and conditional parameter sliders.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects.cpp`

**Build**:
- Add walk mode combo/dropdown after "Movement" section header
- Show mode-specific parameters based on selected mode:
  - Mode 1 (Levy): show existing levyAlpha slider
  - Mode 2 (Persistent): show persistence slider
  - Mode 3 (Run & Tumble): show runDuration, tumbleDuration, runMultiplier
  - Mode 4 (Antipersistent): show antiPersistence slider
  - Mode 5 (Ballistic): show stickThreshold slider
  - Mode 6 (Adaptive): show densityResponse slider
- Use `ModulatableSlider` for all numeric parameters

**Verify**: `./build/AudioJones.exe` → Walk Mode dropdown changes visible parameters.

**Done when**: UI shows only parameters relevant to selected mode.

---

## Phase 6: Param Registry

**Goal**: Register modulatable parameters for LFO/audio routing.
**Depends on**: Phase 1
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add bounds to PARAM_TABLE for all 7 new parameters (matching ranges from Technical Implementation)
- Add pointers to targets array in matching order
- Do NOT register walkMode itself (per design decision: not modulatable)

**Verify**: Route LFO to `physarum.persistence` → value oscillates in real-time.

**Done when**: All numeric walk parameters accept modulation.

---

## Phase 7: Preset Serialization

**Goal**: Save and load walk mode settings in presets.
**Depends on**: Phase 1
**Files**: `src/config/preset.cpp`

**Build**:
- Add `walkMode` to PhysarumConfig NLOHMANN macro field list
- Add all 7 new parameters to the macro
- No migration needed: existing presets default to walkMode=0 (Normal)

**Verify**: Save preset with walkMode=3, reload → Run & Tumble mode active with correct parameters.

**Done when**: Presets persist walk mode and all parameters.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1, Phase 5, Phase 6, Phase 7 | — |
| 2 | Phase 2 | Wave 1 |
| 3 | Phase 3 | Wave 2 |
| 4 | Phase 4 | Wave 3 |

**Notes**:
- Phase 1 provides the config struct that Phases 5, 6, 7 depend on
- Phases 5, 6, 7 can run in parallel after Phase 1 (they touch different files)
- Phase 2 requires Phase 1's config changes
- Phase 3 requires Phase 2's uniform bindings
- Phase 4 requires Phase 3's shader to be functional

---

## Post-Implementation Notes

Changes made after testing that extend beyond the original plan:

### Removed: Persistent, Antipersistent, Run & Tumble, Ballistic modes (2026-01-26)

**Reason**: Modes 2-5 manipulated heading rather than step size, producing results inconsistent with their descriptions. Persistent/Antipersistent had broken state initialization. Run & Tumble duplicated LFO modulation capability. Ballistic disabled chemotaxis, violating physarum simulation principles.

**Replaced with**: Pure step-size distribution modes that require no per-agent state:
- **Cauchy** (mode 3): Heavier tails than Levy, uses `tan(PI * (u - 0.5)) * cauchyScale`
- **Exponential** (mode 4): Mostly short steps, occasional long via `-log(u) * expScale`
- **Gaussian** (mode 5): Clustered around mean via `1.0 + gaussian() * gaussianVariance`
- **Sprint** (mode 6): Step scales with heading change via `1.0 + sprintFactor * |headingDelta|`

**Changes**:
- `src/simulation/physarum.h`: Updated enum (NORMAL, LEVY, ADAPTIVE, CAUCHY, EXPONENTIAL, GAUSSIAN, SPRINT), replaced config fields
- `src/simulation/physarum.cpp`: Updated uniform locations and bindings
- `shaders/physarum_agents.glsl`: Removed skipChemotaxis, implemented new distribution modes
- `src/ui/imgui_effects.cpp`: Updated walk mode dropdown and conditional parameters
- `src/automation/param_registry.cpp`: Replaced param bounds and targets
- `src/config/preset.cpp`: Updated serialization macro

### Added: Gradient mode (2026-01-26)

**Reason**: Edge-tracing behavior requested. Step scales with local gradient magnitude - fast at density boundaries, slow in uniform areas.

**Changes**:
- `src/simulation/physarum.h`: Added PHYSARUM_WALK_GRADIENT (mode 7), `gradientBoost` parameter
- `src/simulation/physarum.cpp`: Added uniform location and binding
- `shaders/physarum_agents.glsl`: Samples density here vs ahead, scales step by `1 + gradientBoost * |ahead - here|`
- `src/ui/imgui_effects.cpp`: Added to dropdown and slider
- `src/automation/param_registry.cpp`: Added bounds {0.0, 10.0}
- `src/config/preset.cpp`: Added to serialization
