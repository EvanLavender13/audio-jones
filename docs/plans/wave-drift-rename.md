# Wave Drift Rename

Rename "Sine Warp" to "Wave Drift", move from Warp (section 1) to Motion (section 3), and add a selectable wave type parameter (triangle, sine, sawtooth, square) to the octave displacement loop. The wave function pattern is lifted from Wave Warp's existing `wave()` implementation.

## Design

### Rename Map

| Before | After |
|--------|-------|
| `SineWarpConfig` | `WaveDriftConfig` |
| `SineWarpEffect` | `WaveDriftEffect` |
| `TRANSFORM_SINE_WARP` | `TRANSFORM_WAVE_DRIFT` |
| `sineWarp` (field names) | `waveDrift` (field names) |
| `sine_warp.h` / `sine_warp.cpp` | `wave_drift.h` / `wave_drift.cpp` |
| `shaders/sine_warp.fs` | `shaders/wave_drift.fs` |
| `"Sine Warp"` (display) | `"Wave Drift"` (display) |
| `"WARP"` badge, section 1 | `"MOT"` badge, section 3 |
| `"sineWarp.*"` param IDs | `"waveDrift.*"` param IDs |

### Types

```c
struct WaveDriftConfig {
  bool enabled = false;
  int waveType = 1;            // Basis function: 0=tri, 1=sine, 2=saw, 3=square
  int octaves = 4;             // Number of cascade octaves (1-8)
  float strength = 0.5f;       // Distortion intensity (0.0-2.0)
  float speed = 0.3f;          // Animation rate (radians/second)
  float octaveRotation = 0.5f; // Rotation per octave in radians (±π)
  bool radialMode = false;     // false=Cartesian warp, true=Polar warp
  bool depthBlend = true;      // true=sample each octave, false=sample once
};

#define WAVE_DRIFT_CONFIG_FIELDS \
  enabled, waveType, octaves, strength, speed, octaveRotation, radialMode, depthBlend
```

```c
typedef struct WaveDriftEffect {
  Shader shader;
  int timeLoc;
  int rotationLoc;
  int waveTypeLoc;       // NEW
  int octavesLoc;
  int strengthLoc;
  int octaveRotationLoc;
  int radialModeLoc;
  int depthBlendLoc;
  float time;
} WaveDriftEffect;
```

Note: `waveType` defaults to `1` (sine) to preserve current visual behavior.

### Algorithm

Add a `wave()` selector and `wave2()` helper to the shader, matching Wave Warp's pattern but adapted for the Cartesian/radial displacement loop:

```glsl
uniform int waveType; // 0=triangle, 1=sine, 2=sawtooth, 3=square

float wave(float x) {
    if (waveType == 0) return (abs(fract(x / 6.283185) - 0.5) - 0.25) * 4.0; // triangle [-1, 1]
    if (waveType == 1) return sin(x);                                          // sine     [-1, 1]
    if (waveType == 2) return fract(x / 6.283185) * 2.0 - 1.0;               // sawtooth [-1, 1]
    return sign(sin(x));                                                       // square   [-1, 1]
}
```

In the octave loop, replace all `sin(...)` calls with `wave(...)`:
- Cartesian: `p.x += wave(p.y * freq + time) * amp * strength;` (was `sin(...)`)
- Cartesian: `p.y += wave(p.x * freq + time * 1.3) * amp * strength;` (was `sin(...)`)
- Radial: `float displacement = wave(origRadius * freq + time) * amp * strength;` (was `sin(...)`)

The wave functions output in [-1, 1] range, same as `sin()`, so no amplitude adjustment needed.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveType | int | 0-3 | 1 | No | `"Wave Type"` (Combo) |
| octaves | int | 1-8 | 4 | No | `"Octaves"` |
| strength | float | 0.0-2.0 | 0.5 | Yes | `"Strength"` |
| speed | float | rad/s | 0.3 | No | `"Speed"` (SliderSpeedDeg) |
| octaveRotation | float | ±π | 0.5 | Yes | `"Octave Rotation"` (AngleDeg) |
| radialMode | bool | — | false | No | `"Radial Mode"` (Checkbox) |
| depthBlend | bool | — | true | No | `"Depth Blend"` (Checkbox) |

### Constants

- Enum: `TRANSFORM_WAVE_DRIFT` (replaces `TRANSFORM_SINE_WARP` at position 0)
- Display name: `"Wave Drift"`
- Badge: `"MOT"`
- Section index: `3` (Motion)

---

## Tasks

### Wave 1: Header + Shader

#### Task 1.1: Create wave_drift.h

**Files**: `src/effects/wave_drift.h` (create)
**Creates**: `WaveDriftConfig`, `WaveDriftEffect` types, function declarations

