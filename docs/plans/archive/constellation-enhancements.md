# Constellation Enhancements: Triangle Fill + Movable Wave Center

Add filled triangles between close points and a movable wave origin to the existing constellation generator. No new files — all changes modify the existing constellation module.

**Research**: `docs/research/constellation_enhancements.md`

## Design

### Types

**ConstellationConfig** (modified fields):

```
// Renamed (was radialFreq/radialAmp/radialSpeed)
float waveFreq = 1.0f;    // Ripple count across grid (0.1-5.0)
float waveAmp = 2.0f;     // Coordination strength (0.0-4.0)
float waveSpeed = 0.5f;   // Ripple propagation speed (0.0-5.0)

// New: Triangle fill
bool fillEnabled = false;
float fillOpacity = 0.3f;     // Triangle fill brightness (0.0-1.0)
float fillThreshold = 2.5f;   // Max perimeter for visible triangles (1.0-4.0)

// New: Wave center
float waveCenterX = 0.5f;     // Wave origin X in UV space (-2.0 to 3.0)
float waveCenterY = 0.5f;     // Wave origin Y in UV space (-2.0 to 3.0)
```

**ConstellationEffect** (add uniform locations):

```
int fillEnabledLoc;
int fillOpacityLoc;
int fillThresholdLoc;
int waveCenterLoc;       // vec2 uniform
```

Rename existing locations: `radialFreqLoc` → `waveFreqLoc`, `radialPhaseLoc` → `wavePhaseLoc`. The `radialAmpLoc` field disappears — `waveAmp` is passed as the same uniform slot but renamed. Rename the `radialPhase` accumulator field to `wavePhase`.

### Algorithm

**Triangle fill** (shader-side, inside `Layer()`):

After gathering the 9 neighbor positions, iterate all unique triples (i < j < k) where i, j, k are in [0..8]. For each triple:

1. Compute perimeter = `length(p[i]-p[j]) + length(p[j]-p[k]) + length(p[i]-p[k])`
2. Skip if perimeter > `fillThreshold`
3. Evaluate `sdTriangle(gv, p[i], p[j], p[k])` using iq's exact formula from the research doc
4. Apply soft edge: `smoothstep(0.0, -lineThickness, dist)` (reuse lineThickness as blur width)
5. Compute barycentric weights for vertex color interpolation from pointLUT
6. Accumulate: `result += fillColor * alpha * fillOpacity`

Add `sdTriangle()` as a standalone function in the shader (before `Layer()`). Add a `BarycentricColor()` helper that computes weights and samples pointLUT at each vertex's `N21(cellID)`.

**Movable wave center** (shader-side, inside `GetPos()`):

Replace `length(cellID + cellOffset)` with `length(cellID + cellOffset - waveCenter * gridScale)` where `waveCenter` is a vec2 uniform. At default (0.5, 0.5), this reproduces current behavior because UV is centered at 0 (see `main()`: `uv = fragTexCoord - 0.5`), so `waveCenter` of (0.5, 0.5) maps to the grid origin. Actually — the existing code uses `length(cellID + cellOffset)` which radiates from grid origin (0,0), and UV is centered, so grid origin IS screen center. The `waveCenter` should be passed in grid-space coordinates directly. Convert on the CPU: `waveCenterGrid = (waveCenterX - 0.5) * gridScale` and similarly for Y (with aspect ratio correction). Pass as a `vec2 waveCenter` uniform already in grid-cell space so the shader just does `length(cellID + cellOffset - waveCenter)`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveFreq | float | 0.1-5.0 | 1.0 | No | "Wave Freq" |
| waveAmp | float | 0.0-4.0 | 2.0 | Yes (existing) | "Wave Amp" |
| waveSpeed | float | 0.0-5.0 | 0.5 | Yes (existing) | "Wave Speed" |
| fillEnabled | bool | — | false | No | "Fill Triangles" |
| fillOpacity | float | 0.0-1.0 | 0.3 | Yes | "Fill Opacity" |
| fillThreshold | float | 1.0-4.0 | 2.5 | No | "Fill Threshold" |
| waveCenterX | float | -2.0-3.0 | 0.5 | No | "Wave Center X" |
| waveCenterY | float | -2.0-3.0 | 0.5 | No | "Wave Center Y" |

### Constants

No new enum values or display names — this enhances the existing `TRANSFORM_CONSTELLATION_BLEND` entry.

### Modulation Param ID Changes

| Old ID | New ID |
|--------|--------|
| `constellation.radialAmp` | `constellation.waveAmp` |
| `constellation.radialSpeed` | `constellation.waveSpeed` |
| (new) | `constellation.fillOpacity` |

---

## Tasks

### Wave 1: Config + Effect Struct

#### Task 1.1: Update ConstellationConfig and ConstellationEffect

**Files**: `src/effects/constellation.h`
**Creates**: Updated struct layout that Wave 2 tasks depend on

**Do**:
- Rename `radialFreq` → `waveFreq`, `radialAmp` → `waveAmp`, `radialSpeed` → `waveSpeed` (keep same defaults)
- Add `fillEnabled`, `fillOpacity`, `fillThreshold`, `waveCenterX`, `waveCenterY` with defaults from Parameters table
- In `ConstellationEffect`: rename `radialFreqLoc` → `waveFreqLoc`, `radialAmpLoc` → `waveAmpLoc`, `radialPhaseLoc` → `wavePhaseLoc`, `radialPhase` → `wavePhase`
- Add `fillEnabledLoc`, `fillOpacityLoc`, `fillThresholdLoc`, `waveCenterLoc` (int)

