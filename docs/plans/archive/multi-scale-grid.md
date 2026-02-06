# Multi-Scale Grid

Transform effect that overlays three sine-warped grids at different scales, sampling the input texture at each grid cell's coordinate with parallax scroll. Cell edges darken to create tile boundaries; a glow threshold brightens hot cells. Produces a cyberpunk tile / LED wall / stage floor aesthetic.

**Research**: `docs/research/multi-scale-grid.md`

## Design

### Types

**MultiScaleGridConfig** (in `src/effects/multi_scale_grid.h`):

```
enabled       bool    false
scale1        float   10.0     Coarse grid density
scale2        float   25.0     Medium grid density
scale3        float   50.0     Fine grid density
scrollSpeed   float   0.005    Base drift rate (multiplied by time in shader)
warpAmount    float   0.5      Sine distortion amplitude on grid lines
edgeContrast  float   0.2      Cell boundary darkness
edgePower     float   3.0      Edge sharpness exponent
glowThreshold float   1.0      Brightness cutoff for glow
glowAmount    float   2.0      Glow intensity multiplier
glowMode      int     0        0 = hard (squared), 1 = soft (linear)
```

**MultiScaleGridEffect** (in `src/effects/multi_scale_grid.h`):

```
shader           Shader
scale1Loc        int
scale2Loc        int
scale3Loc        int
scrollSpeedLoc   int
warpAmountLoc    int
edgeContrastLoc  int
edgePowerLoc     int
glowThresholdLoc int
glowAmountLoc    int
glowModeLoc      int
timeLoc          int
```

No separate time accumulator — pass `transformTime` from PostEffect as the `time` uniform. The shader multiplies by `scrollSpeed` internally.

### Algorithm

The shader follows the Full Shader section from the research doc exactly. Three helper functions:

- `grid(uv, scale)` — scales UV then subtracts `warpAmount * 0.5 * sin(uv)` for organic waviness
- `edge(x)` — returns `abs(fract(x) - 0.5)`, yielding 0 at cell edges, 0.5 at centers
- `main()` — computes three grid layers, derives cell coordinates with parallax scroll offsets (1.0, 0.7, 1.3), blends weighted texture samples (0.8, 0.6, 0.4), subtracts edge darkness, applies glow

**Glow mode addition** (not in research): when `glowMode == 0`, use the research formula `tex *= glowAmount * smoothstep(threshold, threshold + 0.8, tex.r) * tex` (squared). When `glowMode == 1`, drop the final `* tex` for a softer linear glow: `tex *= glowAmount * smoothstep(threshold, threshold + 0.8, tex.r)`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scale1 | float | 5.0–20.0 | 10.0 | Yes | "Coarse Scale" |
| scale2 | float | 15.0–40.0 | 25.0 | Yes | "Medium Scale" |
| scale3 | float | 30.0–80.0 | 50.0 | Yes | "Fine Scale" |
| scrollSpeed | float | 0.0–0.1 | 0.005 | Yes | "Scroll Speed" |
| warpAmount | float | 0.0–1.0 | 0.5 | Yes | "Warp" |
| edgeContrast | float | 0.0–0.5 | 0.2 | Yes | "Edge Contrast" |
| edgePower | float | 1.0–5.0 | 3.0 | Yes | "Edge Power" |
| glowThreshold | float | 0.5–1.5 | 1.0 | Yes | "Glow Threshold" |
| glowAmount | float | 1.0–3.0 | 2.0 | Yes | "Glow Amount" |
| glowMode | int | 0–1 | 0 | No | "Glow Mode" |

### Constants

- Enum: `TRANSFORM_MULTI_SCALE_GRID`
- Display name: `"Multi-Scale Grid"`
- Category: `TRANSFORM_CATEGORY_CELLULAR`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Effect Module Header

**Files**: `src/effects/multi_scale_grid.h` (create)
**Creates**: Config struct + Effect struct + lifecycle declarations that all other tasks depend on

**Do**: Define `MultiScaleGridConfig` and `MultiScaleGridEffect` structs per the Design section above. Declare `Init`, `Setup`, `Uninit`, `ConfigDefault`, `RegisterParams`. Follow `src/effects/lattice_fold.h` as structural pattern. Setup takes `const MultiScaleGridConfig*`, `float deltaTime`, and `float transformTime`.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker errors expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect Registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Six changes, following lattice fold as pattern:
1. Add `#include "effects/multi_scale_grid.h"` with other effect includes
2. Add `TRANSFORM_MULTI_SCALE_GRID` enum entry before `TRANSFORM_EFFECT_COUNT`
3. Add `"Multi-Scale Grid"` case in `TransformEffectName()`
4. Add `TRANSFORM_MULTI_SCALE_GRID` to `TransformOrderConfig::order` array (commonly missed)
5. Add `MultiScaleGridConfig multiScaleGrid;` member to `EffectConfig`
6. Add `case TRANSFORM_MULTI_SCALE_GRID: return e->multiScaleGrid.enabled;` in `IsTransformEnabled()`

**Verify**: Compiles.

---

#### Task 2.2: Effect Module Implementation

