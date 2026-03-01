# Wave Warp

Generalize the existing Triangle Wave Warp into a multi-mode warp effect. Replace the hardcoded triangle basis function with a selectable wave type (triangle, sine, sawtooth, square), and expose `octaves` and `scale` as new parameters. This is an in-place rename: delete `triangle_wave_warp.*`, create `wave_warp.*`, update all integration points.

**Research**: `docs/research/wave-warp.md`

## Design

### Types

**WaveWarpConfig** (`src/effects/wave_warp.h`):

```
int waveType = 0           // Basis function: 0=tri, 1=sine, 2=saw, 3=square
float strength = 0.4f      // Displacement magnitude (0.0-1.0)
float speed = 4.5f         // Crease rotation rate rad/s (0.0-10.0)
int octaves = 4            // fBM iteration count (1-6)
float scale = 0.8f         // Initial noise frequency (0.1-4.0)
```

**WaveWarpEffect** (`src/effects/wave_warp.h`):

```
Shader shader
int waveTypeLoc
int strengthLoc
int timeLoc
int octavesLoc
int scaleLoc
float time               // Accumulated animation time
```

### Algorithm

The shader replaces the hardcoded `tri()` with a selectable `wave()` function. The `waveType` uniform selects the basis via flat branching (zero divergence — all fragments take the same path).

#### Wave basis functions

```glsl
uniform int waveType; // 0=triangle, 1=sine, 2=sawtooth, 3=square

float wave(float x) {
    if (waveType == 0) return abs(fract(x) - 0.5);           // triangle
    if (waveType == 1) return sin(x * 6.283185) * 0.5 + 0.5; // sine
    if (waveType == 2) return fract(x);                        // sawtooth
    return step(0.5, fract(x));                                // square
}

vec2 wave2(vec2 p) {
    return vec2(wave(p.x + wave(p.y * 2.0)), wave(p.y + wave(p.x * 2.0)));
}
```

All four functions map to [0, 1] range with period 1 — drop-in replacements for `tri()`.

#### fBM noise function

Replace `triangleNoise` with `waveNoise`. Parameterize the initial scale and loop bound:

```glsl
uniform int octaves;    // replaces hardcoded loop bound 3
uniform float scale;    // replaces hardcoded 0.8

float waveNoise(vec2 p) {
    float z  = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2  bp = p * scale;                                  // was p * 0.8
    for (int i = 0; i < octaves; i++) {                    // was i <= 3
        vec2 dg = wave2(bp * 2.0) * 0.5;
        dg *= rot(time);
        p  += dg / z2;
        bp *= 1.5;
        z2 *= 0.6;
        z  *= 1.7;
        p  *= 1.2;
        p  *= mat2(0.970, 0.242, -0.242, 0.970);
        rz += wave(p.x + wave(p.y)) / z;
    }
    return rz;
}
```

#### main() — unchanged logic