**Verify**: `cmake.exe --build build` compiles (expect errors in .cpp files that reference old names — Wave 2 fixes those).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update ConstellationEffect Init/Setup/Params

**Files**: `src/effects/constellation.cpp`
**Depends on**: Wave 1 complete

**Do**:
- **Init**: Rename `GetShaderLocation` calls to match new uniform names (`waveFreq`, `waveAmp`, `wavePhase`, `fillEnabled`, `fillOpacity`, `fillThreshold`, `waveCenter`). `waveCenter` is a single vec2 uniform.
- **Setup**: Rename all `SetShaderValue` calls to use new field/loc names. Add binds for `fillEnabled` (as int 0/1), `fillOpacity`, `fillThreshold`. For `waveCenter`: compute grid-space coordinates from `waveCenterX`/`waveCenterY` using `float waveCenter[2] = { (cfg->waveCenterX - 0.5f) * cfg->gridScale * ((float)GetScreenWidth() / (float)GetScreenHeight()), (cfg->waveCenterY - 0.5f) * cfg->gridScale }` and bind as `SHADER_UNIFORM_VEC2`. Rename `radialPhase` accumulator to `wavePhase`, advance with `waveSpeed`.
- **RegisterParams**: Rename `constellation.radialAmp` → `constellation.waveAmp`, `constellation.radialSpeed` → `constellation.waveSpeed`. Add `constellation.fillOpacity` (0.0-1.0).

**Verify**: Compiles after Wave 2 completes.

---

#### Task 2.2: Update constellation.fs Shader

**Files**: `shaders/constellation.fs`
**Depends on**: Wave 1 complete

**Do**:
- **Uniforms**: Rename `radialFreq` → `waveFreq`, `radialAmp` → `waveAmp`, `radialPhase` → `wavePhase`. Add `uniform int fillEnabled`, `uniform float fillOpacity`, `uniform float fillThreshold`, `uniform vec2 waveCenter`.
- **GetPos()**: Replace `length(cellID + cellOffset)` with `length(cellID + cellOffset - waveCenter)`. This is the only line that changes in GetPos.
- **sdTriangle()**: Add iq's exact SDF function from the research doc (the `sdTriangle(p, p0, p1, p2)` code block).
- **BarycentricColor()**: Add helper that takes `(vec2 p, vec2 a, vec2 b, vec2 c, vec2 idA, vec2 idB, vec2 idC)`, computes barycentric weights via cross products per the research doc, and returns `w0*colorA + w1*colorB + w2*colorC` where each color is `textureLod(pointLUT, vec2(N21(cellID), 0.5), 0.0).rgb`.
- **Layer()**: After the existing line and point rendering loops, add the triangle fill block gated by `if (fillEnabled != 0)`. Triple-nested loop for i < j < k across [0..8]. Perimeter check → sdTriangle → smoothstep edge → BarycentricColor → accumulate with `fillOpacity`.

**Verify**: Shader compiles at runtime (no syntax errors in GLSL 330).

---

#### Task 2.3: Update UI Panel

**Files**: `src/ui/imgui_effects_generators.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `DrawGeneratorsConstellation()`:
  - Rename "Radial Wave" section header to "Wave"
  - Rename slider labels: "Radial Freq" → "Wave Freq", "Radial Amp" → "Wave Amp", "Radial Speed" → "Wave Speed"
  - Update field references: `c->radialFreq` → `c->waveFreq`, etc.
  - Update mod param IDs: `constellation.radialAmp` → `constellation.waveAmp`, `constellation.radialSpeed` → `constellation.waveSpeed`
  - Add `ImGui::SliderFloat("Wave Center X##constellation", &c->waveCenterX, -2.0f, 3.0f, "%.2f")` and same for Y, in the Wave section
  - Add new "Triangles" section (use `ImGui::SeparatorText("Triangles")`) after Lines section, before Output:
    - `ImGui::Checkbox("Fill Triangles##constellation", &c->fillEnabled)`
    - `ModulatableSlider("Fill Opacity##constellation", &c->fillOpacity, "constellation.fillOpacity", "%.2f", modSources)` (only if fillEnabled)
    - `ImGui::SliderFloat("Fill Threshold##constellation", &c->fillThreshold, 1.0f, 4.0f, "%.1f")` (only if fillEnabled)

**Verify**: Compiles.

---

#### Task 2.4: Update Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConstellationConfig, ...)` macro: rename `radialFreq` → `waveFreq`, `radialAmp` → `waveAmp`, `radialSpeed` → `waveSpeed`. Add `fillEnabled`, `fillOpacity`, `fillThreshold`, `waveCenterX`, `waveCenterY`.

**Verify**: Compiles.

---

#### Task 2.5: Update SPIROL.json Preset

**Files**: `presets/SPIROL.json`
**Depends on**: Wave 1 complete

**Do**:
- Rename JSON keys: `"radialFreq"` → `"waveFreq"`, `"radialAmp"` → `"waveAmp"`, `"radialSpeed"` → `"waveSpeed"`
- In the LFO routes section, rename `paramId` values: `"constellation.radialSpeed"` → `"constellation.waveSpeed"`, `"constellation.radialAmp"` → `"constellation.waveAmp"`

**Verify**: JSON is valid.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Enable constellation, toggle "Fill Triangles" — filled triangles appear between close points
- [ ] Drag Wave Center X/Y off-screen — wave direction changes from radial to directional
- [ ] Default waveCenterX/Y = 0.5 reproduces existing radial behavior
- [ ] Load SPIROL.json preset — no errors, constellation params load correctly
- [ ] fillOpacity responds to LFO modulation