**Files**: `src/effects/multi_scale_grid.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement all five functions following `src/effects/lattice_fold.cpp` pattern:
- `Init` — load `shaders/multi_scale_grid.fs`, cache all 11 uniform locations
- `Setup` — set all uniforms via `SetShaderValue`. Pass `transformTime` as the `time` uniform. All config fields are direct pass-through (no accumulator needed).
- `Uninit` — unload shader
- `ConfigDefault` — return default-constructed config
- `RegisterParams` — register all 9 float parameters with `ModEngineRegisterParam` using ranges from the Parameters table. `glowMode` (int) is not modulatable.

Parameter IDs follow `"multiScaleGrid.fieldName"` convention.

**Verify**: Compiles.

---

#### Task 2.3: Fragment Shader

**Files**: `shaders/multi_scale_grid.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Full Shader from the research doc (`docs/research/multi-scale-grid.md`, lines 78–122) with these modifications:
- Add `uniform int glowMode;`
- Glow line becomes: if `glowMode == 0`, use `tex *= glowAmount * smoothstep(glowThreshold, glowThreshold + 0.8, tex.r) * tex;` (research formula). If `glowMode == 1`, use `tex *= glowAmount * smoothstep(glowThreshold, glowThreshold + 0.8, tex.r);` (no squaring).
- Standard header: `#version 330`, `in vec2 fragTexCoord`, `out vec4 finalColor`, `uniform sampler2D texture0`

**Verify**: Shader compiles at runtime (no build-time check for GLSL).

---

#### Task 2.4: PostEffect Integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow lattice fold pattern in both files:
- **post_effect.h**: Add `#include "effects/multi_scale_grid.h"` and `MultiScaleGridEffect multiScaleGrid;` member near the other cellular effects
- **post_effect.cpp**: Add `MultiScaleGridEffectInit` call in `PostEffectInit()`, `MultiScaleGridEffectUninit` in `PostEffectUninit()`, and `MultiScaleGridRegisterParams(&pe->effects.multiScaleGrid)` in `PostEffectRegisterParams()` (commonly missed)

**Verify**: Compiles.

---

#### Task 2.5: Shader Setup Dispatch

**Files**: `src/render/shader_setup_cellular.h` (modify), `src/render/shader_setup_cellular.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow lattice fold pattern across all three files:
- **shader_setup_cellular.h**: Declare `void SetupMultiScaleGrid(PostEffect* pe);`
- **shader_setup_cellular.cpp**: Add `#include "effects/multi_scale_grid.h"` and implement `SetupMultiScaleGrid` — delegates to `MultiScaleGridEffectSetup(&pe->multiScaleGrid, &pe->effects.multiScaleGrid, pe->currentDeltaTime, pe->transformTime)`
- **shader_setup.cpp**: Add `case TRANSFORM_MULTI_SCALE_GRID:` returning `{&pe->multiScaleGrid.shader, SetupMultiScaleGrid, &pe->effects.multiScaleGrid.enabled}`. `shader_setup_cellular.h` include already exists.

**Verify**: Compiles.

---

#### Task 2.6: Build System

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/multi_scale_grid.cpp` to `EFFECTS_SOURCES` near the other cellular effects (lattice_fold, voronoi, phyllotaxis).

**Verify**: Compiles.

---

#### Task 2.7: UI Panel

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_cellular.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow lattice fold UI pattern:
- **imgui_effects.cpp**: Add `case TRANSFORM_MULTI_SCALE_GRID:` in `GetTransformCategory()` alongside other cellular entries (commonly missed)
- **imgui_effects_cellular.cpp**:
  1. Add `static bool sectionMultiScaleGrid = false;` with other section bools
  2. Add `DrawCellularMultiScaleGrid` helper before `DrawCellularCategory`. Controls:
     - Enabled checkbox with `MoveTransformToEnd` on first enable
     - Three `ModulatableSlider` for scale1/2/3 with labels "Coarse Scale", "Medium Scale", "Fine Scale"
     - `ModulatableSlider` for scrollSpeed, warpAmount, edgeContrast, edgePower, glowThreshold, glowAmount
     - Combo or radio button for glowMode: "Hard" / "Soft"
  3. Call `DrawCellularMultiScaleGrid` from `DrawCellularCategory` with spacing

All slider paramIds use `"multiScaleGrid.fieldName"` to match RegisterParams.

**Verify**: Compiles.

---

#### Task 2.8: Preset Serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Three additions following lattice fold pattern:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MultiScaleGridConfig, enabled, scale1, scale2, scale3, scrollSpeed, warpAmount, edgeContrast, edgePower, glowThreshold, glowAmount, glowMode)` macro
2. Add `if (e.multiScaleGrid.enabled) { j["multiScaleGrid"] = e.multiScaleGrid; }` in `to_json`
3. Add `e.multiScaleGrid = j.value("multiScaleGrid", e.multiScaleGrid);` in `from_json`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Cellular" category badge (not "???")
- [ ] Enabling effect adds it to the pipeline list
- [ ] Three grid layers visible with distinct scales
- [ ] Scroll creates parallax depth between layers
- [ ] Edge darkening creates visible cell boundaries
- [ ] Both glow modes produce visibly different results
- [ ] UI sliders modify effect in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to all 9 registered parameters