**Do**: Copy `sine_warp.h`, apply rename map, add `waveType` field (int, default 1) and `waveTypeLoc` field. Add `waveType` to the `WAVE_DRIFT_CONFIG_FIELDS` macro. Update include guard to `WAVE_DRIFT_H`. Update top-of-file comment to describe Wave Drift.

**Verify**: File exists with correct types.

#### Task 1.2: Create wave_drift.fs shader

**Files**: `shaders/wave_drift.fs` (create)
**Creates**: Shader with wave type selector

**Do**: Copy `sine_warp.fs`. Add `uniform int waveType;` and implement the `wave()` function from the Algorithm section. Replace all three `sin(...)` calls in the octave loop with `wave(...)`. Update the top-of-file comment to "Wave Drift" and describe multi-mode wave basis.

**Verify**: Shader file has `wave()` function and `waveType` uniform.

---

### Wave 2: Implementation + Integration

#### Task 2.1: Create wave_drift.cpp

**Files**: `src/effects/wave_drift.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Copy `sine_warp.cpp`, apply full rename map. Changes beyond rename:
- Include `"wave_drift.h"` instead of `"sine_warp.h"`
- Add `e->waveTypeLoc = GetShaderLocation(e->shader, "waveType");` in Init
- Add `SetShaderValue(e->shader, e->waveTypeLoc, &cfg->waveType, SHADER_UNIFORM_INT);` in Setup
- Load `"shaders/wave_drift.fs"` in Init
- In `DrawWaveDriftParams`: add `ImGui::Combo("Wave Type##waveDrift", &cfg->waveDrift.waveType, "Triangle\0Sine\0Sawtooth\0Square\0");` as the first widget
- Registration macro: `REGISTER_EFFECT(TRANSFORM_WAVE_DRIFT, WaveDrift, waveDrift, "Wave Drift", "MOT", 3, EFFECT_FLAG_NONE, SetupWaveDrift, NULL, DrawWaveDriftParams)`
- All param IDs use `"waveDrift.*"` prefix
- All ImGui IDs use `##waveDrift` suffix

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Update integration files

**Files**: `src/config/effect_config.h`, `src/config/effect_serialization.cpp`, `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Do**:
- `effect_config.h`:
  - Replace `#include "effects/sine_warp.h"` → `#include "effects/wave_drift.h"`
  - Replace `TRANSFORM_SINE_WARP` → `TRANSFORM_WAVE_DRIFT` in enum (keep at position 0)
  - Replace `SineWarpConfig sineWarp;` → `WaveDriftConfig waveDrift;` with updated comment `// Wave drift (multi-mode cascading displacement)`
- `effect_serialization.cpp`:
  - Replace `#include "effects/sine_warp.h"` → `#include "effects/wave_drift.h"`
  - Replace `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SineWarpConfig, SINE_WARP_CONFIG_FIELDS)` → `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveDriftConfig, WAVE_DRIFT_CONFIG_FIELDS)`
  - Replace `X(sineWarp)` → `X(waveDrift)` in `EFFECT_CONFIG_FIELDS` macro
- `post_effect.h`:
  - Replace `#include "effects/sine_warp.h"` → `#include "effects/wave_drift.h"`
  - Replace `SineWarpEffect sineWarp;` → `WaveDriftEffect waveDrift;`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Delete old files

**Files**: `src/effects/sine_warp.h` (delete), `src/effects/sine_warp.cpp` (delete), `shaders/sine_warp.fs` (delete)
**Depends on**: Tasks 2.1 and 2.2 complete

**Do**: Delete the three old files. Update `CMakeLists.txt` if it explicitly lists `sine_warp.cpp` (check first — if it uses a glob, no change needed).

**Verify**: `cmake.exe --build build` compiles with no missing file errors.

#### Task 2.4: Update presets (MANUAL)

**Files**: `presets/GRAYBOB.json`, `presets/YUPP.json`, `presets/WOBBYBOB.json`, `presets/RINGER.json`, `presets/GLITCHYBOB.json`
**Depends on**: Wave 1 complete (only needs to know the new key name)

**Do**: In each preset file, find-and-replace the JSON key `"sineWarp"` → `"waveDrift"`. The nested object contents stay the same — only the top-level key name changes.

**Verify**: Each file has `"waveDrift"` key, no `"sineWarp"` key remains.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] No references to `sineWarp`, `SineWarp`, `sine_warp`, or `SINE_WARP` remain in `src/` or `shaders/`
- [ ] No references to `sineWarp` remain in `presets/`
- [ ] Wave Drift appears in Motion section (section 3) in the UI
- [ ] Wave Type combo works: triangle, sine, sawtooth, square all produce distinct visuals
- [ ] Default wave type is Sine (preserves original behavior)
- [ ] Depth Blend and Radial Mode still function correctly
