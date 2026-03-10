# Shard Crush

Shatters the input image into noise-rotated angular blocks with prismatic chromatic aberration. Each iteration samples the input texture at a slightly offset position and tints with a spectral color, accumulating a rainbow-fringed mosaic of displaced fragments. Transform counterpart to the Digital Shard generator — same noise-rotation geometry, but applied to the input image instead of procedural color.

**Research**: `docs/research/shard_crush.md`

## Design

### Types

**ShardCrushConfig** (`src/effects/shard_crush.h`):
```
bool enabled = false;
int iterations = 100;           // Loop count (20-100)
float zoom = 0.4f;              // Base coordinate scale (0.1-2.0)
float aberrationSpread = 0.01f; // Per-iteration UV offset (0.001-0.05)
float noiseScale = 64.0f;       // Noise tiling scale (16.0-256.0)
float rotationLevels = 8.0f;    // Angle quantization steps (2.0-16.0)
float softness = 0.0f;          // Hard binary to smooth gradient (0.0-1.0)
float speed = 1.0f;             // Animation rate (0.1-5.0)
float mix = 1.0f;               // Blend with original (0.0-1.0)
```

**ShardCrushEffect** (`src/effects/shard_crush.h`):
```
Shader shader;
int resolutionLoc;
int timeLoc;
int iterationsLoc;
int zoomLoc;
int aberrationSpreadLoc;
int noiseScaleLoc;
int rotationLevelsLoc;
int softnessLoc;
int mixLoc;       // maps to shader uniform "mixAmount" (avoids GLSL mix() collision)
float time;
```

### Algorithm

Same noise-rotation loop as Digital Shard, but instead of accumulating a scalar mask, each iteration displaces UV coordinates and samples the input texture with a spectral tint.

The shader samples `texture0` (the transform pipeline input). No gradient LUT, no FFT texture.

**Noise source**: Procedural cell noise hash — NOT the shared noise texture. The codebase's `NoiseTextureGet()` is 1024×1024 with trilinear filtering; this algorithm requires nearest-filtered 64×64 noise. Use `floor()` to quantize to integer cells:

```glsl
float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
#define N(x) hash21(floor((x) * 64.0 / noiseScale))
```

Same hash and macro as `shaders/digital_shard.fs`.

**Core loop** — per iteration `j` from `0` to `iterations-1`:

```glsl
float invIter = 1.0 / float(iterations);
vec3 O = vec3(0.0);

for (int j = 0; j < iterations; j++) {
    // Map j to i in [-1, +1]
    float i = float(j) * invIter * 2.0 - 1.0;

    // Centered coordinates with per-iteration aberration offset
    vec2 c = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0)
             / (zoom + i * aberrationSpread);

    // Noise-driven coordinate division (same as Digital Shard)
    c /= (0.1 + N(c));

    // Quantized rotation matrix
    float angle = ceil(N(c) * rotationLevels) * angleStep;
    vec2 rotated = c * mat2(cos(angle + vec4(0, 33, 11, 0)));

    // Frequency stripe mask
    float cosVal = cos(rotated.x / N(N(c) + ceil(c) + time));

    // Softness: smoothstep with epsilon to avoid degenerate edges at softness=0
    float mask = smoothstep(-softness, softness + 0.001, cosVal);

    // Recover UV from displaced coordinates for texture sampling
    vec2 sampleUV = c * (zoom + i * aberrationSpread)
                    / vec2(resolution.x / resolution.y, 1.0) + 0.5;
    sampleUV = clamp(sampleUV, 0.0, 1.0);

    // Sample input texture at displaced UV
    vec3 texSample = texture(texture0, sampleUV).rgb;

    // Spectral tint: red at i=-1, green at i=0, blue at i=1
    vec3 spectral = vec3(1.0 + i, 2.0 - abs(2.0 * i), 1.0 - i);

    // Accumulate tinted, masked sample
    O += texSample * spectral * mask * invIter;
}

// Mix with original
vec3 original = texture(texture0, fragTexCoord).rgb;
finalColor = vec4(mix(original, O, mixAmount), 1.0);
```

**Key constants** (same geometric meaning as reference):
- `0.4` → `zoom`, `0.01` → `aberrationSpread`, `8.0` → `rotationLevels`, `64.0` → `noiseScale`
- `angleStep = 2π / rotationLevels`

