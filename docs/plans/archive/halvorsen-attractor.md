# Halvorsen Attractor

Add Halvorsen as the 6th attractor type in both Attractor Lines (generator) and Attractor Flow (simulation). Three-fold symmetric chaotic attractor with a triangular-lobed shape — visually distinct from the butterfly (Lorenz), spiral (Rossler), toroidal (Aizawa), three-lobed (Thomas), and chaotic web (Dadras) forms already in the system. Also backfills the missing Dadras support in Attractor Flow.

**Research**: `docs/research/halvorsen-attractor.md`

## Design

### Types

**Enum** — add to `attractor_types.h`:
```
ATTRACTOR_HALVORSEN  // value 5, before ATTRACTOR_COUNT
```

**AttractorLinesConfig** — add field:
```
float halvorsenA = 1.89f;  // Dissipation (1.0-3.0)
```

**AttractorFlowConfig** — add fields:
```
float dadrasA = 3.0f;    // Dadras a (1-5)
float dadrasB = 2.7f;    // Dadras b (1-5)
float dadrasC = 1.7f;    // Dadras c (0.5-3)
float dadrasD = 2.0f;    // Dadras d (0.5-4)
float dadrasE = 9.0f;    // Dadras e (4-15)
float halvorsenA = 1.89f; // Halvorsen dissipation (1.0-3.0)
```

**AttractorLinesEffect** — add uniform location:
```
int halvorsenALoc;
```

**AttractorFlow** — add uniform locations:
```
int dadrasALoc;
int dadrasBLoc;
int dadrasCLoc;
int dadrasDLoc;
int dadrasELoc;
int halvorsenALoc;
```

### Algorithm

**Halvorsen derivative** (identical in both shaders):
```glsl
// dx/dt = -A*x - 4*y - 4*z - y^2
// dy/dt = -A*y - 4*z - 4*x - z^2
// dz/dt = -A*z - 4*x - 4*y - x^2
vec3 derivativeHalvorsen(vec3 p) {
    return vec3(
        -halvorsenA * p.x - 4.0 * p.y - 4.0 * p.z - p.y * p.y,
        -halvorsenA * p.y - 4.0 * p.z - 4.0 * p.x - p.z * p.z,
        -halvorsenA * p.z - 4.0 * p.x - 4.0 * p.y - p.x * p.x
    );
}
```

**Dadras derivative** (for compute shader backfill — copy from `attractor_lines.fs`):
```glsl
vec3 dadrasDerivative(vec3 p) {
    return vec3(
        p.y - dadrasA * p.x + dadrasB * p.y * p.z,
        dadrasC * p.y - p.x * p.z + p.z,
        dadrasD * p.x * p.y - dadrasE * p.z
    );
}
```

**Per-attractor tuning constants** (fragment shader only):
```
STEP_HALVORSEN  = 0.008   // Moderate dynamics, similar speed to Lorenz
SCALE_HALVORSEN = 2.5     // Ranges ±6, needs scaling to match other attractors' screen fill
DIVERGE_HALVORSEN = 20.0  // Per research doc
```

