# Triangle-Wave fBM Domain Warp

Radial domain warp driven by nimitz's triangle-wave fBM noise. The noise produces sharp creases rather than smooth hills, and an internal rotation animates the crease directions for a slowly churning organic feel. Displacement is multiplicative-radial — pixels are pushed/pulled from screen center based on noise value.

**Research**: `docs/research/triangle-wave-warp.md`

## Design

### Types

**TriangleWaveWarpConfig** (`src/effects/triangle_wave_warp.h`):

```cpp
struct TriangleWaveWarpConfig {
  bool enabled = false;
  float noiseStrength = 0.4f; // Displacement magnitude (0.0-1.0)
  float noiseSpeed = 4.5f;    // Crease rotation rate rad/s (0.0-10.0)
};
```

**TriangleWaveWarpEffect** (`src/effects/triangle_wave_warp.h`):

```cpp
typedef struct TriangleWaveWarpEffect {
  Shader shader;
  int noiseStrengthLoc;
  int timeLoc;
  float time; // Accumulated animation time
} TriangleWaveWarpEffect;
```

### Algorithm

The shader implements triangle-wave fBM noise — a fractal built from `abs(fract(x) - 0.5)` instead of sine or Perlin. Four fixed octaves with internal crease rotation driven by `time`.

UV is centered at screen center and scaled to -1..1 before noise sampling. After noise lookup, coordinates are radially scaled by `1.0 + (nz - 0.5) * noiseStrength`, then converted back to UV.

```glsl
#version 330

// Triangle-Wave fBM Domain Warp
// Based on "Filaments" by nimitz (stormoid)
// https://www.shadertoy.com/view/4lcSWs
// License: CC BY-NC-SA 3.0 Unported
// Modified: Extracted noise warp as standalone radial post-process effect

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float noiseStrength; // Displacement magnitude (0.0-1.0)
uniform float time;          // Crease rotation angle (accumulated on CPU)

float tri(float x) { return abs(fract(x) - 0.5); }

vec2 tri2(vec2 p) {
    return vec2(tri(p.x + tri(p.y * 2.0)), tri(p.y + tri(p.x * 2.0)));
}

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

float triangleNoise(vec2 p) {
    float z  = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2  bp = p * 0.8;
    for (int i = 0; i <= 3; i++) {
        vec2 dg = tri2(bp * 2.0) * 0.5;
        dg *= rot(time);                           // animate crease directions
        p  += dg / z2;                             // warp by crease gradient
        bp *= 1.5;
        z2 *= 0.6;
        z  *= 1.7;
        p  *= 1.2;
        p  *= mat2(0.970, 0.242, -0.242, 0.970);  // per-octave rotation
        rz += tri(p.x + tri(p.y)) / z;
    }
    return rz;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p  = (uv - 0.5) * 2.0;                   // center at screen center, -1..1

    float nz = clamp(triangleNoise(p), 0.0, 1.0);
    p *= 1.0 + (nz - 0.5) * noiseStrength;        // radial push/pull

    uv = p * 0.5 + 0.5;                            // back to UV
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| noiseStrength | float | 0.0-1.0 | 0.4 | Yes | `"Strength##triWaveWarp"` |
| noiseSpeed | float | 0.0-10.0 | 4.5 | Yes | `"Speed##triWaveWarp"` |

### Constants

- Enum: `TRANSFORM_TRIANGLE_WAVE_WARP`
- Display name: `"Tri-Wave Warp"`
- Category badge: `"WARP"`
- Section index: `1`
- Config field: `triangleWaveWarp`
- Effect field: `triangleWaveWarp`
- Config fields macro: `TRIANGLE_WAVE_WARP_CONFIG_FIELDS`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/triangle_wave_warp.h` (create)
**Creates**: Config and Effect structs that all other tasks depend on

**Do**: Create the header following the domain_warp.h pattern. Define `TriangleWaveWarpConfig`, `TRIANGLE_WAVE_WARP_CONFIG_FIELDS`, `TriangleWaveWarpEffect`, and 5 public function declarations (Init, Setup, Uninit, ConfigDefault, RegisterParams). Use the Types section above for struct layouts.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker deps yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create effect implementation

**Files**: `src/effects/triangle_wave_warp.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following domain_warp.cpp as pattern. Include order per conventions. Key details:
- `Init`: Load `shaders/triangle_wave_warp.fs`, cache `noiseStrengthLoc` and `timeLoc`, init `time = 0.0f`
- `Setup`: Accumulate `e->time += cfg->noiseSpeed * deltaTime`, bind `noiseStrength` and `time` uniforms
- `Uninit`: Unload shader
- `RegisterParams`: Register `"triangleWaveWarp.noiseStrength"` (0.0-1.0) and `"triangleWaveWarp.noiseSpeed"` (0.0-10.0)
- `ConfigDefault`: Return `TriangleWaveWarpConfig{}`
- UI section: `DrawTriangleWaveWarpParams` with two `ModulatableSlider` calls — Strength (`"%.2f"`) and Speed (`"%.1f"`)
- Bridge: `SetupTriangleWaveWarp(PostEffect* pe)` (non-static)
- Registration: `REGISTER_EFFECT(TRANSFORM_TRIANGLE_WAVE_WARP, TriangleWaveWarp, triangleWaveWarp, "Tri-Wave Warp", "WARP", 1, EFFECT_FLAG_NONE, SetupTriangleWaveWarp, NULL, DrawTriangleWaveWarpParams)`

**Verify**: Compiles (needs Tasks 2.2 and 2.3 for full build).

---

#### Task 2.2: Create shader

**Files**: `shaders/triangle_wave_warp.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section above verbatim. The attribution comment block is already included in the Algorithm section — copy it as the shader file header before `#version 330`.

**Verify**: File exists with correct uniforms (`noiseStrength`, `time`).

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Four small integration edits:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/triangle_wave_warp.h"` with the other effect includes
   - Add `TRANSFORM_TRIANGLE_WAVE_WARP,` before `TRANSFORM_EFFECT_COUNT` in the enum
   - Add `TriangleWaveWarpConfig triangleWaveWarp;` to `EffectConfig` struct (near the other warp configs)

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/triangle_wave_warp.h"` with the other effect includes
   - Add `TriangleWaveWarpEffect triangleWaveWarp;` to `PostEffect` struct (near the other warp effects)

3. **`CMakeLists.txt`**:
   - Add `src/effects/triangle_wave_warp.cpp` to `EFFECTS_SOURCES` (alphabetical, near other warp entries)

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/triangle_wave_warp.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriangleWaveWarpConfig, TRIANGLE_WAVE_WARP_CONFIG_FIELDS)`
   - Add `X(triangleWaveWarp) \` to `EFFECT_CONFIG_FIELDS(X)` macro

**Verify**: `cmake.exe --build build` compiles with all Wave 2 tasks complete.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Warp section of effects sidebar
- [ ] Strength and Speed sliders respond in real-time
- [ ] Noise pattern churns organically when Speed > 0
- [ ] Preset save/load round-trips correctly