**Coordinate note**: `fragTexCoord - 0.5` centers the origin. The `* vec2(r.x/r.y, 1.0)` corrects aspect ratio. After displacement, reverse these transforms to recover a valid `[0,1]` UV for texture sampling. Clamp to `[0,1]` to avoid edge artifacts.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| iterations | int | 20-100 | 100 | No | Iterations |
| zoom | float | 0.1-2.0 | 0.4 | Yes | Zoom |
| aberrationSpread | float | 0.001-0.05 | 0.01 | Yes | Aberration |
| noiseScale | float | 16.0-256.0 | 64.0 | Yes | Noise Scale |
| rotationLevels | float | 2.0-16.0 | 8.0 | Yes | Rotation Levels |
| softness | float | 0.0-1.0 | 0.0 | Yes | Softness |
| speed | float | 0.1-5.0 | 1.0 | Yes | Speed |
| mix | float | 0.0-1.0 | 1.0 | Yes | Mix |

### Constants

- Enum name: `TRANSFORM_SHARD_CRUSH`
- Display name: `"Shard Crush"`
- Category badge: `"RET"`
- Category section: `6` (Retro)
- Flags: `EFFECT_FLAG_NONE`
- Macro: `REGISTER_EFFECT`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create ShardCrush header

**Files**: `src/effects/shard_crush.h` (create)
**Creates**: `ShardCrushConfig` struct, `ShardCrushEffect` typedef, function declarations

**Do**: Create the header following `ascii_art.h` as pattern. Define:
- `ShardCrushConfig` with all fields from the Design > Types section (defaults and range comments inline)
- `SHARD_CRUSH_CONFIG_FIELDS` macro listing all serializable fields
- `ShardCrushEffect` typedef struct with shader handle, uniform location ints (one per uniform from the Design > Types section), and `float time` accumulator
- Declare 5 public functions: `ShardCrushEffectInit`, `ShardCrushEffectSetup` (takes `const ShardCrushConfig* cfg, float deltaTime`), `ShardCrushEffectUninit`, `ShardCrushConfigDefault`, `ShardCrushRegisterParams`
- Include `"raylib.h"` and `<stdbool.h>`
- Guard with `#ifndef SHARD_CRUSH_H`

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create ShardCrush implementation

**Files**: `src/effects/shard_crush.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation following `ascii_art.cpp` as pattern. Include groups per conventions (transform effect `.cpp` with colocated UI).

Core logic:
- `ShardCrushEffectInit`: Load `shaders/shard_crush.fs`, cache all uniform locations from the Effect struct, init `time = 0.0f`. Return false if shader fails. Note: `mixLoc` maps to shader uniform `"mixAmount"` (avoids GLSL `mix()` name collision).
- `ShardCrushEffectSetup`: Advance `time += speed * deltaTime`, wrap with `fmodf(time, 1000.0f)`. Bind all uniforms. Bind `resolution` as `vec2{GetScreenWidth(), GetScreenHeight()}`.
- `ShardCrushEffectUninit`: `UnloadShader`.
- `ShardCrushConfigDefault`: Return `ShardCrushConfig{}`.
- `ShardCrushRegisterParams`: Register all modulatable params (zoom, aberrationSpread, noiseScale, rotationLevels, softness, speed, mix) with `"shardCrush."` prefix and ranges matching the config struct comments.

Bridge function (NOT static): `SetupShardCrush(PostEffect* pe)` — calls `ShardCrushEffectSetup(&pe->shardCrush, &pe->effects.shardCrush, pe->currentDeltaTime)`.

Colocated UI section (`// === UI ===`):
- `static void DrawShardCrushParams(EffectConfig*, const ModSources*, ImU32)` — draws:
  - `ImGui::SliderInt("Iterations##shardCrush", &cfg->iterations, 20, 100)`
  - `ModulatableSlider` for zoom (`"%.2f"`), aberration (`"%.3f"`), noiseScale (`"%.1f"`), rotationLevels (`"%.1f"`), softness (`"%.2f"`), speed (`"%.2f"`), mix (`"%.2f"`)

Registration macro at bottom:
```cpp
// clang-format off
REGISTER_EFFECT(TRANSFORM_SHARD_CRUSH, ShardCrush, shardCrush,
                "Shard Crush", "RET", 6, EFFECT_FLAG_NONE,
                SetupShardCrush, NULL, DrawShardCrushParams)
// clang-format on
```

