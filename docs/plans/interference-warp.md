# Interference Warp

UV displacement effect using multi-axis harmonic interference. Summed sine waves across configurable axes create lattice-like distortion patterns (triangular, rectangular, quasicrystal) that evolve as harmonics drift in and out of phase.

## Specification

### Types

```cpp
// src/config/interference_warp_config.h
#ifndef INTERFERENCE_WARP_CONFIG_H
#define INTERFERENCE_WARP_CONFIG_H

// Interference Warp - multi-axis harmonic UV displacement
// Sums sine waves across configurable axes to create lattice-like distortion.
// Higher axes produce more complex quasicrystal patterns.
struct InterferenceWarpConfig {
    bool enabled = false;
    float amplitude = 0.1f;      // Displacement strength (0.0-0.5)
    float scale = 2.0f;          // Pattern frequency/density (0.5-10.0)
    int axes = 3;                // Lattice symmetry type (2-8)
    float axisRotation = 0.0f;   // Rotates entire pattern, radians (0-π)
    int harmonics = 64;          // Detail level (8-256)
    float decay = 1.0f;          // Amplitude falloff exponent (0.5-2.0)
    float speed = 0.2f;          // Time evolution rate (0.0-2.0)
    float drift = 2.0f;          // Harmonic phase drift exponent (1.0-3.0)
};

#endif // INTERFERENCE_WARP_CONFIG_H
```

### Algorithm

The shader computes a 2D displacement vector by summing harmonics across multiple interference axes.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float amplitude;
uniform float scale;
uniform int axes;
uniform float axisRotation;
uniform int harmonics;
uniform float decay;
uniform float drift;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;

// Sum sine waves across all axes for a given wave number k
float psi(float k, vec2 s) {
    float sum = 0.0;
    for (int i = 0; i < axes; i++) {
        float angle = axisRotation + float(i) * PI / float(axes);
        vec2 dir = vec2(cos(angle), sin(angle));
        sum += sin(k * dot(s, dir));
    }
    return sum;
}

// Time-varying phase rotation per harmonic
vec2 omega(float k, float t) {
    float p = pow(k, drift) * t;
    return vec2(sin(p), cos(p));
}

