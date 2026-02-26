# Muons Phase & Drift Controls

Expose the hardcoded rotation axis phase vector `vec3(7,1,0)` and add a drift rate parameter so users can tune the spatial orientation and break the ~6.3s temporal cycle. Four new config fields, two new shader uniforms, four new UI sliders.

**Research**: `docs/research/muons-phase-drift.md`

## Design

### Types

**MuonsConfig additions** (add after `cameraDistance`):

```cpp
float phaseX = 0.717f;  // Rotation axis X phase offset (-PI_F to PI_F)
float phaseY = 1.0f;    // Rotation axis Y phase offset (-PI_F to PI_F)
float phaseZ = 0.0f;    // Rotation axis Z phase offset (-PI_F to PI_F)
float drift = 0.0f;     // Per-axis speed divergence — 0 = vanilla cycling (0.0-0.5)
```

- `phaseX` default 0.717f ≈ 7.0 − 2π, equivalent to the original hardcoded 7.0 but within the -PI..+PI slider range
- `drift` default 0.0 is backward compatible — existing presets keep their current behavior

**MuonsEffect additions** (add after `cameraDistanceLoc`):

```cpp
int phaseLoc;
int driftLoc;
```

**MUONS_CONFIG_FIELDS**: append `phaseX, phaseY, phaseZ, drift` after `cameraDistance`.

### Algorithm

Replace the hardcoded rotation axis line in `muons.fs`:

```glsl
// Current (line 60):
vec3 a = normalize(cos(vec3(7.0, 1.0, 0.0) + time - s));

// Replace with:
vec3 a = normalize(cos(phase + time * (1.0 + drift * vec3(1.0, PHI, PHI * PHI)) - s));
```

Where `phase` and `drift` are new uniforms. The fixed ratio `vec3(1.0, PHI, PHI * PHI)` ensures incommensurable per-axis periods when drift > 0, preventing the pattern from ever repeating.

- At `drift = 0`: all axes animate at speed 1.0, pattern cycles every 2π seconds (current behavior)
- At `drift > 0`: per-axis speeds diverge, cycle never repeats
- `phase` controls starting spatial orientation — which regions of the volume are dense vs sparse

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| phaseX | float | -PI_F to PI_F | 0.717f | yes | "Phase X" |
| phaseY | float | -PI_F to PI_F | 1.0f | yes | "Phase Y" |
| phaseZ | float | -PI_F to PI_F | 0.0f | yes | "Phase Z" |
| drift | float | 0.0 to 0.5 | 0.0f | yes | "Drift" |

---

## Tasks

### Wave 1: Config struct

#### Task 1.1: Add phase and drift fields to MuonsConfig and MuonsEffect

**Files**: `src/effects/muons.h`
**Creates**: Config fields and uniform locations that Wave 2 tasks depend on

**Do**:

1. Add `phaseX`, `phaseY`, `phaseZ`, `drift` fields to `MuonsConfig` after `cameraDistance`, with types, defaults, and range comments matching the Design section
2. Add `phaseLoc` and `driftLoc` to `MuonsEffect` after `cameraDistanceLoc`
3. Update `MUONS_CONFIG_FIELDS` macro — append `phaseX, phaseY, phaseZ, drift` after `cameraDistance`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader + C++ (parallel)

#### Task 2.1: Add phase and drift uniforms to shader

**Files**: `shaders/muons.fs`
**Depends on**: Wave 1 complete

**Do**:

1. Add two uniform declarations after the existing `uniform int turbulenceMode;` line:
   ```glsl
   uniform vec3 phase;
   uniform float drift;
   ```
2. Replace line 60 (`vec3 a = normalize(cos(vec3(7.0, 1.0, 0.0) + time - s));`) with:
   ```glsl
   vec3 a = normalize(cos(phase + time * (1.0 + drift * vec3(1.0, PHI, PHI * PHI)) - s));
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Bind uniforms, register params, add UI sliders

**Files**: `src/effects/muons.cpp`
**Depends on**: Wave 1 complete

**Do**:

1. **Init**: Cache two uniform locations after `cameraDistanceLoc`:
   ```cpp
   e->phaseLoc = GetShaderLocation(e->shader, "phase");
   e->driftLoc = GetShaderLocation(e->shader, "drift");
   ```

2. **Setup**: Bind uniforms after `cameraDistance` binding. Pack phase as vec3:
   ```cpp
   float phase[3] = {cfg->phaseX, cfg->phaseY, cfg->phaseZ};
   SetShaderValue(e->shader, e->phaseLoc, phase, SHADER_UNIFORM_VEC3);
   SetShaderValue(e->shader, e->driftLoc, &cfg->drift, SHADER_UNIFORM_FLOAT);
   ```

3. **RegisterParams**: Add 4 registrations after `cameraDistance`:
   ```
   "muons.phaseX"  → -PI_F to PI_F
   "muons.phaseY"  → -PI_F to PI_F
   "muons.phaseZ"  → -PI_F to PI_F
   "muons.drift"   → 0.0f to 0.5f
   ```
   Include `"config/constants.h"` for `PI_F`.

4. **UI**: In `DrawMuonsParams`, add 4 sliders inside the Raymarching section, after the Turbulence Mode combo and before March Steps:
   - Three `ModulatableSliderAngleDeg` for Phase X/Y/Z with IDs `"muons.phaseX"` etc.
   - One `ModulatableSlider` for Drift with `"%.3f"` format and ID `"muons.drift"`
   - All suffixed `##muons`

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Phase X/Y/Z sliders appear in Raymarching section after Turbulence Mode combo
- [ ] Phase sliders display degrees, store radians
- [ ] Drift = 0 matches previous visual behavior exactly
- [ ] Drift > 0 creates non-repeating animation
- [ ] Phase values shift which regions of the volume are dense/sparse
- [ ] Preset save/load round-trips all 4 new fields
