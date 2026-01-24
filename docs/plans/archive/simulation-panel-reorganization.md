# Simulation Panel Reorganization

Reorganize the 6 simulation panels from flat parameter lists into semantically grouped sections using `SeparatorText` headers. Start with Physarum as the reference implementation, then apply the template to the remaining simulations.

## Current State

All simulation UI lives in `src/ui/imgui_effects.cpp:217-445`. Each simulation dumps 10-20 parameters in a flat list with no visual grouping. The shared tail (Deposit/Decay/Diffusion/Boost/Blend/Color/Debug) repeats across all simulations without demarcation.

- `src/ui/imgui_effects.cpp:217-266` - Physarum panel (flat)
- `src/ui/imgui_effects.cpp:271-293` - Curl Flow panel
- `src/ui/imgui_effects.cpp:298-343` - Attractor Flow panel
- `src/ui/imgui_effects.cpp:348-379` - Boids panel
- `src/ui/imgui_effects.cpp:384-411` - Curl Advection panel
- `src/ui/imgui_effects.cpp:416-445` - Cymatics panel
- `src/simulation/curl_advection.h:12-30` - CurlAdvectionConfig (missing decay/diffusion fields)
- `src/simulation/curl_advection.cpp:249` - Hardcoded TrailMapProcess(0.5f, 0)

## Design

### Universal Template

Every simulation follows this skeleton:

```
[Enabled]
Agents: [N]                            <- top-level (only when applicable)

── Bounds ──────────────────────       <- SeparatorText (only when simulation has bounds mode)
   Bounds Mode / conditional params

── <Behavior Group 1> ──────────       <- SeparatorText, simulation-specific
   (unique params)

── <Behavior Group 2> ──────────       <- optional additional behavior groups
   (unique params)

── Trail ───────────────────────       <- SeparatorText, shared
   Deposit / Decay / Diffusion

── Output ──────────────────────       <- SeparatorText, shared
   Boost / Blend Mode / Color
   [Debug]
```

- **Enabled** and **Agents** are the only ungrouped top-level controls (one checkbox, one slider — no separator needed)
- **Bounds** appears only for simulations with spatial containment modes (Physarum, Boids). Groups the mode selector and all conditional params it reveals.
- **Behavior groups** use descriptive names per simulation (not a generic "Behavior" label)
- **Trail** and **Output** use consistent names across all simulations
- **Gravity** (Physarum) moves into Movement — it directly affects agent trajectories
- Use `ImGui::SeparatorText()` — non-collapsible, zero interaction cost
- Do NOT use `TreeNodeAccented` for these groups

### Physarum Layout

```
[Enabled]
Agents: [100000]

── Bounds ───────────────────
Bounds Mode: [Toroidal]
  Respawn             (conditional: Redirect/Multi-Home)
  Attractors          (conditional: Multi-Home)
  Orbit Offset        (conditional: Species Orbit, modulatable)

── Sensing ──────────────────
Sensor Dist          (modulatable)
Sensor Variance      (modulatable)
Sensor Angle         (modulatable, degrees)
Turn Angle           (modulatable, degrees)
Sense Blend

── Movement ─────────────────
Step Size            (modulatable)
Levy Alpha           (modulatable)
Gravity              (modulatable)
[Vector Steering]

── Species ──────────────────
Repulsion            (modulatable)
Sampling Exp         (modulatable)

── Trail ────────────────────
Deposit
Decay
Diffusion

── Output ───────────────────
Boost
Blend Mode
Color
[Debug]
```

**Parameter moves from current order:**
- `Bounds Mode` + conditionals (Respawn, Attractors, Orbit Offset) grouped under Bounds
- `Gravity` moves into Movement (directly affects agent trajectories)
- `Sense Blend` moves up into Sensing (controls what agents sense: trail vs accumulator)
- `Vector Steering` moves up into Movement (changes how turning works)
- `Repulsion` and `Sampling Exp` grouped as Species (only relevant for multi-hue)

### Curl Flow Layout

```
[Enabled]
Agents

── Field ────────────────────
Frequency
Evolution
Momentum

── Sensing ──────────────────
Density Influence
Sense Blend
Gradient Radius

── Movement ─────────────────
Step Size

── Trail ────────────────────
Deposit / Decay / Diffusion

── Output ───────────────────
Boost / Blend Mode / Color / [Debug]
```

### Attractor Flow Layout

```
[Enabled]
Agents

── Attractor ────────────────
Type: [Lorenz/Rossler/Aizawa/Thomas]
Time Scale
Scale
(type-specific: Sigma/Rho/Beta, C, B)

── Projection ───────────────
X / Y
Angle X/Y/Z         (modulatable, degrees)
Spin X/Y/Z          (modulatable, degrees/s)

── Trail ────────────────────
Deposit / Decay / Diffusion

── Output ───────────────────
Boost / Blend Mode / Color / [Debug]
```

### Boids Layout