```glsl
void main() {
    vec2 uv = fragTexCoord;
    vec2 p  = (uv - 0.5) * 2.0;

    float nz = clamp(waveNoise(p), 0.0, 1.0);
    p *= 1.0 + (nz - 0.5) * strength;                     // was noiseStrength

    uv = p * 0.5 + 0.5;
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveType | int | 0-3 | 0 | No | Combo: "Wave Type" with items "Triangle\0Sine\0Sawtooth\0Square\0" |
| strength | float | 0.0-1.0 | 0.4 | Yes | "Strength##waveWarp" |
| speed | float | 0.0-10.0 | 4.5 | Yes | "Speed##waveWarp" |
| octaves | int | 1-6 | 4 | No | SliderInt: "Octaves##waveWarp" |
| scale | float | 0.1-4.0 | 0.8 | Yes | "Scale##waveWarp" |

### Constants

- Enum: `TRANSFORM_WAVE_WARP` (replaces `TRANSFORM_TRIANGLE_WAVE_WARP`)
- Display name: `"Wave Warp"`
- Badge: `"WARP"`, section `1`
- Flags: `EFFECT_FLAG_NONE`
- Config fields macro: `WAVE_WARP_CONFIG_FIELDS`
- Mod param prefix: `"waveWarp."`

### Rename Map

| Old | New |
|-----|-----|
| `shaders/triangle_wave_warp.fs` | `shaders/wave_warp.fs` |
| `src/effects/triangle_wave_warp.h` | `src/effects/wave_warp.h` |
| `src/effects/triangle_wave_warp.cpp` | `src/effects/wave_warp.cpp` |
| `TriangleWaveWarpConfig` | `WaveWarpConfig` |
| `TriangleWaveWarpEffect` | `WaveWarpEffect` |
| `TRANSFORM_TRIANGLE_WAVE_WARP` | `TRANSFORM_WAVE_WARP` |
| `TRIANGLE_WAVE_WARP_CONFIG_FIELDS` | `WAVE_WARP_CONFIG_FIELDS` |
| `triangleWaveWarp` (field) | `waveWarp` |
| `"Tri-Wave Warp"` | `"Wave Warp"` |
| `"triangleWaveWarp.*"` (mod params) | `"waveWarp.*"` |

---

## Tasks

### Wave 0: Delete Old Files

#### Task 0.1: Delete old triangle_wave_warp files

**Files**: `src/effects/triangle_wave_warp.h` (delete), `src/effects/triangle_wave_warp.cpp` (delete), `shaders/triangle_wave_warp.fs` (delete)

**Do**: Delete all three old files. They will be fully replaced by new files in Wave 1-2.

**Verify**: Files no longer exist.

---

### Wave 1: Header

#### Task 1.1: Create wave_warp.h

**Files**: `src/effects/wave_warp.h` (create)
**Creates**: `WaveWarpConfig`, `WaveWarpEffect`, `WAVE_WARP_CONFIG_FIELDS` macro, function declarations

**Do**: Create the header following the same structure as the old `triangle_wave_warp.h`. Use the Types section above for struct layouts. Define:
- `WaveWarpConfig` with fields: `enabled`, `waveType`, `strength`, `speed`, `octaves`, `scale` (types, defaults, range comments from Design)
- `WAVE_WARP_CONFIG_FIELDS` macro listing all serializable fields: `enabled, waveType, strength, speed, octaves, scale`
- `WaveWarpEffect` typedef struct with: `shader`, `waveTypeLoc`, `strengthLoc`, `timeLoc`, `octavesLoc`, `scaleLoc`, `time`
- 5 function declarations: `WaveWarpEffectInit`, `WaveWarpEffectSetup`, `WaveWarpEffectUninit`, `WaveWarpConfigDefault`, `WaveWarpRegisterParams`

Include guard: `WAVE_WARP_H`. Includes: `"raylib.h"`, `<stdbool.h>`.

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2, but header parses).

---

### Wave 2: Implementation, Shader, Integration

#### Task 2.1: Create wave_warp.cpp

**Files**: `src/effects/wave_warp.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation following the same structure as the old `triangle_wave_warp.cpp`. Include order per conventions: own header, project headers (`automation/mod_sources.h`, `automation/modulation_engine.h`, `config/effect_descriptor.h`, `render/post_effect.h`), UI headers (`imgui.h`, `ui/modulatable_slider.h`), system headers (`<stddef.h>`).

Functions:
- `WaveWarpEffectInit`: Load `shaders/wave_warp.fs`, cache all 5 uniform locations (`waveType`, `strength`, `time`, `octaves`, `scale`), init `time = 0.0f`
- `WaveWarpEffectSetup`: Accumulate `time += cfg->speed * deltaTime`. Set all uniforms — `waveType` and `octaves` use `SHADER_UNIFORM_INT`, the rest use `SHADER_UNIFORM_FLOAT`
- `WaveWarpEffectUninit`: `UnloadShader`
- `WaveWarpConfigDefault`: `return WaveWarpConfig{};`
- `WaveWarpRegisterParams`: Register `waveWarp.strength` (0-1), `waveWarp.speed` (0-10), `waveWarp.scale` (0.1-4)