**Starting point** (both shaders):
- `base = vec3(0.5, 0.5, 0.5)` with `r = 1.5` spread (similar to Thomas's 3-fold structure)

**Center offset**: `vec3(0.0)` (already centered at origin)

**Projection plane**: XY (same as Rossler/Thomas/Dadras)

**Scale in compute shader `projectToScreen()`**: `4.0` (similar to Thomas, both have 3-fold symmetry)

**Respawn in compute shader**: Small random position ±1.0 on each axis (same as Thomas)

**Dadras in compute shader** — copy from Attractor Lines patterns:
- Starting point: `base = vec3(1.0, 1.0, 1.0)` with `r = 1.5`
- Projection plane: XY, scale factor 2.7
- Respawn: ±1.5 random on each axis

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| halvorsenA (Lines) | float | 1.0 - 3.0 | 1.89 | Yes | `"Halvorsen A##attractorLines"` |
| halvorsenA (Flow) | float | 1.0 - 3.0 | 1.89 | No (Flow uses plain SliderFloat) | `"Halvorsen A##attr"` |
| dadrasA-E (Flow) | float | same as Lines | same as Lines | No | `"Dadras A"` through `"Dadras E"` |

### Constants

No new enum values in `TransformEffectType` or `effect_config.h`. The `AttractorType` enum gets one new entry. No new descriptors needed — both effects already have their descriptor rows.

---

## Tasks

### Wave 1: Shared Type Enum

#### Task 1.1: Add ATTRACTOR_HALVORSEN to enum

**Files**: `src/config/attractor_types.h`
**Creates**: The `ATTRACTOR_HALVORSEN` enum value that all other files reference

**Do**: Add `ATTRACTOR_HALVORSEN` before `ATTRACTOR_COUNT`. Value will be 5.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Config + Shader + C++ (all parallel — no file overlap)

#### Task 2.1: Attractor Lines config + C++

**Files**: `src/effects/attractor_lines.h`, `src/effects/attractor_lines.cpp`
**Depends on**: Wave 1

**Do**:
- **Header**: Add `float halvorsenA = 1.89f; // Dissipation (1.0-3.0)` to `AttractorLinesConfig`. Add `int halvorsenALoc;` to `AttractorLinesEffect`. Add `halvorsenA` to `ATTRACTOR_LINES_CONFIG_FIELDS` macro.
- **Source**: In `CacheLocations()`, add `e->halvorsenALoc = GetShaderLocation(e->shader, "halvorsenA")`. In `BindScalarUniforms()`, add `SetShaderValue(e->shader, e->halvorsenALoc, &cfg->halvorsenA, SHADER_UNIFORM_FLOAT)`. In `AttractorLinesRegisterParams()`, add `ModEngineRegisterParam("attractorLines.halvorsenA", &cfg->halvorsenA, 1.0f, 3.0f)`.

**Verify**: Compiles.

#### Task 2.2: Attractor Flow config + C++

**Files**: `src/simulation/attractor_flow.h`, `src/simulation/attractor_flow.cpp`
**Depends on**: Wave 1

**Do**:
- **Header**: Add Dadras fields (`dadrasA` through `dadrasE` with same defaults/ranges as Attractor Lines) and `float halvorsenA = 1.89f;` to `AttractorFlowConfig`. Add `dadrasA/B/C/D/E` to `ATTRACTOR_FLOW_CONFIG_FIELDS` and `halvorsenA`. Add uniform location ints `dadrasALoc` through `dadrasELoc` and `halvorsenALoc` to `AttractorFlow` struct.
- **Source**: In `LoadComputeProgram()`, add `rlGetLocationUniform` calls for `dadrasA`, `dadrasB`, `dadrasC`, `dadrasD`, `dadrasE`, `halvorsenA`. In `AttractorFlowUpdate()`, add `rlSetUniform` calls for all 6 new uniforms. In `InitializeAgents()`, add `case ATTRACTOR_DADRAS` (±1.5 random) and `case ATTRACTOR_HALVORSEN` (±1.0 random, same as Thomas). In `AttractorFlowRegisterParams()`, add registration for `halvorsenA` (1.0-3.0). Follow existing pattern for Lorenz/Thomas params.

**Verify**: Compiles.

#### Task 2.3: Attractor Lines fragment shader

**Files**: `shaders/attractor_lines.fs`
**Depends on**: Wave 1

**Do**:
- Add `uniform float halvorsenA;` with other attractor uniforms.
- Add tuning constants: `STEP_HALVORSEN = 0.008`, `SCALE_HALVORSEN = 2.5`, `DIVERGE_HALVORSEN = 20.0`.
- Add `derivativeHalvorsen(vec3 p)` function (see Algorithm section).
- In `getStepSize()`: add `if (type == 5) return STEP_HALVORSEN;`
- In `getScaleFactor()`: add `if (type == 5) return SCALE_HALVORSEN;`
- In `getDivergeLimit()`: add `if (type == 5) return DIVERGE_HALVORSEN;`
- In `getCenterOffset()`: Halvorsen uses `vec3(0.0)` — no change needed (default branch already returns that).
- In `getStartingPoint()`: change the final `else` (Dadras) to `else if (type == 4)`, then add `else { base = vec3(0.5, 0.5, 0.5); r = 1.5; }` for Halvorsen.
- In `attractorDerivative()`: change final `return derivativeDadras(p);` to `if (type == 4) return derivativeDadras(p); return derivativeHalvorsen(p);`
- In `project()`: Halvorsen uses XY projection — falls through to the default `return p.xy * s;` branch, no change needed.

**Verify**: Restart app, enable Attractor Lines, select Halvorsen type — should render triangular lobes.

#### Task 2.4: Attractor Flow compute shader

**Files**: `shaders/attractor_agents.glsl`
**Depends on**: Wave 1

**Do**:
- Add uniforms: `uniform float dadrasA;`, `uniform float dadrasB;`, `uniform float dadrasC;`, `uniform float dadrasD;`, `uniform float dadrasE;`, `uniform float halvorsenA;`
- Add `dadrasDerivative(vec3 p)` function (see Algorithm section).
- Add `derivativeHalvorsen(vec3 p)` function (see Algorithm section — same as fragment shader but uses uniform name `halvorsenA`).
- Update `attractorDerivative()` dispatch: add `else if (attractorType == 4) { return dadrasDerivative(p); } else if (attractorType == 5) { return derivativeHalvorsen(p); }` after the Thomas branch.
- Update `projectToScreen()`: add Dadras branch (`projected = vec2(rotated.x * 2.7, rotated.y * 2.7);`) and Halvorsen branch (`projected = vec2(rotated.x * 4.0, rotated.y * 4.0);`). Insert before the final `else` (Thomas) — restructure the chain to handle types 0-5.
- Update `respawnAgent()`: add Dadras case (±1.5 random) and Halvorsen case (±1.0 random). Restructure the final `else` to distinguish Thomas/Dadras/Halvorsen.

**Verify**: Restart app, enable Attractor Flow, select Dadras and Halvorsen — both should render.

#### Task 2.5: UI updates

**Files**: `src/ui/imgui_effects_gen_filament.cpp`, `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1

**Do**:
- **`imgui_effects_gen_filament.cpp`** (Attractor Lines UI):
  - Update `attractorNames[]` array to include `"Halvorsen"` (6 entries total).
  - In `DrawAttractorSystemParams()`: add `else if (c->attractorType == ATTRACTOR_HALVORSEN)` branch with `ModulatableSlider("Halvorsen A##attractorLines", &c->halvorsenA, "attractorLines.halvorsenA", "%.2f", modSources)`.
- **`imgui_effects.cpp`** (Attractor Flow UI):
  - Update `attractorTypes[]` array to `{"Lorenz", "Rossler", "Aizawa", "Thomas", "Dadras", "Halvorsen"}` and change Combo count from `4` to `ATTRACTOR_COUNT`.
  - Add Dadras param block: `else if (e->attractorFlow.attractorType == ATTRACTOR_DADRAS)` with 5 SliderFloat calls for dadrasA-E (same ranges as Attractor Lines).
  - Add Halvorsen param block: `else if (e->attractorFlow.attractorType == ATTRACTOR_HALVORSEN)` with `ImGui::SliderFloat("Halvorsen A##attr", &e->attractorFlow.halvorsenA, 1.0f, 3.0f, "%.2f")`.

**Verify**: UI shows all 6 attractor types in both dropdowns.

#### Task 2.6: Serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1

**Do**:
- No new includes needed (headers already included).
- `ATTRACTOR_LINES_CONFIG_FIELDS` and `ATTRACTOR_FLOW_CONFIG_FIELDS` macros in their respective headers were already updated in Tasks 2.1/2.2 — the serialization macros reference those, so they'll pick up the new fields automatically.
- Verify both `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` lines reference the updated macros.

**Verify**: Save and load a preset with Halvorsen selected and halvorsenA modified — value persists.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Attractor Lines: Halvorsen renders with triangular-lobed shape
- [ ] Attractor Flow: both Dadras and Halvorsen render correctly
- [ ] halvorsenA slider modifies the attractor shape in both effects
- [ ] Type dropdown shows all 6 types in both UIs
- [ ] Preset save/load preserves halvorsenA and Dadras params
- [ ] Modulation routes to `attractorLines.halvorsenA`
