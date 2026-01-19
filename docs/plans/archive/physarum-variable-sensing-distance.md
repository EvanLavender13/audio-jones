# Physarum Variable Sensing Distance (Pdist)

Add Gaussian-distributed sensing distance variation to physarum agents. When `sensorDistanceVariance > 0`, each agent samples its sensing distance from a Gaussian distribution centered on `sensorDistance`. Creates multi-scale feature detection—some agents sense fine detail, others detect large structure.

## Current State

- `shaders/physarum_agents.glsl:141-144` - Fixed `sensorDistance` uniform for all 3 sensors
- `src/simulation/physarum.h:22` - `sensorDistance = 20.0f` in PhysarumConfig
- `src/simulation/physarum.cpp:163` - Uploads sensorDistance uniform
- `src/automation/param_registry.cpp:13` - `physarum.sensorDistance` registered `{1.0f, 100.0f}`
- `src/ui/imgui_effects.cpp:199-200` - "Sensor Dist" slider
- `src/config/preset.cpp:88-91` - PhysarumConfig serialization macro

## Technical Implementation

**Source**: [MCPM Paper (MIT Press, 2022)](https://direct.mit.edu/artl/article/28/1/22/109954/Monte-Carlo-Physarum-Machine-Characteristics-of) - Section 3.2 Variable Distances

**Core algorithm** (Box-Muller transform for Gaussian sampling):

```glsl
// Box-Muller: generate Gaussian from two uniform randoms
// Returns value with mean=0, stddev=1
float gaussian(inout uint state)
{
    float u1 = float(hash(state)) / 4294967295.0;
    state = hash(state);
    float u2 = float(hash(state)) / 4294967295.0;
    state = hash(state);

    // Avoid log(0)
    u1 = max(u1, 1e-6);

    return sqrt(-2.0 * log(u1)) * cos(6.28318530718 * u2);
}
```

**Sensor distance sampling**:

```glsl
// Sample distance from Gaussian centered on sensorDistance
float sampleSensorDistance(float variance, inout uint hashState)
{
    if (variance < 0.001) {
        return sensorDistance;  // No variance, use base value
    }

    float offset = gaussian(hashState) * variance;
    float dist = sensorDistance + offset;

    // Clamp to valid range: minimum 1.0 pixel, maximum 2x base
    return clamp(dist, 1.0, sensorDistance * 2.0);
}
```

**Parameter**:
| Name | Type | Range | Default | Effect |
|------|------|-------|---------|--------|
| sensorDistanceVariance | float | 0.0-20.0 | 0.0 | Gaussian stddev. 0 = uniform behavior. Higher = more scale diversity. |

**Behavior by variance**:
- `0.0`: All agents sense at exactly `sensorDistance` (current behavior)
- `5.0`: 68% of agents sense within ±5px of base, 95% within ±10px
- `15.0`: Wide spread—agents detect at very different scales simultaneously

---

## Phase 1: Config and Shader

**Goal**: Add sensorDistanceVariance parameter and implement Gaussian sampling in shader.

**Build**:

1. `src/simulation/physarum.h` - Add to `PhysarumConfig` after `sensorDistance`:
   ```cpp
   float sensorDistanceVariance = 0.0f;  // Gaussian stddev for sensing distance (0 = uniform)
   ```

2. `src/simulation/physarum.h` - Add to `Physarum` struct after `sensorDistanceLoc`:
   ```cpp
   int sensorDistanceVarianceLoc;
   ```

3. `src/simulation/physarum.cpp` in `LoadComputeProgram` after sensorDistanceLoc:
   ```cpp
   p->sensorDistanceVarianceLoc = rlGetLocationUniform(program, "sensorDistanceVariance");
   ```

4. `src/simulation/physarum.cpp` in `PhysarumUpdate` after sensorDistance uniform:
   ```cpp
   rlSetUniform(p->sensorDistanceVarianceLoc, &p->config.sensorDistanceVariance, RL_SHADER_UNIFORM_FLOAT, 1);
   ```

5. `shaders/physarum_agents.glsl` - Add uniform after `sensorDistance`:
   ```glsl
   uniform float sensorDistanceVariance;
   ```

6. `shaders/physarum_agents.glsl` - Add Box-Muller function after existing `hash` function:
   ```glsl
   // Box-Muller: generate Gaussian from two uniform randoms (mean=0, stddev=1)
   float gaussian(inout uint state)
   {
       float u1 = float(hash(state)) / 4294967295.0;
       state = hash(state);
       float u2 = float(hash(state)) / 4294967295.0;
       state = hash(state);
       u1 = max(u1, 1e-6);
       return sqrt(-2.0 * log(u1)) * cos(6.28318530718 * u2);
   }
   ```

7. `shaders/physarum_agents.glsl` - In `main()`, replace the 3 sensor position calculations (lines 141-144) with:
   ```glsl
   // Sample per-agent sensing distance from Gaussian distribution
   float agentSensorDist = sensorDistance;
   if (sensorDistanceVariance > 0.001) {
       float offset = gaussian(hashState) * sensorDistanceVariance;
       agentSensorDist = clamp(sensorDistance + offset, 1.0, sensorDistance * 2.0);
   }

   vec2 frontPos = pos + frontDir * agentSensorDist;
   vec2 leftPos = pos + leftDir * agentSensorDist;
   vec2 rightPos = pos + rightDir * agentSensorDist;
   ```

**Done when**: Shader compiles. Setting `sensorDistanceVariance = 10.0` produces visible multi-scale behavior—some agents form tight local structures, others create longer-range connections.

---

## Phase 2: Modulation and UI

**Goal**: Enable audio modulation and add UI slider.

**Build**:

1. `src/automation/param_registry.cpp` - Add to `PARAM_TABLE` after `physarum.sensorDistance`:
   ```cpp
   {"physarum.sensorDistanceVariance", {0.0f, 20.0f}},
   ```

2. `src/automation/param_registry.cpp` - Add to `targets[]` array (same position):
   ```cpp
   &effects->physarum.sensorDistanceVariance,
   ```

3. `src/ui/imgui_effects.cpp` - Add slider after "Sensor Dist" (line 200):
   ```cpp
   ModulatableSlider("Sensor Variance", &e->physarum.sensorDistanceVariance,
                     "physarum.sensorDistanceVariance", "%.1f px", modSources);
   ```

**Done when**: Slider appears in Physarum panel below "Sensor Dist". Modulation route can target sensorDistanceVariance. Audio modulation creates breathing between uniform and varied sensing.

---

## Phase 3: Preset Serialization

**Goal**: Persist sensorDistanceVariance in presets.

**Build**:

1. `src/config/preset.cpp` - Add `sensorDistanceVariance` to the PhysarumConfig macro (line 88-91):
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysarumConfig,
       enabled, agentCount, sensorDistance, sensorDistanceVariance, sensorAngle, turningAngle,
       stepSize, depositAmount, decayHalfLife, diffusionScale, boostIntensity,
       blendMode, accumSenseBlend, repulsionStrength, samplingExponent, vectorSteering, color, debugOverlay)
   ```

**Done when**: Save/load preset preserves sensorDistanceVariance value.
