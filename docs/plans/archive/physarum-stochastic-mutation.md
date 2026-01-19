# Physarum Stochastic Mutation (Pmut)

Replace deterministic steering with probability-based branching from the MCPM paper. Agents use a sampling exponent to control the sharpness of directional decisions: low values create diffuse cloud patterns, high values create sharp filaments.

## Current State

- `shaders/physarum_agents.glsl:169-181` - Discrete steering: always turns toward lowest-affinity sensor
- `src/simulation/physarum.h:19-36` - `PhysarumConfig` struct (no sampling exponent)
- `src/simulation/physarum.cpp:150-194` - `PhysarumUpdate` sets uniforms
- `src/automation/param_registry.cpp:13-17` - Physarum params registered for modulation
- `src/ui/imgui_effects.cpp:195-223` - UI panel

## Technical Implementation

**Source**: [MCPM Paper (MIT Press, 2022)](https://direct.mit.edu/artl/article/28/1/22/109954/Monte-Carlo-Physarum-Machine-Characteristics-of)

**Core algorithm** (discrete steering mode only):

```glsl
// d0 = forward sensor affinity (lower = more attractive)
// d1 = best lateral sensor affinity
// samplingExponent controls decision sharpness

float p0 = pow(d0, samplingExponent);
float p1 = pow(d1, samplingExponent);
float Pmut = p1 / (p0 + p1 + 0.0001);  // Probability of mutation

if (random < Pmut) {
    agent.heading += turnDir * turningAngle;  // Turn toward mutation direction
}
// else: maintain current heading
```

**Behavior by exponent**:
- `samplingExponent = 0`: Skip stochastic logic, use original deterministic steering
- `samplingExponent = 1`: Diffuse clouds, broad coverage (linear probability)
- `samplingExponent = 4`: Balanced branching and structure (default)
- `samplingExponent = 10`: Sharp filaments, near-deterministic

**Parameter**:
| Name | Type | Range | Default | Effect |
|------|------|-------|---------|--------|
| samplingExponent | float | 0.0-10.0 | 4.0 | 0 = disabled. Low = diffuse clouds. High = sharp filaments. |

---

## Phase 1: Config and Shader

**Goal**: Add samplingExponent parameter and implement MCPM steering in shader.

**Build**:

1. `src/simulation/physarum.h` - Add to `PhysarumConfig`:
   ```cpp
   float samplingExponent = 4.0f;  // MCPM mutation probability exponent (0 = disabled)
   ```

2. `src/simulation/physarum.h` - Add to `Physarum` struct:
   ```cpp
   int samplingExponentLoc;
   ```

3. `src/simulation/physarum.cpp` in `LoadComputeProgram` - Get uniform location:
   ```cpp
   p->samplingExponentLoc = rlGetLocationUniform(program, "samplingExponent");
   ```

4. `src/simulation/physarum.cpp` in `PhysarumUpdate` - Upload uniform:
   ```cpp
   rlSetUniform(p->samplingExponentLoc, &p->config.samplingExponent, RL_SHADER_UNIFORM_FLOAT, 1);
   ```

5. `shaders/physarum_agents.glsl` - Add uniform declaration:
   ```glsl
   uniform float samplingExponent;
   ```

6. `shaders/physarum_agents.glsl` - Replace discrete steering block (lines 169-181) with:
   ```glsl
   // Original discrete steering: turn toward lowest affinity
   if (samplingExponent > 0.0) {
       // Stochastic mutation (MCPM): probabilistic choice between forward and mutation
       float d0 = front;  // Forward affinity
       float d1;          // Best lateral affinity
       float turnDir;     // Mutation turn direction

       if (left < right) {
           d1 = left;
           turnDir = 1.0;
       } else {
           d1 = right;
           turnDir = -1.0;
       }

       // MCPM probability formula: Pmut = d1^exp / (d0^exp + d1^exp)
       float p0 = pow(d0, samplingExponent);
       float p1 = pow(d1, samplingExponent);
       float Pmut = p1 / (p0 + p1 + 0.0001);

       if (rnd < Pmut) {
           agent.heading += turnDir * turningAngle;
       }
       // else: no turn (maintain heading)
   } else {
       // Original deterministic steering (samplingExponent = 0)
       if (front < left && front < right) {
           // Front is best, no turn
       } else if (front > left && front > right) {
           // Both sides better: random turn
           agent.heading += (rnd - 0.5) * turningAngle * 2.0;
       } else if (left < right) {
           agent.heading += turningAngle;
       } else if (right < left) {
           agent.heading -= turningAngle;
       }
   }
   ```

**Done when**: Shader compiles. Setting `samplingExponent = 4.0` produces visibly different patterns than the deterministic baseline (`samplingExponent = 0`).

---

## Phase 2: Modulation and UI

**Goal**: Enable audio modulation and add UI slider.

**Build**:

1. `src/automation/param_registry.cpp` - Add to `PARAM_TABLE` after `physarum.repulsionStrength`:
   ```cpp
   {"physarum.samplingExponent", {0.0f, 10.0f}},
   ```

2. `src/automation/param_registry.cpp` - Add to `targets[]` array (same position):
   ```cpp
   &effects->physarum.samplingExponent,
   ```

3. `src/ui/imgui_effects.cpp` - Add slider after "Vector Steering" checkbox (line 218):
   ```cpp
   ModulatableSlider("Sampling Exp", &e->physarum.samplingExponent,
                     "physarum.samplingExponent", "%.1f", modSources);
   ```

**Done when**: Slider appears in Physarum panel. Modulation route can target samplingExponent. Sweeping 1→6 produces breathing between soft and sharp patterns.

---

## Phase 3: Preset Serialization

**Goal**: Persist samplingExponent in presets.

**Build**:

1. `src/config/preset.cpp` - Add to `SerializePhysarum`:
   ```cpp
   j["samplingExponent"] = config.samplingExponent;
   ```

2. `src/config/preset.cpp` - Add to `DeserializePhysarum`:
   ```cpp
   if (j.contains("samplingExponent")) {
       config.samplingExponent = j["samplingExponent"].get<float>();
   }
   ```

**Done when**: Save/load preset preserves samplingExponent value.
