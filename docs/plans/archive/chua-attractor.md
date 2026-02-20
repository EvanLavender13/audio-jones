# Chua Attractor

Add Chua's double-scroll attractor as a new attractor type available in both Attractor Flow (compute simulation) and Attractor Lines (fragment generator). The Chua system features two spiral lobes connected by chaotic transitions — particles wind tightly in one lobe then jump unpredictably to the other. Extends existing attractor infrastructure with no new pipeline stages.

**Research**: `docs/research/chua-attractor.md`

## Design

### Types

**Enum** (in `attractor_types.h`):
```
ATTRACTOR_CHUA  // added before ATTRACTOR_COUNT
```

**Config fields** (added to both `AttractorLinesConfig` and `AttractorFlowConfig`):
```
float chuaAlpha = 15.6f;   // Primary chaos control (5.0-30.0)
float chuaGamma = 25.58f;  // Y-axis coupling (10.0-40.0)
float chuaM0 = -2.0f;      // Inner diode slope (-3.0-0.0)
float chuaM1 = 0.0f;       // Outer diode slope (-1.0-1.0)
```

**Uniform locations** (added to both `AttractorLinesEffect` and `AttractorFlow`):
```
int chuaAlphaLoc;
int chuaGammaLoc;
int chuaM0Loc;
int chuaM1Loc;
```

No modulation registration for Chua params — user decision. The research doc recommends modulation for alpha/gamma with interaction patterns, but chaotic system params are fragile and modulation risks divergence into non-chaotic or unstable regimes. <!-- Intentional deviation from research doc modulation recommendations -->

### Algorithm

Both shaders get identical derivative functions:

```glsl
// Piecewise-linear Chua diode characteristic
float chuaDiode(float x) {
    return chuaM1 * x + 0.5 * (chuaM0 - chuaM1) * (abs(x + 1.0) - abs(x - 1.0));
}

vec3 chuaDerivative(vec3 p) {
    float g = chuaDiode(p.x);
    return vec3(
        chuaAlpha * (p.y - p.x - g),
        p.x - p.y + p.z,
        -chuaGamma * p.y
    );
}
```

Wire into `attractorDerivative()` dispatch as type index 5.

**Tuning constants** (attractor_lines.fs only):
```glsl
const float STEP_CHUA    = 0.008;  // Similar to Lorenz — moderately stiff
const float SCALE_CHUA   = 3.0;    // Attractor spans ~±3 in x, needs scaling up
const float DIVERGE_CHUA = 50.0;   // Compact basin, ~12 units radius
```

**Projection**: XZ plane (like Lorenz — shows both scroll lobes side by side).

**Center offset**: `vec3(0.0)` — Chua is centered at origin.

**Seeding** (both systems): Split 50/50 into each scroll lobe at x ≈ ±1.5, small random offset (r ≈ 0.1) around lobe centers. On divergence, respawn into random lobe.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| chuaAlpha | float | 5.0-30.0 | 15.6 | No | Alpha |
| chuaGamma | float | 10.0-40.0 | 25.58 | No | Gamma |
| chuaM0 | float | -3.0-0.0 | -2.0 | No | M0 |
| chuaM1 | float | -1.0-1.0 | 0.0 | No | M1 |

UI labels: `"Alpha##chua"`, `"Gamma##chua"`, `"M0##chua"`, `"M1##chua"` (for attractor lines).
`"Alpha##chuaAttr"`, `"Gamma##chuaAttr"`, `"M0##chuaAttr"`, `"M1##chuaAttr"` (for attractor flow).

### Constants

- Enum value: `ATTRACTOR_CHUA`
- Attractor type index: 5 (next after `ATTRACTOR_DADRAS = 4`)
- Display name in combo: `"Chua"`

---

## Tasks

### Wave 1: Shared Types

#### Task 1.1: Add Chua enum and config fields

**Files**: `src/config/attractor_types.h`, `src/effects/attractor_lines.h`, `src/simulation/attractor_flow.h`
**Creates**: `ATTRACTOR_CHUA` enum value, 4 config fields in both config structs, 4 uniform location ints in both effect structs

