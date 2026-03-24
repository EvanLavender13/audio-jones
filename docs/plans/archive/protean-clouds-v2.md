# Protean Clouds v2 - Camera & Mode Enhancements

Enhance the existing Protean Clouds generator with three additions: configurable camera drift motion (replacing hardcoded `disp()`), swappable turbulence displacement waveforms, and swappable density evaluation functions. Uses existing shared enums (`WaveformMode`, `PeriodicSurfaceMode`) and UI combos.

**Research**: `docs/research/protean_clouds_v2.md`

## Design

### Types

**ProteanCloudsConfig** (modify existing struct in `protean_clouds.h`):

Add after existing `speed` field in Motion section:

```
int densityMode = 0;           // Density evaluation function (0-4, PeriodicSurfaceMode)
int turbulenceMode = 0;        // FBM displacement waveform (0-7, WaveformMode)
float rollSpeed = 0.0f;        // Barrel roll rate (rad/s, -ROTATION_SPEED_MAX to ROTATION_SPEED_MAX)

DualLissajousConfig drift = {
    .amplitude = 2.0f,
    .motionSpeed = 1.0f,
    .freqX1 = 0.22f,
    .freqY1 = 0.175f,
    .freqX2 = 0.0f,
    .freqY2 = 0.0f,
    .offsetX2 = 0.3f,
    .offsetY2 = 2.09f,
};
```

**ProteanCloudsEffect** (modify existing struct in `protean_clouds.h`):

Add fields:

```
float rollAngle;          // CPU-accumulated roll (rollSpeed * deltaTime)
int densityModeLoc;
int turbulenceModeLoc;
int rollAngleLoc;
int driftAmplitudeLoc;
int driftFreqX1Loc;
int driftFreqY1Loc;
int driftFreqX2Loc;
int driftFreqY2Loc;
int driftOffsetX2Loc;
int driftOffsetY2Loc;
```

**CONFIG_FIELDS macro** - add: `densityMode, turbulenceMode, rollSpeed, drift`

### Algorithm

#### Shader `disp()` replacement

Replace the hardcoded `disp()` function with a parameterized version. The shader evaluates this per-sample (not CPU-side) because `map()` calls it at each sample point's z-position to center the density field around the flight path. `motionSpeed` and `phase` from `DualLissajousConfig` are not used - drift rate is inherent to flight speed since frequencies operate on z-position.

```glsl
uniform float driftAmplitude;
uniform float driftFreqX1, driftFreqY1;
uniform float driftFreqX2, driftFreqY2;
uniform float driftOffsetX2, driftOffsetY2;
uniform float rollAngle;

vec2 disp(float t) {
    vec2 d = vec2(sin(t * driftFreqX1), cos(t * driftFreqY1));
    if (driftFreqX2 > 0.0) d.x += sin(t * driftFreqX2 + driftOffsetX2);
    if (driftFreqY2 > 0.0) d.y += cos(t * driftFreqY2 + driftOffsetY2);
    return d * driftAmplitude;
}
```

#### Roll replacement

Replace line:
```glsl
rd.xy *= rot(-disp(flyPhase + 3.5).x * 0.2);
```
With:
```glsl
rd.xy *= rot(rollAngle);
```

`rollAngle` is accumulated on CPU: `e->rollAngle += cfg->rollSpeed * deltaTime`.

#### Turbulence mode switch (Swap 1)

Replace the sine displacement in the FBM loop:
```glsl
p += sin(p.zxy * 0.75 * trk + time * trk * .8) * dspAmp;
```

With a switch on `turbulenceMode`. The argument is `vec3 arg = p.zxy * 0.75 * trk + time * trk * .8`. The `* dspAmp` stays outside the switch.

```glsl
uniform int turbulenceMode;

// Inside FBM loop, replacing the sin(arg) line:
vec3 arg = p.zxy * 0.75 * trk + time * trk * .8;
vec3 warp;
switch (turbulenceMode) {
case 1: // Fract Fold
    warp = fract(arg) * 2.0 - 1.0;
    break;
case 2: // Abs-Sin
    warp = abs(sin(arg));
    break;
case 3: // Triangle
    warp = abs(fract(arg / TWO_PI + 0.5) * 2.0 - 1.0) * 2.0 - 1.0;
    break;
case 4: // Squared Sin
    warp = sin(arg) * abs(sin(arg));
    break;
case 5: // Square Wave
    warp = step(0.5, fract(arg / TWO_PI)) * 2.0 - 1.0;
    break;
case 6: // Quantized
    warp = floor(sin(arg) * 4.0 + 0.5) / 4.0;
    break;
case 7: // Cosine
    warp = cos(arg);
    break;
default: // 0: Sine (original)
    warp = sin(arg);
    break;
}
p += warp * dspAmp;
```

#### Density mode switch (Swap 2)

Replace the density evaluation in the FBM loop:
```glsl
d -= abs(dot(cos(p), sin(p.yzx)) * z);
```

