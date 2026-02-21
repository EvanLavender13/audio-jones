# Cellular Mode Simplification

Replace the 9 separate intensity floats in Voronoi and Phyllotaxis with a single mode dropdown and intensity slider. The current multi-toggle + blend-mix UI is confusing and the sub-effects don't blend well together. One mode at a time with a single intensity control.

## Design

### Shared Mode Enum

Both Voronoi and Phyllotaxis use the same 9 sub-effects. Define a shared enum in a common location so the shader `int mode` uniform and C++ `int mode` field agree on values.

```
CELL_MODE_UV_DISTORT     = 0   "Distort"
CELL_MODE_ORGANIC_FLOW   = 1   "Organic Flow"
CELL_MODE_EDGE_ISO       = 2   "Edge Iso"
CELL_MODE_CENTER_ISO     = 3   "Center Iso"
CELL_MODE_FLAT_FILL      = 4   "Flat Fill"
CELL_MODE_EDGE_GLOW      = 5   "Edge Glow"
CELL_MODE_RATIO          = 6   "Ratio"
CELL_MODE_DETERMINANT    = 7   "Determinant"
CELL_MODE_EDGE_DETECT    = 8   "Edge Detect"
CELL_MODE_COUNT          = 9
```

### VoronoiConfig Changes

Remove: `uvDistortIntensity`, `edgeIsoIntensity`, `centerIsoIntensity`, `flatFillIntensity`, `organicFlowIntensity`, `edgeGlowIntensity`, `determinantIntensity`, `ratioIntensity`, `edgeDetectIntensity`

Add: `int mode = 0;` and `float intensity = 0.5f;`

Keep unchanged: `enabled`, `smoothMode`, `scale`, `speed`, `edgeFalloff`, `isoFrequency`

### PhyllotaxisConfig Changes

Same removal and addition as Voronoi.

Keep unchanged: `enabled`, `smoothMode`, `scale`, `divergenceAngle`, `angleSpeed`, `phaseSpeed`, `spinSpeed`, `cellRadius`, `isoFrequency`

### VoronoiEffect / PhyllotaxisEffect Changes

Replace the 9 `*IntensityLoc` fields with two: `modeLoc` and `intensityLoc`.

### Shader Changes (voronoi.fs, phyllotaxis.fs)

Replace the 9 `uniform float *Intensity` with `uniform int mode` and `uniform float intensity`.

Replace the early-out check (`totalIntensity <= 0.0`) with `if (intensity <= 0.0)`.

Replace the per-effect `if (*Intensity > 0.0)` blocks with a single `if/else if` chain (or switch) on `mode`. Each branch uses `intensity` where it previously used its own `*Intensity` value.

### UI Changes

Replace the `IntensityToggleButton` grid + blend-mix sliders with:
- `ImGui::Combo("Mode##vor", &v->mode, "Distort\0Organic Flow\0Edge Iso\0Center Iso\0Flat Fill\0Edge Glow\0Ratio\0Determinant\0Edge Detect\0")`
- `ModulatableSlider("Intensity##vor", &v->intensity, "voronoi.intensity", "%.2f", modSources)`

Remove the `activeCount` logic and the "Blend Mix" section entirely.

### Param Registration Changes

Remove the 9 intensity param registrations per effect. Add one `"voronoi.intensity"` / `"phyllotaxis.intensity"` registration (range 0.0-1.0).

### Preset Updates

- STAYINNIT: `voronoi.mode = 6` (ratio), `voronoi.intensity = 1.0`
- SOLO: `voronoi.mode = 0`, `voronoi.intensity = 0.0` (was all zeros)
- WINNY: `voronoi.mode = 0`, `voronoi.intensity = 0.0` (was all zeros)
- ZOOP: `phyllotaxis.mode = 1` (organicFlow), `phyllotaxis.intensity = 1.0`

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| mode | int | 0-8 | 0 | No | "Mode" |
| intensity | float | 0.0-1.0 | 0.5 | Yes | "Intensity" |

---

## Tasks

### Wave 1: Config Headers

#### Task 1.1: Update VoronoiConfig and VoronoiEffect

**Files**: `src/effects/voronoi.h`

**Do**:
- Remove the 9 `*Intensity` float fields from `VoronoiConfig`
- Add `int mode = 0;` and `float intensity = 0.5f;`
- Update `VORONOI_CONFIG_FIELDS` macro to list the new fields instead of the old 9
- In `VoronoiEffect`, remove the 9 `*IntensityLoc` fields, add `modeLoc` and `intensityLoc`

**Verify**: Compiles (will have errors in .cpp files until Wave 2, but header is syntactically valid).

#### Task 1.2: Update PhyllotaxisConfig and PhyllotaxisEffect

**Files**: `src/effects/phyllotaxis.h`