**Do**:
1. `attractor_types.h`: Add `ATTRACTOR_CHUA,` before `ATTRACTOR_COUNT`
2. `attractor_lines.h` — `AttractorLinesConfig`: Add 4 fields after `dadrasE`:
   - `float chuaAlpha = 15.6f;   // Chua primary chaos (5.0-30.0)`
   - `float chuaGamma = 25.58f;  // Chua y-coupling (10.0-40.0)`
   - `float chuaM0 = -2.0f;      // Chua inner diode slope (-3.0-0.0)`
   - `float chuaM1 = 0.0f;       // Chua outer diode slope (-1.0-1.0)`
3. `attractor_lines.h` — `ATTRACTOR_LINES_CONFIG_FIELDS`: Add `chuaAlpha, chuaGamma, chuaM0, chuaM1` after `dadrasE`
4. `attractor_lines.h` — `AttractorLinesEffect`: Add 4 ints after `dadrasELoc`:
   - `int chuaAlphaLoc;`
   - `int chuaGammaLoc;`
   - `int chuaM0Loc;`
   - `int chuaM1Loc;`
5. `attractor_flow.h` — `AttractorFlowConfig`: Add same 4 config fields after `dadrasE`
6. `attractor_flow.h` — `ATTRACTOR_FLOW_CONFIG_FIELDS`: Add `chuaAlpha, chuaGamma, chuaM0, chuaM1` after `dadrasE`
7. `attractor_flow.h` — `AttractorFlow`: Add 4 uniform location ints after `dadrasELoc`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks in this wave touch different files with no overlap.

#### Task 2.1: Attractor Lines shader — add Chua derivative and dispatch

**Files**: `shaders/attractor_lines.fs`
**Depends on**: Wave 1 complete (enum value used as int constant)

**Do**:
1. Add `uniform float chuaAlpha;`, `uniform float chuaGamma;`, `uniform float chuaM0;`, `uniform float chuaM1;` after the Dadras uniforms
2. Add tuning constants after DADRAS constants:
   ```
   const float STEP_CHUA    = 0.008;
   const float SCALE_CHUA   = 3.0;
   const float DIVERGE_CHUA = 50.0;
   ```
3. Add Chua case (type 5) to `getStepSize()`, `getScaleFactor()`, `getDivergeLimit()` — add `if (type == 5) return *_CHUA;` before the default return
4. Add `chuaDiode()` and `derivativeChua()` functions after `derivativeDadras()`. Implement the GLSL from the plan's Design > Algorithm section above.
5. Add Chua case in `attractorDerivative()`: `if (type == 5) return derivativeChua(p);` before the Dadras default
6. Add Chua case in `getStartingPoint()`: type 5 seeds into ±1.5 lobes — `base = vec3(sign * 1.5, 0.0, 0.0); r = 0.1;` where `sign` alternates by `pid` (even=+1, odd=-1)
7. Add Chua case in `project()`: XZ plane like Lorenz — `if (type == 5) return vec2(p.x, p.z) * s;`

**Verify**: Shader syntax review (no build needed for shaders).

#### Task 2.2: Attractor agents compute shader — add Chua derivative and dispatch

**Files**: `shaders/attractor_agents.glsl`
**Depends on**: Wave 1 complete

**Do**:
1. Add `uniform float chuaAlpha;`, `uniform float chuaGamma;`, `uniform float chuaM0;`, `uniform float chuaM1;` after the Dadras uniforms
2. Add `chuaDiode()` and `chuaDerivative()` functions after `dadrasDerivative()`. Implement the GLSL from the plan's Design > Algorithm section above.
3. Add Chua case in `attractorDerivative()`: `} else if (attractorType == 5) { return chuaDerivative(p); }` before the default Lorenz return
4. Add Chua case in `projectToScreen()`:
   - Center offset: `centered = p;` (no offset — Chua centered at origin)
   - Projection: XZ plane with scale 3.0 — `projected = vec2(rotated.x * 3.0, rotated.z * 3.0);`
5. Add Chua case in `respawnAgent()`: Split 50/50 into lobes — `float sign = hashFloat(seed) < 0.5 ? 1.0 : -1.0; agent.x = sign * 1.5 + (hashFloat(seed+1u) - 0.5) * 0.2; agent.y = (hashFloat(seed+2u) - 0.5) * 0.2; agent.z = (hashFloat(seed+3u) - 0.5) * 0.2;`