With a switch on `densityMode`:

```glsl
uniform int densityMode;

#define PHI 1.618033988
const mat3 GOLD = mat3(
    -0.571464913, +0.814921382, +0.096597072,
    -0.278044873, -0.303026659, +0.911518454,
    +0.772087367, +0.494042493, +0.399753815);

// Inside FBM loop, replacing the dot(cos(p), sin(p.yzx)) line:
float density;
switch (densityMode) {
case 1: // Schwarz P
    density = cos(p.x) + cos(p.y) + cos(p.z);
    break;
case 2: // Diamond
    density = (cos(p.x)*cos(p.y)*cos(p.z) + sin(p.x)*sin(p.y)*cos(p.z)
             + sin(p.x)*cos(p.y)*sin(p.z) + cos(p.x)*sin(p.y)*sin(p.z)) * 1.5;
    break;
case 3: // Neovius
    density = (3.0*(cos(p.x)+cos(p.y)+cos(p.z))
              + 4.0*cos(p.x)*cos(p.y)*cos(p.z)) * 0.23;
    break;
case 4: // Dot Noise
    density = dot(cos(GOLD * p), sin(PHI * p * GOLD));
    break;
default: // 0: Gyroid (original)
    density = dot(cos(p), sin(p.yzx));
    break;
}
d -= abs(density * z);
```

`GOLD` and `PHI` are declared as shader-level constants (outside any function).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| densityMode | int | 0-4 | 0 | No | `DrawPeriodicSurfaceCombo("Density##proteanClouds", ...)` |
| turbulenceMode | int | 0-7 | 0 | No | `DrawWaveformCombo("Turbulence Mode##proteanClouds", ...)` |
| rollSpeed | float | -ROTATION_SPEED_MAX to ROTATION_SPEED_MAX | 0.0 | Yes | `ModulatableSliderSpeedDeg("Roll##proteanClouds", ...)` |
| drift.amplitude | float | 0.0-4.0 | 2.0 | Yes | Via `DrawLissajousControls` |
| drift.freqX1 | float | 0.0-1.0 | 0.22 | No | Via `DrawLissajousControls` (freqMax=1.0) |
| drift.freqY1 | float | 0.0-1.0 | 0.175 | No | Via `DrawLissajousControls` (freqMax=1.0) |
| drift.freqX2 | float | 0.0-1.0 | 0.0 | No | Via `DrawLissajousControls` (freqMax=1.0) |
| drift.freqY2 | float | 0.0-1.0 | 0.0 | No | Via `DrawLissajousControls` (freqMax=1.0) |
| drift.offsetX2 | float | 0.0-TWO_PI | 0.3 | No | Via `DrawLissajousControls` |
| drift.offsetY2 | float | 0.0-TWO_PI | 2.09 | No | Via `DrawLissajousControls` |

---

## Tasks

### Wave 1: Header

#### Task 1.1: Update config and effect structs

**Files**: `src/effects/protean_clouds.h`
**Creates**: Updated `ProteanCloudsConfig` and `ProteanCloudsEffect` structs that Wave 2 tasks depend on

**Do**:
1. Add `#include "config/dual_lissajous_config.h"` and `#include "config/waveform_mode.h"` and `#include "config/periodic_surface_mode.h"` to includes
2. Add to `ProteanCloudsConfig` after the `speed` field (still in Motion section):
   - `int densityMode = 0;` with range comment `(0-4, PeriodicSurfaceMode)`
   - `int turbulenceMode = 0;` with range comment `(0-7, WaveformMode)`
   - `float rollSpeed = 0.0f;` with range comment `(-ROTATION_SPEED_MAX to ROTATION_SPEED_MAX)`
   - `DualLissajousConfig drift = { ... };` with overridden defaults from Design section
3. Add to `ProteanCloudsEffect` struct:
   - `float rollAngle;` for CPU-accumulated roll
   - Uniform location ints: `densityModeLoc`, `turbulenceModeLoc`, `rollAngleLoc`, `driftAmplitudeLoc`, `driftFreqX1Loc`, `driftFreqY1Loc`, `driftFreqX2Loc`, `driftFreqY2Loc`, `driftOffsetX2Loc`, `driftOffsetY2Loc`
4. Update `PROTEAN_CLOUDS_CONFIG_FIELDS` macro to add: `densityMode, turbulenceMode, rollSpeed, drift`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update effect module logic and UI