```
[Enabled]
Agents

── Bounds ───────────────────
Bounds Mode: [Wrap/Contain]

── Flocking ─────────────────
Perception
Separation Radius
Cohesion             (modulatable)
Separation Wt        (modulatable)
Alignment            (modulatable)

── Species ──────────────────
Hue Affinity
Accum Repulsion

── Movement ─────────────────
Max Speed
Min Speed

── Trail ────────────────────
Deposit / Decay / Diffusion

── Output ───────────────────
Boost / Blend Mode / Color / [Debug]
```

### Curl Advection Layout

```
[Enabled]

── Field ────────────────────
Steps
Advection Curl       (modulatable)
Curl Scale           (modulatable)
Self Amp             (modulatable)

── Pressure ─────────────────
Laplacian
Pressure
Div Scale
Div Update
Div Smooth
Update Smooth

── Injection ────────────────
Injection            (modulatable)
Inj Threshold

── Trail ────────────────────
Decay
Diffusion

── Output ───────────────────
Boost / Blend Mode / Color / [Debug]
```

Note: Curl Advection currently hardcodes decay (0.5s) and diffusion (0) in `curl_advection.cpp:249`. Add `decayHalfLife` and `diffusionScale` fields to `CurlAdvectionConfig` and pass them to `TrailMapProcess` instead of literals.

### Cymatics Layout

```
[Enabled]

── Wave ─────────────────────
Wave Scale           (modulatable)
Falloff              (modulatable)
Gain                 (modulatable)
Contours

── Sources ──────────────────
Source Count
Base Radius          (modulatable)
Pattern Angle        (modulatable, degrees)
Source Amplitude
Source Freq X
Source Freq Y

── Trail ────────────────────
Decay
Diffusion

── Output ───────────────────
Boost / Blend Mode / Color / [Debug]
```

Note: Cymatics Trail section omits Deposit (not applicable).

## Phase 1: Physarum Panel

**Goal**: Reorganize Physarum panel as the reference implementation.
**Depends on**: —
**Files**: `src/ui/imgui_effects.cpp`

**Build**:
- Modify Physarum section (lines 217-266) in `src/ui/imgui_effects.cpp`
- Only `Enabled` checkbox and `Agents` slider remain ungrouped at top
- Add `ImGui::SeparatorText()` calls for Bounds, Sensing, Movement, Species, Trail, Output
- Move `Bounds Mode` + conditional params (Respawn, Attractors, Orbit Offset) under the Bounds separator
- Move `Gravity` into Movement (affects agent trajectories)
- Reorder remaining parameters to match the Physarum Layout in the Design section above
- No new files, no config changes, no behavior changes

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Physarum panel shows 6 labeled groups with parameters in specified order.

**Done when**: Physarum panel displays Bounds/Sensing/Movement/Species/Trail/Output separator groups.

---

## Phase 2: Curl Advection Backend

**Goal**: Add configurable decay/diffusion fields to CurlAdvectionConfig, replacing hardcoded values.
**Depends on**: —
**Files**: `src/simulation/curl_advection.h`, `src/simulation/curl_advection.cpp`

**Build**:
- Add `float decayHalfLife = 0.5f;` and `int diffusionScale = 0;` fields to `CurlAdvectionConfig` in `src/simulation/curl_advection.h`
- Replace hardcoded `TrailMapProcess(ca->trailMap, deltaTime, 0.5f, 0)` in `src/simulation/curl_advection.cpp:249` with `TrailMapProcess(ca->trailMap, deltaTime, ca->config.decayHalfLife, ca->config.diffusionScale)`

**Verify**: `cmake.exe --build build` → compiles without errors. Existing behavior unchanged (defaults match previous hardcoded values).

**Done when**: CurlAdvectionConfig exposes decay and diffusion as configurable fields.

---

## Phase 3: Remaining Simulation Panels

**Goal**: Apply SeparatorText grouping to Curl Flow, Attractor Flow, Boids, Curl Advection, and Cymatics panels.
**Depends on**: Phase 1, Phase 2
**Files**: `src/ui/imgui_effects.cpp`

**Build**:
- Apply the same `ImGui::SeparatorText()` pattern established in Phase 1 to each remaining simulation
- Reorder parameters within each section to match the layouts in the Design section above
- Preserve all existing widget types, `##id` suffixes, and parameter ranges
- For Curl Advection: add `Decay` and `Diffusion` sliders in the Trail group (using the new config fields from Phase 2)
- For Cymatics: Trail group contains only Decay and Diffusion (no Deposit)
- Sections to modify:
  - Curl Flow (lines 271-293): Field / Sensing / Movement / Trail / Output
  - Attractor Flow (lines 298-343): Attractor / Projection / Trail / Output
  - Boids (lines 348-379): Bounds / Flocking / Species / Movement / Trail / Output
  - Curl Advection (lines 384-411): Field / Pressure / Injection / Trail / Output
  - Cymatics (lines 416-445): Wave / Sources / Trail / Output

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → all 6 simulation panels display grouped parameters with consistent Trail/Output sections.

**Done when**: All 6 simulation panels use SeparatorText grouping matching the Design layouts.
