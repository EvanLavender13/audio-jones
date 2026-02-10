# Circuit Board (Squarenoi)

Replace the existing Circuit Board warp (triangle-wave fold algorithm) with an SDF square Voronoi approach. Each grid cell picks a random shape (box, circle, diamond), and the second-closest distance trick merges neighboring cells into flowing PCB trace-like pathways. Cell sizes breathe with time; an optional dual-layer mode overlaps two grids for denser trace networks.

**Research**: `docs/research/circuit_board.md`

## Design

### Types

**CircuitBoardConfig** (replaces existing — all fields change):

```
enabled     bool    false
tileScale   float   8.0     Grid density (2.0-16.0)
strength    float   0.3     Warp displacement intensity (0.0-1.0)
baseSize    float   0.7     Static cell radius before animation (0.3-0.9)
breathe     float   0.2     Cell size oscillation amplitude (0.0-0.4)
breatheSpeed float  1.0     Cell size oscillation rate (0.1-4.0)
dualLayer   bool    false   Enable second overlapping grid evaluation
layerOffset float   40.0    Spatial offset between grids (5.0-80.0)
shapeMix    float   0.5     Bias toward boxes vs mixed shapes (0.0-1.0)
```

**CircuitBoardEffect** (replaces existing):

```
shader          Shader
tileScaleLoc    int
strengthLoc     int
baseSizeLoc     int
breatheLoc      int
timeLoc         int
dualLayerLoc    int
layerOffsetLoc  int
shapeMixLoc     int
time            float    // Accumulated animation time
```

Uniform `time` drives all animation (cell breathing). `breatheSpeed` multiplies in Setup before sending to GPU — shader receives pre-scaled time: `time += breatheSpeed * deltaTime`.

### Algorithm

Fully specified in research doc. Key points for implementation:

**Hash**: Murmur hash `uvec3 → uvec3` producing 3 random values per cell. The `get_point()` function maps cell coordinates through `(ivec2 - 1) * 4` to decorrelate neighbors. Shader implements both `murmurHash33()` and `hash()` wrapper as described in research.

**Cell evaluation**: For each pixel, scale UV by `tileScale`, determine grid cell, then search all 24 neighbors (5×5 minus center). Each neighbor's random `rng.z` selects shape (box if `rng.z < shapeMixThreshold`, circle if mid-range, diamond if high — thresholds derived from `shapeMix` parameter). Track closest and second-closest SDF distances. Output the second-closest distance as the displacement field.

**Shape selection thresholds**: `shapeMix` controls the bias. At `shapeMix = 0.0`, all cells are boxes. At `shapeMix = 1.0`, distribution is ~50% box, ~30% circle, ~20% diamond. Interpolate thresholds: `boxThreshold = 1.0 - shapeMix * 0.5`, `circleThreshold = boxThreshold + shapeMix * 0.3`.

**Displacement**: Compute gradient of the SDF field via central differences (small epsilon offset). Displace UV perpendicular to trace edges by `strength * sdField * normalize(gradient)`. Clamp final UV to [0, 1].

**Dual layer**: When `dualLayer` is on, evaluate the full squarenoi twice — second pass offsets UV by `vec2(layerOffset)` — and sum the two distance fields before computing displacement. This doubles the cost but creates crossing trace networks.

**Breathing**: Each cell's SDF radius oscillates as a `vec2` with elliptical variation:
```
rad = vec2(baseSize + cos(time + rng.z * TAU) * breathe,
           baseSize + sin(time + rng.z * TAU) * breathe)
```
The `cos`/`sin` split gives boxes slightly different X/Y radii over time. Circle and diamond SDFs use `rad.x` only (single radius). The per-cell phase from `rng.z` prevents synchronization.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| tileScale | float | 2.0-16.0 | 8.0 | yes | Tile Scale |
| strength | float | 0.0-1.0 | 0.3 | yes | Strength |
| baseSize | float | 0.3-0.9 | 0.7 | yes | Base Size |
| breathe | float | 0.0-0.4 | 0.2 | yes | Breathe |
| breatheSpeed | float | 0.1-4.0 | 1.0 | yes | Breathe Speed |
| dualLayer | bool | — | false | no | Dual Layer |
| layerOffset | float | 5.0-80.0 | 40.0 | yes | Layer Offset |
| shapeMix | float | 0.0-1.0 | 0.5 | yes | Shape Mix |

### Constants

- Enum: `TRANSFORM_CIRCUIT_BOARD` (existing — no change)
- Display name: `"Circuit Board"` (existing — no change)
- Category: `"WARP"` section 1 (existing — no change)
- Descriptor flags: `EFFECT_FLAG_NONE` (existing — no change)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Replace CircuitBoardConfig and CircuitBoardEffect structs

**Files**: `src/effects/circuit_board.h`
**Creates**: New config and effect structs that all other tasks depend on

**Do**: Replace the contents of both structs with the fields from the Design section above. Keep the same struct names, same include guard, same function declarations. The function signatures stay identical: `Init(CircuitBoardEffect*)`, `Setup(CircuitBoardEffect*, const CircuitBoardConfig*, float)`, `Uninit(CircuitBoardEffect*)`, `ConfigDefault(void)`, `RegisterParams(CircuitBoardConfig*)`. Follow the existing header as a pattern for comment style.

