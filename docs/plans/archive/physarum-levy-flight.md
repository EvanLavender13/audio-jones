# Physarum Lévy Flight Step Lengths

Add power-law distributed step lengths to physarum agents. When `levyAlpha > 0`, agents draw step lengths from a Pareto distribution—many short steps with occasional long jumps. Creates exploratory foraging patterns optimal for sparse target detection.

## Current State

- `shaders/physarum_agents.glsl:233` - Fixed `stepSize` uniform: `pos += moveDir * stepSize`
- `src/simulation/physarum.h:26` - `stepSize = 1.5f` in PhysarumConfig
- `src/simulation/physarum.cpp:55,168` - `stepSizeLoc` uniform upload
- `src/automation/param_registry.cpp:17` - `physarum.stepSize` registered `{0.1f, 100.0f}`
- `src/ui/imgui_effects.cpp:207-208` - "Step Size" slider
- `src/config/preset.cpp:90` - PhysarumConfig serialization includes stepSize

## Technical Implementation

**Source**: [MCPM Paper (MIT Press, 2022)](https://direct.mit.edu/artl/article/28/1/22/109954/Monte-Carlo-Physarum-Machine-Characteristics-of) - Section 3.2, [Lévy Flight Foraging (PLOS)](https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1009490)

**Core algorithm** (inverse CDF sampling from Pareto/power-law):
```glsl
// Lévy flight step length from power-law distribution
// CDF: F(t) = 1 - (t/t0)^(-alpha) for t >= t0
// Inverse CDF: t = t0 * u^(-1/alpha) where u ~ Uniform(0,1)

float levyStep(float minStep, float maxStep, float alpha, inout uint hashState)
{
    if (alpha < 0.001) {
        return minStep;  // alpha=0: fixed step (backward compatible)
    }

    float u = float(hash(hashState)) / 4294967295.0;
    hashState = hash(hashState);
    u = max(u, 0.001);  // Avoid division by zero

    float step = minStep * pow(u, -1.0 / alpha);
    return min(step, maxStep);  // Truncate extreme jumps
}
```

**Parameter**:
| Name | Type | Range | Default | Effect |
|------|------|-------|---------|--------|
| levyAlpha | float | 0.0-3.0 | 0.0 | Power-law exponent. 0 = fixed step. 1 = extreme jumps. 2 = optimal foraging. 3 = near-Brownian. |

**Behavior by alpha**:
- `0.0`: All agents step exactly `stepSize` pixels (current behavior)
- `1.0`: Heavy tail—frequent long jumps, sparse exploratory patterns
- `2.0`: Optimal foraging—balanced exploration/exploitation (inverse-square law)
- `3.0`: Near-Brownian—mostly short steps, dense localized patterns

**Max step truncation**: Fixed at `stepSize * 50` to prevent screen-spanning jumps while preserving scale-free character.

---

## Phase 1: Config and Shader

**Goal**: Add levyAlpha parameter and implement power-law step sampling in shader.

**Build**:

1. `src/simulation/physarum.h` - Add to `PhysarumConfig` after `stepSize`:
   - `float levyAlpha = 0.0f;`  (power-law exponent, 0 = fixed step)

2. `src/simulation/physarum.h` - Add to `Physarum` struct after `stepSizeLoc`:
   - `int levyAlphaLoc;`

3. `src/simulation/physarum.cpp` in `LoadComputeProgram` after stepSizeLoc:
   - Get uniform location for "levyAlpha"

4. `src/simulation/physarum.cpp` in `PhysarumUpdate` after stepSize uniform:
   - Upload levyAlpha uniform

5. `shaders/physarum_agents.glsl` - Add uniform after `stepSize`:
   - `uniform float levyAlpha;`

6. `shaders/physarum_agents.glsl` - In `main()`, replace line 233 (`pos += moveDir * stepSize;`) with:
   ```glsl
   // Lévy flight step length
   float agentStep = stepSize;
   if (levyAlpha > 0.001) {
       float u = float(hash(hashState)) / 4294967295.0;
       hashState = hash(hashState);
       u = max(u, 0.001);
       agentStep = stepSize * pow(u, -1.0 / levyAlpha);
       agentStep = min(agentStep, stepSize * 50.0);  // Truncate extreme jumps
   }
   pos += moveDir * agentStep;
   ```

**Done when**: Shader compiles. Setting `levyAlpha = 2.0` produces visible multi-scale movement—most agents take small steps, occasional agents make long jumps creating extended filaments.

---

## Phase 2: Modulation and UI

**Goal**: Enable audio modulation and add UI slider.

**Build**:

1. `src/automation/param_registry.cpp` - Add to `PARAM_TABLE` after `physarum.stepSize`:
   - `{"physarum.levyAlpha", {0.0f, 3.0f}},`

2. `src/automation/param_registry.cpp` - Add to `targets[]` array (same position after stepSize):
   - `&effects->physarum.levyAlpha,`

3. `src/ui/imgui_effects.cpp` - Add slider after "Step Size" (line 208):
   - ModulatableSlider for levyAlpha with format "%.1f"

**Done when**: Slider appears in Physarum panel below "Step Size". Modulation route can target levyAlpha. Audio modulation on bass creates exploratory bursts (low alpha) that tighten on beats (high alpha).

---

## Phase 3: Preset Serialization

**Goal**: Persist levyAlpha in presets.

**Build**:

1. `src/config/preset.cpp` - Add `levyAlpha` to PhysarumConfig macro (line 88-91), after `stepSize`

**Done when**: Save/load preset preserves levyAlpha value.