// Harmonic summation with amplitude decay
vec2 phi(vec2 s, float t) {
    vec2 p = vec2(0.0);
    for (int j = 1; j < harmonics; j++) {
        float k = float(j) * TAU;
        float amp = pow(k, -decay);
        p += omega(k, t) * psi(k, s) * amp;
    }
    return p;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 displacement = phi(uv * scale, time * speed) * amplitude;
    vec2 displaced = uv + displacement;

    // Mirror at boundaries
    displaced = 1.0 - abs(mod(displaced, 2.0) - 1.0);

    finalColor = texture(texture0, displaced);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| amplitude | float | 0.0-0.5 | 0.1 | Yes | "Amplitude" |
| scale | float | 0.5-10.0 | 2.0 | No | "Scale" |
| axes | int | 2-8 | 3 | No | "Axes" |
| axisRotation | float | 0-π | 0.0 | Yes | "Axis Rotation" |
| harmonics | int | 8-256 | 64 | No | "Harmonics" |
| decay | float | 0.5-2.0 | 1.0 | No | "Decay" |
| speed | float | 0.0-2.0 | 0.2 | Yes | "Speed" |
| drift | float | 1.0-3.0 | 2.0 | No | "Drift" |

### Constants

- Enum value: `TRANSFORM_INTERFERENCE_WARP`
- Display name: `"Interference Warp"`
- Category: `TRANSFORM_CATEGORY_WARP`
- Config member: `interferenceWarp`

---

## Tasks

### Wave 1: Config Header

Creates the struct that all other files depend on.

#### Task 1.1: Create config header

**Files**: `src/config/interference_warp_config.h`
**Creates**: `InterferenceWarpConfig` struct

**Build**:
1. Create file with include guard `INTERFERENCE_WARP_CONFIG_H`
2. Add struct comment explaining the effect
3. Define `InterferenceWarpConfig` with all fields from Types section above
4. Use exact field names, types, and defaults from specification

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks modify different files with no overlap. Run in parallel.

#### Task 2.1: Register effect in effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "interference_warp_config.h"` with other config includes
2. Add `TRANSFORM_INTERFERENCE_WARP,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add case in `TransformEffectName()`: `case TRANSFORM_INTERFERENCE_WARP: return "Interference Warp";`
4. Add `TRANSFORM_INTERFERENCE_WARP` to `TransformOrderConfig::order` array (before closing brace)
5. Add `InterferenceWarpConfig interferenceWarp;` member to `EffectConfig` struct
6. Add case in `IsTransformEnabled()`: `case TRANSFORM_INTERFERENCE_WARP: return e->interferenceWarp.enabled;`

**Verify**: Compiles.

#### Task 2.2: Create fragment shader

**Files**: `shaders/interference_warp.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create shader file with `#version 330`
2. Declare inputs: `in vec2 fragTexCoord;`
3. Declare output: `out vec4 finalColor;`
4. Declare uniforms:
   - `uniform sampler2D texture0;`
   - `uniform float time;`
   - `uniform float amplitude;`
   - `uniform float scale;`
   - `uniform int axes;`
   - `uniform float axisRotation;`
   - `uniform int harmonics;`
   - `uniform float decay;`
   - `uniform float drift;`
5. Define PI and TAU constants
6. Implement `psi()` function from Algorithm section
7. Implement `omega()` function from Algorithm section
8. Implement `phi()` function from Algorithm section
9. Implement `main()` from Algorithm section with boundary mirroring

**Verify**: Shader file exists.

#### Task 2.3: Add PostEffect shader members

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `post_effect.h`:
1. Add shader member: `Shader interferenceWarpShader;`
2. Add time accumulator: `float interferenceWarpTime;`
3. Add uniform locations:
   - `int interferenceWarpTimeLoc;`
   - `int interferenceWarpAmplitudeLoc;`
   - `int interferenceWarpScaleLoc;`
   - `int interferenceWarpAxesLoc;`
   - `int interferenceWarpAxisRotationLoc;`
   - `int interferenceWarpHarmonicsLoc;`
   - `int interferenceWarpDecayLoc;`
   - `int interferenceWarpDriftLoc;`

In `post_effect.cpp`:
1. In `LoadPostEffectShaders()`: add `pe->interferenceWarpShader = LoadShader(0, "shaders/interference_warp.fs");`
2. Add to success check: `&& pe->interferenceWarpShader.id != 0`
3. In `GetShaderUniformLocations()`: get all uniform locations (time, amplitude, scale, axes, axisRotation, harmonics, decay, drift)
4. In `PostEffectUninit()`: add `UnloadShader(pe->interferenceWarpShader);`
5. Initialize `pe->interferenceWarpTime = 0.0f;` in `PostEffectInit()`

**Verify**: Compiles.

#### Task 2.4: Implement shader setup

**Files**: `src/render/shader_setup_warp.h`, `src/render/shader_setup_warp.cpp`, `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `shader_setup_warp.h`:
1. Add declaration: `void SetupInterferenceWarp(PostEffect *pe);`

In `shader_setup_warp.cpp`:
1. Implement `SetupInterferenceWarp()`:
   - Get config pointer: `const InterferenceWarpConfig *iw = &pe->effects.interferenceWarp;`
   - Accumulate time: `pe->interferenceWarpTime += pe->currentDeltaTime * iw->speed;`
   - Set all uniforms using `SetShaderValue()` for time, amplitude, scale, axes, axisRotation, harmonics, decay, drift

In `shader_setup.cpp`:
1. Add dispatch case in `GetTransformEffect()`:
   ```cpp
   case TRANSFORM_INTERFERENCE_WARP:
       return { &pe->interferenceWarpShader, SetupInterferenceWarp, &pe->effects.interferenceWarp.enabled };
   ```

**Verify**: Compiles.

#### Task 2.5: Add UI controls

**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `imgui_effects.cpp`:
1. Add case in `GetTransformCategory()`: `case TRANSFORM_INTERFERENCE_WARP: return TRANSFORM_CATEGORY_WARP;`

In `imgui_effects_warp.cpp`:
1. Add static bool: `static bool sectionInterferenceWarp = false;`
2. Add helper function `DrawWarpInterferenceWarp()`:
   - Use `DrawSectionBegin("Interference Warp", categoryGlow, &sectionInterferenceWarp)`
   - Enabled checkbox with `MoveTransformToEnd` pattern
   - `ModulatableSlider` for amplitude (0.0-0.5, "%.3f")
   - `ImGui::SliderFloat` for scale (0.5-10.0, "%.1f")
   - `ImGui::SliderInt` for axes (2-8)
   - `ModulatableSliderAngleDeg` for axisRotation
   - `ImGui::SliderInt` for harmonics (8-256)
   - `ImGui::SliderFloat` for decay (0.5-2.0, "%.2f")
   - `ModulatableSlider` for speed (0.0-2.0, "%.2f")
   - `ImGui::SliderFloat` for drift (1.0-3.0, "%.2f")
   - `DrawSectionEnd()`
3. Add call in `DrawWarpCategory()`: `ImGui::Spacing(); DrawWarpInterferenceWarp(e, modSources, categoryGlow);`

**Verify**: Compiles.

#### Task 2.6: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterferenceWarpConfig, enabled, amplitude, scale, axes, axisRotation, harmonics, decay, speed, drift)`
2. Add to `to_json()`: `if (e.interferenceWarp.enabled) { j["interferenceWarp"] = e.interferenceWarp; }`
3. Add to `from_json()`: `e.interferenceWarp = j.value("interferenceWarp", e.interferenceWarp);`

**Verify**: Compiles.

#### Task 2.7: Register modulatable parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add to `PARAM_TABLE` (maintain alphabetical order within warp section):
   ```cpp
   {"interferenceWarp.amplitude", {0.0f, 0.5f}},
   {"interferenceWarp.axisRotation", {0.0f, ROTATION_OFFSET_MAX}},
   {"interferenceWarp.speed", {0.0f, 2.0f}},
   ```
2. Add corresponding pointers to `targets[]` array at matching indices:
   ```cpp
   &effects->interferenceWarp.amplitude,
   &effects->interferenceWarp.axisRotation,
   &effects->interferenceWarp.speed,
   ```

**Verify**: Compiles.

---

## Final Verification

After all waves complete:
- [ ] Build succeeds: `cmake.exe --build build`
- [ ] No compiler warnings
- [ ] Effect appears in Warp category UI
- [ ] Effect shows "Warp" badge in transform pipeline list
- [ ] Enabling effect moves it to end of pipeline
- [ ] Sliders modify effect in real-time
- [ ] Pattern changes with axes (2=rectangular, 3=triangular, 5=quasicrystal)
- [ ] Harmonics slider changes detail level
- [ ] Speed slider controls animation rate
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to amplitude, axisRotation, speed