**Files**: `src/effects/protean_clouds.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. Add includes: `"config/dual_lissajous_config.h"`, `"config/waveform_mode.h"`, `"config/periodic_surface_mode.h"`
2. In `ProteanCloudsEffectInit()`:
   - Cache all new uniform locations: `densityMode`, `turbulenceMode`, `rollAngle`, `driftAmplitude`, `driftFreqX1`, `driftFreqY1`, `driftFreqX2`, `driftFreqY2`, `driftOffsetX2`, `driftOffsetY2`
   - Initialize `e->rollAngle = 0.0f;`
3. In `BindUniforms()`:
   - Bind `densityMode` and `turbulenceMode` as `SHADER_UNIFORM_INT`
   - Bind `rollAngle` as `SHADER_UNIFORM_FLOAT`
   - Bind drift fields as individual `SHADER_UNIFORM_FLOAT`: `cfg->drift.amplitude`, `cfg->drift.freqX1`, `cfg->drift.freqY1`, `cfg->drift.freqX2`, `cfg->drift.freqY2`, `cfg->drift.offsetX2`, `cfg->drift.offsetY2`
4. In `ProteanCloudsEffectSetup()`:
   - Add `e->rollAngle += cfg->rollSpeed * deltaTime;` after the existing `flyPhase` accumulation
5. In `ProteanCloudsRegisterParams()`:
   - Register `"proteanClouds.rollSpeed"` with bounds `-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX`
   - Register `"proteanClouds.drift.amplitude"` with bounds `0.0f, 4.0f`
6. In `DrawProteanCloudsParams()`:
   - In the Volume section, add before March Steps: `DrawPeriodicSurfaceCombo("Density##proteanClouds", &pc->densityMode);` and `DrawWaveformCombo("Turbulence Mode##proteanClouds", &pc->turbulenceMode);`
   - In the Motion section, add after Speed: `ModulatableSliderSpeedDeg("Roll##proteanClouds", &pc->rollSpeed, "proteanClouds.rollSpeed", modSources);`
   - Add a new Camera section after Motion: `ImGui::SeparatorText("Camera");` then `DrawLissajousControls(&pc->drift, "pc_drift", "proteanClouds.drift", modSources, 1.0f);`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Update shader

**Files**: `shaders/protean_clouds.fs`
**Depends on**: Wave 1 complete (for understanding uniform names, but no file dependency)

**Do**:
1. Add new uniforms after existing uniform block:
   - `uniform int densityMode;`
   - `uniform int turbulenceMode;`
   - `uniform float rollAngle;`
   - `uniform float driftAmplitude;`
   - `uniform float driftFreqX1, driftFreqY1;`
   - `uniform float driftFreqX2, driftFreqY2;`
   - `uniform float driftOffsetX2, driftOffsetY2;`
2. Add constants before `disp()`:
   - `#define TWO_PI 6.28318530718`
   - `#define PHI 1.618033988`
   - `const mat3 GOLD = mat3(...)` (values from Design section)
3. Replace `disp()` function with the parameterized version from the Algorithm section
4. In `map()` function, replace the FBM loop body:
   - Replace `p += sin(p.zxy * 0.75 * trk + time * trk * .8) * dspAmp;` with the turbulence mode switch from Algorithm section
   - Replace `d -= abs(dot(cos(p), sin(p.yzx)) * z);` with the density mode switch from Algorithm section
5. In `main()`, replace the roll line:
   - Replace `rd.xy *= rot(-disp(flyPhase + 3.5).x * 0.2);` with `rd.xy *= rot(rollAngle);`

**Verify**: `cmake.exe --build build` compiles. Launch app, enable Protean Clouds, verify default appearance matches original (densityMode=0, turbulenceMode=0, rollSpeed=0, drift defaults match original disp). Switch density/turbulence modes and confirm visual changes.

---

## Implementation Notes

**Density modes and turbulence modes were cut.** Both were implemented as specified
but produced negligible visual differences in practice. The 5-octave FBM with m3
rotation matrix homogenizes the TPMS density surfaces - Gyroid, Schwarz P, Diamond
all look nearly identical through the FBM. The turbulence waveforms (adapted from
Muons) either looked identical to sine or produced hard-edged grid artifacts
incompatible with volumetric cloud rendering. The Muons waveforms operate in a
fundamentally different FBM context and do not transfer.

**Kept:** Configurable camera drift (DualLissajousConfig), barrel roll, FBM octaves
slider. The octaves slider was added during implementation to expose structural
variation that the mode switches failed to provide.

**Other fixes during implementation:**
- Removed hardcoded `dspAmp = 0.85` camera multiplier - amplitude is now fully
  controlled by `driftAmplitude`
- Roll applies to screen-space `p` before look-at projection, not to `rd.xy` after
  (world-space XY rotation is not a barrel roll when camera is off-axis)
- `motionSpeed` from DualLissajousConfig has no runtime effect - drift rate is
  inherent to flight speed since disp() evaluates per-sample on z-position

## Final Verification

- [x] Build succeeds with no warnings
- [x] Roll speed slider produces barrel roll when non-zero
- [x] Drift amplitude=0 produces straight-line flight
- [x] DrawLissajousControls renders and frequency sliders respond
- [x] Octaves slider changes cloud structure
- [ ] Presets save/load all new fields (create test preset, reload)