**Verify**: `cmake.exe --build build` compiles (will warn about unused params in .cpp until Wave 2).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Replace shader

**Files**: `shaders/circuit_board.fs`
**Depends on**: Wave 1 complete

**Do**: Replace the entire shader with the squarenoi algorithm from the research doc. Implement:
- `murmurHash33()` and `hash()` functions exactly as specified in research
- `get_point()` cell lookup with `(ivec2 - 1) * 4` mapping
- Three SDF primitives: `sdBox`, `sdCircle`, `sdDiamond`
- `sdVoronoi()` function: scale UV by `tileScale`, 24-neighbor search (5×5 minus center), per-cell shape selection using `shapeMix` thresholds, breathing animation using `time`/`breathe`/`baseSize`, track closest and second-closest SDF distances, return second-closest
- Main: compute displacement via central differences gradient of `sdVoronoi`, apply `strength * displacement`, optionally sum two `sdVoronoi` evaluations when `dualLayer` is on (second pass offsets UV by `layerOffset`), clamp final UV to [0, 1]

Note: The reference uses a `+.001` UV offset to hide tiling artifacts at grid edges — include this and test whether it's needed.

Uniforms: `sampler2D texture0`, `vec2 resolution`, `float tileScale`, `float strength`, `float baseSize`, `float breathe`, `float time`, `int dualLayer`, `float layerOffset`, `float shapeMix`.

#### Task 2.2: Replace effect module

**Files**: `src/effects/circuit_board.cpp`
**Depends on**: Wave 1 complete

**Do**: Replace all function bodies. Follow the existing file as a pattern for structure:
- `Init`: Load shader, cache all uniform locations (9 uniforms), init `time = 0`
- `Setup`: Accumulate `time += breatheSpeed * deltaTime`, bind all uniforms via `SetShaderValue`. Send `dualLayer` as int (0/1). All other params pass through directly.
- `Uninit`: `UnloadShader` (same as before)
- `ConfigDefault`: Return default-initialized `CircuitBoardConfig{}`
- `RegisterParams`: Register 6 modulatable floats — `tileScale`, `strength`, `baseSize`, `breathe`, `breatheSpeed`, `shapeMix`, `layerOffset` (7 total). Same pattern as existing: `ModEngineRegisterParam("circuitBoard.fieldName", &cfg->field, min, max)`.

#### Task 2.3: Replace UI controls

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1 complete

**Do**: Replace the body of `DrawWarpCircuitBoard()` (lines 317-350). Keep the function signature, the `DrawSectionBegin`/`DrawSectionEnd` wrapper, and the enable checkbox + `MoveTransformToEnd` pattern. Replace the parameter sliders with:
- `ModulatableSlider` for: Tile Scale, Strength, Base Size, Breathe, Breathe Speed, Shape Mix
- `ImGui::Checkbox` for: Dual Layer
- `ModulatableSlider` for: Layer Offset (only visible when `dualLayer` is on)

Use `"circuitBoard.fieldName"` as the param ID string matching `RegisterParams`. Follow the existing slider patterns in this file for format strings and ID suffixes (`##circuitboard`).

#### Task 2.4: Update preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**: Replace the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `CircuitBoardConfig` (line 404-408). New field list: `enabled, tileScale, strength, baseSize, breathe, breatheSpeed, dualLayer, layerOffset, shapeMix`. The `to_json`/`from_json` entries for `circuitBoard` remain unchanged (they already use the config field name).

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build`
- [ ] Effect appears in Warp section with "Circuit Board" label
- [ ] Enabling shows new UI controls (Tile Scale, Strength, Base Size, Breathe, Breathe Speed, Dual Layer, Layer Offset, Shape Mix)
- [ ] No old parameters visible (Pattern X/Y, Iterations, Scale Decay, Chromatic gone)
- [ ] Dual Layer checkbox toggles Layer Offset slider visibility
- [ ] Shader produces visible warp displacement when enabled
- [ ] Cell shapes vary (not all identical) when Shape Mix > 0
- [ ] Cells animate (breathe) over time
- [ ] Dual Layer mode produces denser trace pattern

---

## Implementation Notes

- **shapeMix removed**: All cells use `sdBox` only, matching the reference. Circle and diamond SDFs dropped. The `shapeMix` parameter, uniform, config field, UI slider, and preset field are all gone.
- **contourFreq added**: New parameter (0.0-80.0, default 0.0) modulates displacement with `cos(contourFreq * sdField)`, producing periodic contour bands that follow cell boundaries. At 0.0, displacement uses the raw distance field.
- **sdBox must be signed Chebyshev**: `p = abs(p) - rad; return max(p.x, p.y);` — NOT `length(max(abs(p) - rad, 0.0))`. The signed distance (negative inside cells) creates the trace boundary merging that defines the visual.
- **Center cell initialization**: `sdVoronoi` initializes `nearest`/`dists` from the center cell before the 24-neighbor loop, matching the reference.
- **Second-closest re-evaluation**: Tracks `lastnearest` position and `lastrng` phase through the sort. Returns a fresh `sdBox(p - lastnearest, rad)` at the end instead of a stale distance float.
- **Grid centering**: `(uv - 0.5) * tileScale` in main centers the pattern on screen. `sdVoronoi` does its own `pos -= 0.5` to shift the grid by half a cell (matching reference).