UI section (`// === UI ===`):
- `static void DrawWaveWarpParams(EffectConfig*, const ModSources*, ImU32)`:
  - `ImGui::Combo("Wave Type##waveWarp", &cfg->waveType, "Triangle\0Sine\0Sawtooth\0Square\0")`
  - `ModulatableSlider("Strength##waveWarp", ..., "waveWarp.strength", "%.2f", ms)`
  - `ModulatableSlider("Speed##waveWarp", ..., "waveWarp.speed", "%.1f", ms)`
  - `ImGui::SliderInt("Octaves##waveWarp", &cfg->octaves, 1, 6)`
  - `ModulatableSlider("Scale##waveWarp", ..., "waveWarp.scale", "%.2f", ms)`

Bridge function (non-static):
- `void SetupWaveWarp(PostEffect* pe)` — calls `WaveWarpEffectSetup(&pe->waveWarp, &pe->effects.waveWarp, pe->currentDeltaTime)`

Registration:
```
REGISTER_EFFECT(TRANSFORM_WAVE_WARP, WaveWarp, waveWarp,
                "Wave Warp", "WARP", 1, EFFECT_FLAG_NONE,
                SetupWaveWarp, NULL, DrawWaveWarpParams)
```

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.2: Create wave_warp.fs shader

**Files**: `shaders/wave_warp.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section above. The shader must include:
- Attribution header (same as old shader — "Filaments" by nimitz, CC BY-NC-SA 3.0, modified description updated to mention multi-mode wave basis)
- `#version 330`
- Uniforms: `texture0`, `strength`, `time`, `waveType` (int), `octaves` (int), `scale` (float)
- `wave()` function with 4 branches per Algorithm section
- `wave2()` function using `wave()` instead of `tri()`
- `rot()` function (unchanged from reference)
- `waveNoise()` function using `scale` and `octaves` per Algorithm section
- `main()` with radial displacement using `strength` per Algorithm section

**Verify**: File exists, valid GLSL 330.

---

#### Task 2.3: Update integration points

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Replace all `triangle_wave_warp` references with `wave_warp` equivalents:

1. **`src/config/effect_config.h`**:
   - Replace `#include "effects/triangle_wave_warp.h"` with `#include "effects/wave_warp.h"`
   - Replace `TRANSFORM_TRIANGLE_WAVE_WARP,` with `TRANSFORM_WAVE_WARP,` in the enum
   - Replace `TRANSFORM_TRIANGLE_WAVE_WARP` with `TRANSFORM_WAVE_WARP` in the `TransformOrderConfig::order` array
   - Replace `TriangleWaveWarpConfig triangleWaveWarp;` with `WaveWarpConfig waveWarp;` in `EffectConfig`

2. **`src/render/post_effect.h`**:
   - Replace `#include "effects/triangle_wave_warp.h"` with `#include "effects/wave_warp.h"`
   - Replace `TriangleWaveWarpEffect triangleWaveWarp;` with `WaveWarpEffect waveWarp;` in `PostEffect`

3. **`CMakeLists.txt`**:
   - Replace `src/effects/triangle_wave_warp.cpp` with `src/effects/wave_warp.cpp` in `EFFECTS_SOURCES`

4. **`src/config/effect_serialization.cpp`**:
   - Replace `#include "effects/triangle_wave_warp.h"` with `#include "effects/wave_warp.h"`
   - Replace `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriangleWaveWarpConfig, TRIANGLE_WAVE_WARP_CONFIG_FIELDS)` with `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveWarpConfig, WAVE_WARP_CONFIG_FIELDS)`
   - Replace `X(triangleWaveWarp)` with `X(waveWarp)` in `EFFECT_CONFIG_FIELDS`

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Old `triangle_wave_warp.*` files are deleted
- [ ] Effect appears as "Wave Warp" with "WARP" badge in UI
- [ ] Wave Type combo switches between Triangle/Sine/Sawtooth/Square
- [ ] Octaves slider adjusts detail level (1-6)
- [ ] Scale slider adjusts noise frequency
- [ ] Strength and Speed behave identically to old effect at default settings
- [ ] Preset save/load works with new field names
- [ ] Modulation routes to strength, speed, scale