**Verify**: Shader syntax review (no build needed for shaders).

#### Task 2.3: Attractor Lines C++ — uniform binding

**Files**: `src/effects/attractor_lines.cpp`
**Depends on**: Wave 1 complete (struct has new fields)

**Do**:
1. `CacheLocations()`: Add 4 lines after `dadrasELoc`:
   ```
   e->chuaAlphaLoc = GetShaderLocation(e->shader, "chuaAlpha");
   e->chuaGammaLoc = GetShaderLocation(e->shader, "chuaGamma");
   e->chuaM0Loc = GetShaderLocation(e->shader, "chuaM0");
   e->chuaM1Loc = GetShaderLocation(e->shader, "chuaM1");
   ```
2. `BindScalarUniforms()`: Add 4 `SetShaderValue` calls after the Dadras block, same pattern as existing attractor params.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Attractor Flow C++ — uniform binding and seeding

**Files**: `src/simulation/attractor_flow.cpp`
**Depends on**: Wave 1 complete (struct has new fields)

**Do**:
1. `LoadComputeProgram()`: Add 4 `rlGetLocationUniform` calls after `dadrasELoc`:
   ```
   af->chuaAlphaLoc = rlGetLocationUniform(program, "chuaAlpha");
   af->chuaGammaLoc = rlGetLocationUniform(program, "chuaGamma");
   af->chuaM0Loc = rlGetLocationUniform(program, "chuaM0");
   af->chuaM1Loc = rlGetLocationUniform(program, "chuaM1");
   ```
2. `AttractorFlowUpdate()`: Add 4 `rlSetUniform` calls after the Dadras block.
3. `InitializeAgents()`: Add `case ATTRACTOR_CHUA:` — seed into ±1.5 lobes with small random offset (±0.1). Pattern: `sign * 1.5f + random(±0.1)` for x, `random(±0.1)` for y and z.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Attractor Lines UI — add Chua to combo and param sliders

**Files**: `src/ui/imgui_effects_gen_filament.cpp`
**Depends on**: Wave 1 complete (config fields exist)

**Do**:
1. Update `attractorNames[]` array in `DrawGeneratorsAttractorLines()`: add `"Chua"` as 6th entry
2. Add `else if (c->attractorType == ATTRACTOR_CHUA)` block in `DrawAttractorSystemParams()` with 4 `ImGui::SliderFloat` calls:
   - `"Alpha##chua"`, range 5.0-30.0, format `"%.1f"`
   - `"Gamma##chua"`, range 10.0-40.0, format `"%.2f"`
   - `"M0##chua"`, range -3.0-0.0, format `"%.1f"`
   - `"M1##chua"`, range -1.0-1.0, format `"%.2f"`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Attractor Flow UI — add Chua to combo and param sliders

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1 complete (config fields exist)

**Do**:
1. Find the `attractorNames[]` array in the attractor flow section and add `"Chua"` as 6th entry
2. Add `else if (e->attractorFlow.attractorType == ATTRACTOR_CHUA)` block after the Dadras block with 4 `ImGui::SliderFloat` calls:
   - `"Alpha##chuaAttr"`, range 5.0-30.0, format `"%.1f"`
   - `"Gamma##chuaAttr"`, range 10.0-40.0, format `"%.2f"`
   - `"M0##chuaAttr"`, range -3.0-0.0, format `"%.1f"`
   - `"M1##chuaAttr"`, range -1.0-1.0, format `"%.2f"`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] "Chua" appears in attractor type combo for both Attractor Lines and Attractor Flow
- [ ] Selecting Chua shows Alpha/Gamma/M0/M1 sliders in both UIs
- [ ] Attractor Lines draws recognizable double-scroll shape
- [ ] Attractor Flow particles trace double-scroll trajectories
- [ ] Changing alpha/gamma produces visible behavior change
- [ ] Preset save/load round-trips Chua params correctly (automatic via CONFIG_FIELDS macro)
- [ ] Switching away from Chua and back resets trails correctly