**Do**: Same changes as Task 1.1 but for Phyllotaxis. Remove the 9 `*Intensity` float fields, add `int mode = 0;` and `float intensity = 0.5f;`. Update `PHYLLOTAXIS_CONFIG_FIELDS`. Update `PhyllotaxisEffect` struct.

**Verify**: Header is syntactically valid.

---

### Wave 2: Implementation (all files are independent)

#### Task 2.1: Update voronoi.cpp

**Files**: `src/effects/voronoi.cpp`
**Depends on**: Wave 1

**Do**:
- In `VoronoiEffectInit`: replace 9 `GetShaderLocation` calls with two: `modeLoc` ("mode") and `intensityLoc` ("intensity")
- In `VoronoiEffectSetup`: replace 9 `SetShaderValue` calls with two: mode as `SHADER_UNIFORM_INT`, intensity as `SHADER_UNIFORM_FLOAT`
- In `VoronoiRegisterParams`: remove the 9 intensity param registrations, add `ModEngineRegisterParam("voronoi.intensity", &cfg->intensity, 0.0f, 1.0f)`

**Verify**: Compiles.

#### Task 2.2: Update phyllotaxis.cpp

**Files**: `src/effects/phyllotaxis.cpp`
**Depends on**: Wave 1

**Do**: Same changes as Task 2.1 but for Phyllotaxis. Replace 9 loc fields with `modeLoc`/`intensityLoc`, replace 9 `SetShaderValue` calls with two, update param registration.

**Verify**: Compiles.

#### Task 2.3: Update voronoi.fs shader

**Files**: `shaders/voronoi.fs`

**Do**:
- Replace the 9 `uniform float *Intensity` declarations with `uniform int mode;` and `uniform float intensity;`
- Replace the early-out `totalIntensity` check with `if (intensity <= 0.0)`
- UV distort modes (mode 0, 1) run before the base texture sample; all other modes run after. Structure:
  - If mode == 0 (UV Distort): apply displacement to `finalUV` using `intensity`
  - Else if mode == 1 (Organic Flow): apply displacement to `finalUV` using `intensity`
  - Sample `color = texture(texture0, finalUV).rgb`
  - If mode == 2 (Edge Iso): apply using `intensity`
  - Else if mode == 3 (Center Iso): apply using `intensity`
  - ... and so on for modes 4-8
- Each branch uses `intensity` where it previously used its mode-specific intensity float. The math inside each branch stays identical.

**Verify**: No build step needed (runtime GPU compilation). Visual inspection of shader syntax.

#### Task 2.4: Update phyllotaxis.fs shader

**Files**: `shaders/phyllotaxis.fs`

**Do**: Same structural change as Task 2.3. Replace 9 uniform floats with `int mode` + `float intensity`. Same if/else chain pattern. Preserve the existing math in each branch (phyllotaxis has slightly different formulas than voronoi — pulse modulation, different edge distance calc, etc.).

**Verify**: Visual inspection of shader syntax.

#### Task 2.5: Update cellular UI

**Files**: `src/ui/imgui_effects_cellular.cpp`
**Depends on**: Wave 1

**Do**:
- In `DrawCellularVoronoi`: replace the `IntensityToggleButton` grid, `activeCount` logic, and "Blend Mix" section with:
  - `ImGui::Combo("Mode##vor", &v->mode, "Distort\0Organic Flow\0Edge Iso\0Center Iso\0Flat Fill\0Edge Glow\0Ratio\0Determinant\0Edge Detect\0")`
  - `ModulatableSlider("Intensity##vor", &v->intensity, "voronoi.intensity", "%.2f", modSources)`
- In `DrawCellularPhyllotaxis`: same replacement with `##phyllo` suffixes and `"phyllotaxis.intensity"` param ID
- Keep the Iso Settings tree node in both (isoFrequency, edgeFalloff for voronoi; isoFrequency, cellRadius for phyllotaxis)

**Verify**: Compiles.

#### Task 2.6: Update preset files

**Files**: `presets/STAYINNIT.json`, `presets/SOLO.json`, `presets/WINNY.json`, `presets/ZOOP.json`

**Do**:
- STAYINNIT voronoi section: remove 9 intensity fields, add `"mode": 6, "intensity": 1.0`
- SOLO voronoi section: remove 9 intensity fields, add `"mode": 0, "intensity": 0.0`
- WINNY voronoi section: remove 9 intensity fields, add `"mode": 0, "intensity": 0.0`
- ZOOP phyllotaxis section: remove 9 intensity fields, add `"mode": 1, "intensity": 1.0`

**Verify**: JSON is valid.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Presets STAYINNIT and ZOOP load and look correct (the only two with active sub-effects)
- [ ] Mode dropdown cycles through all 9 modes for both Voronoi and Phyllotaxis
- [ ] Intensity slider modulates the active mode's strength
- [ ] LFO modulation works on the intensity param