**Verify**: Compiles (after all Wave 2 tasks).

---

#### Task 2.2: Create ShardCrush shader

**Files**: `shaders/shard_crush.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Create the fragment shader. Begin with attribution block:
```glsl
// Based on "Gltch" by Xor (@XorDev)
// https://www.shadertoy.com/view/mdfGRs
// License: CC BY-NC-SA 3.0 Unported
// Modified: samples input texture with spectral tinting instead of procedural color
```

Then `#version 330`, standard `in vec2 fragTexCoord; out vec4 finalColor;`.

Uniforms: `texture0` (sampler2D), `resolution` (vec2), `time` (float), `iterations` (int), `zoom` (float), `aberrationSpread` (float), `noiseScale` (float), `rotationLevels` (float), `softness` (float), `mixAmount` (float).

Implement the Algorithm section from this plan exactly. Key points:
- `hash21` and `#define N(x)` identical to `digital_shard.fs`
- Loop from `j=0` to `iterations-1`, mapping `j` to `i` in `[-1, +1]`
- Center coordinates: `(fragTexCoord - 0.5) * vec2(r.x/r.y, 1.0)`
- Recover sample UV by reversing the centering/aspect transforms, clamp to `[0,1]`
- Spectral tint: `vec3(1.0+i, 2.0-abs(2.0*i), 1.0-i)` — hardcoded, not configurable
- Final mix: `mix(original, O, mixAmount)`

**Verify**: Shader file exists with correct attribution.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/shard_crush.h"` with the other effect includes
2. Add `TRANSFORM_SHARD_CRUSH,` enum entry — place it near the other Retro effects (after `TRANSFORM_GLITCH` or near `TRANSFORM_CRT`). Must be before `TRANSFORM_ACCUM_COMPOSITE` and `TRANSFORM_EFFECT_COUNT`.
3. Add `ShardCrushConfig shardCrush;` member to the `EffectConfig` struct with a comment `// Shard Crush (angular block shattering with chromatic aberration)`

The order array auto-populates from enum indices (constructor loops `0..TRANSFORM_EFFECT_COUNT-1`), so no manual order edit needed.

**Verify**: Compiles.

---

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/shard_crush.h"` with the other effect includes
2. Add `ShardCrushEffect shardCrush;` member to the `PostEffect` struct

**Verify**: Compiles.

---

#### Task 2.5: Add to build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/shard_crush.cpp` to the `EFFECTS_SOURCES` list (alphabetical position).

**Verify**: Compiles.

---

#### Task 2.6: Add serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/shard_crush.h"` with the other effect includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShardCrushConfig, SHARD_CRUSH_CONFIG_FIELDS)` with the other per-config JSON macros
3. Add `X(shardCrush) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Implementation Notes

### Staggered cell noise animation (applied to both shard_crush.fs and digital_shard.fs)

The original `hash21` produced static cells — block boundaries and values never changed over time. Three approaches were tried:

1. **Linear time offset in noise coords** (`+ vec2(time * 0.3, ...)`): Caused the entire grid to visibly drift toward bottom-left. Rejected.
2. **Oscillating offset** (`+ vec2(sin(time), cos(time)) * 2.0`): Looked like a smooth grid sliding over the image — wrong aesthetic for a hard-edged glitch effect. Rejected.
3. **Discrete per-cell phase** (`floor(time + phase)` in hash seed): Each cell gets a random phase from its position (`fract(sin(base * 1.731) * 21345.6789)`). Cells reshuffle at staggered times — individual blocks pop independently throughout each second. Hard snaps, no drift, no synchronized swapping. Accepted.

### UV displacement

The plan's UV recovery formula (`c * zoom / aspect + 0.5`) attempted to invert the centering/aspect transforms, but after the noise division (`c /= 0.1 + N(c)`) the coordinates are non-invertible — center blocks mapped back to near-original UV, producing no visible displacement. Final approach uses cell hash to drive UV offset direction and magnitude directly from the block's rotation angle, independent of screen position.

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Retro section of transform pipeline UI
- [ ] Effect shows "RET" category badge
- [ ] Enabling the effect shatters the input into prismatic angular blocks
- [ ] Iterations slider controls spectral spread / performance
- [ ] Softness transitions from hard binary blocks to smooth gradient
- [ ] Mix blends between original and crushed output
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
