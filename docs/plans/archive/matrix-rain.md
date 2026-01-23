# Matrix Rain

Falling columns of procedural rune glyphs with bright leading characters and fading green trails — the iconic "digital rain" from The Matrix. Overlays on existing content with configurable opacity, using CPU-accumulated time for smooth animation.

<!-- Intentional deviation from research: charStyle toggle (rune vs hash blocks) omitted — rune-only per user decision. -->

## Current State

- `docs/research/matrix-rain.md` - Algorithm reference with GLSL snippets for grid division, rune rendering, rain trails, and overlay compositing
- `src/config/ascii_art_config.h` - Closest config pattern (grid-based character effect with cellSize)
- `shaders/ascii_art.fs` - Closest shader pattern (cell division, character mask rendering)
- `src/render/shader_setup.cpp:967` - Pencil Sketch setup (reference for CPU time accumulation pattern)
- `src/ui/imgui_effects_style.cpp:368` - Last entry in Style category (new effect goes after it)
- `src/config/effect_config.h:95` - `TRANSFORM_PENCIL_SKETCH` enum (new enum goes after it, before `TRANSFORM_EFFECT_COUNT`)

## Technical Implementation

- **Source**: `docs/research/matrix-rain.md` — rune stroke algorithm, rain trail mask, overlay compositing
- **Pipeline Position**: Transform stage, Style category

### Core Algorithm

Single-pass grid-based effect. Divides screen into cells, renders procedural rune glyphs per cell, applies per-column falling brightness mask, composites over underlying image.

#### Grid Division

```glsl
vec2 cellSizeUV = vec2(cellSize) / resolution;
vec2 cellID = floor(uv / cellSizeUV);           // integer cell (column, row)
vec2 localUV = fract(uv / cellSizeUV);          // 0-1 within cell
float col = cellID.x;
float row = cellID.y;
```

#### Hash Functions

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec4 hash4(vec2 p) {
    return fract(sin(vec4(
        dot(p, vec2(127.1, 311.7)),
        dot(p, vec2(269.5, 183.3)),
        dot(p, vec2(419.2, 371.9)),
        dot(p, vec2(347.1, 247.5))
    )) * 43758.5453);
}
```

#### Procedural Rune Rendering

4 line segments per character, snapped to a 2×3 grid. Each stroke touches a different box edge:

```glsl
float rune_line(vec2 p, vec2 a, vec2 b) {
    p -= a; b -= a;
    float h = clamp(dot(p, b) / dot(b, b), 0.0, 1.0);
    return length(p - b * h);
}

float rune(vec2 uv, vec2 seed, float highlight) {
    float d = 1e5;
    for (int i = 0; i < 4; i++) {
        vec4 pos = hash4(seed);
        seed += 1.0;
        if (i == 0) pos.y = 0.0;
        if (i == 1) pos.x = 0.999;
        if (i == 2) pos.x = 0.0;
        if (i == 3) pos.y = 0.999;
        vec4 snaps = vec4(2, 3, 2, 3);
        pos = (floor(pos * snaps) + 0.5) / snaps;
        if (pos.xy != pos.zw)
            d = min(d, rune_line(uv, pos.xy, pos.zw + 0.001));
    }
    return smoothstep(0.1, 0.0, d) + highlight * smoothstep(0.4, 0.0, d);
}
```

#### Character Animation (Refresh)

Characters change over time. The `refreshRate` uniform controls base flicker speed. Lead character (closest to trail head) flickers rapidly; tail characters change slowly:

<!-- Intentional deviation from research: research shows refreshRate in parameter table but never integrates it into the algorithm. Plan uses it as a multiplier on animation time for user control. -->

```glsl
// charDist computed in rain trail mask loop (distance from nearest trail head)
float timeFactor;
if (charDist < 1.0) {
    timeFactor = floor(time * refreshRate * 5.0);
} else {
    float baseRate = hash(vec2(col, 2.0));
    float charRate = pow(hash(vec2(charDist, col)), 4.0);
    timeFactor = floor(time * refreshRate * (baseRate + charRate * 4.0));
}
vec2 charSeed = vec2(col + timeFactor, row);
```

#### Rain Trail Mask

Multiple fallers per column at different speeds create overlapping trails. Also tracks closest trail head distance (`charDist`) for character animation:

<!-- Intentional deviation from research: research uses mod() with magic constants and row*0.05 offset. Plan uses fract()-based wrapping over the full column height for clearer semantics. Linear fade replaces research's raw f value. leadBrightness boost on lead character is integrated here rather than as a separate tail-fade step. -->

```glsl
float brightness = 0.0;
float charDist = trailLength;  // distance to nearest trail head (for char animation)
float numRows = resolution.y / cellSize;
for (int i = 0; i < fallerCount; i++) {
    float fSpeed = hash(vec2(col, float(i))) * 0.7 + 0.3;
    float fOffset = hash(vec2(col, float(i) + 100.0));
    float headPos = fract(time * fSpeed + fOffset) * (numRows + trailLength);
    float dist = headPos - row;
    if (dist > 0.0 && dist < trailLength) {
        float t = dist / trailLength;
        float fade = 1.0 - t;
        brightness += fade;
        if (dist < 1.0) brightness += leadBrightness;
        charDist = min(charDist, dist);
    }
}
brightness = clamp(brightness, 0.0, 1.0 + leadBrightness);
```

#### Color and Compositing

Classic green gradient from bright white-green (lead) to dark green (tail). Overlay with configurable intensity:

<!-- Intentional deviation from research: research uses charIndex-based conditional for lead vs. tail color with a mid-green value. Plan uses brightness as a continuous interpolant between dark green and bright white-green, which produces a smoother gradient and integrates naturally with the multi-faller brightness accumulation. Tail fade (research's clamp formula) is subsumed by the linear fade in the brightness calculation. -->

```glsl
// Color: bright at head, dark at tail
vec3 rainColor = mix(vec3(0.0, 0.2, 0.0), vec3(0.67, 1.0, 0.82), clamp(brightness, 0.0, 1.0));

// Character mask modulates color
float charMask = rune(localUV, charSeed, clamp(brightness - 1.0, 0.0, 1.0));

// Overlay compositing
vec3 original = texture(texture0, fragTexCoord).rgb;
float rainAlpha = charMask * clamp(brightness, 0.0, 1.0) * overlayIntensity;
vec3 result = mix(original, rainColor, rainAlpha);
```

### Parameters

| Uniform | Type | Range | Purpose |
|---------|------|-------|---------|
| resolution | vec2 | — | Screen dimensions for cell math |
| cellSize | float | 4.0–32.0 | Character grid cell size in pixels |
| trailLength | float | 5.0–40.0 | Characters per rain strip |
| fallerCount | int | 1–20 | Independent rain drops per column |
| overlayIntensity | float | 0.0–1.0 | Rain opacity over underlying image |
| refreshRate | float | 0.1–5.0 | Character change frequency |
| leadBrightness | float | 0.5–3.0 | Extra brightness on leading character |
| time | float | — | CPU-accumulated animation time (scaled by rainSpeed on CPU) |

`rainSpeed` (config range 0.1–5.0) does NOT appear as a shader uniform. It scales the CPU time accumulation rate to avoid position jumps when the user adjusts speed mid-animation.

### CPU Time Accumulation

`rainSpeed` drives animation through CPU accumulation. The shader receives only the accumulated `time` value and uses per-column hash speeds (`fSpeed`) as multipliers inside `fract()`. Changing `rainSpeed` smoothly adjusts the accumulation rate without causing faller position jumps:

```cpp
pe->matrixRainTime += pe->currentDeltaTime * cfg->rainSpeed;
// Shader uses: fract(time * fSpeed + offset) — no rainSpeed uniform
```

## Phase 1: Config and Registration

**Goal**: Create config struct and register the effect type in the framework.
**Depends on**: —
**Files**: `src/config/matrix_rain_config.h`, `src/config/effect_config.h`

**Build**:
- Create `src/config/matrix_rain_config.h` with `MatrixRainConfig` struct: `enabled`, `cellSize` (12.0f), `rainSpeed` (1.0f), `trailLength` (15.0f), `fallerCount` (5), `overlayIntensity` (0.8f), `refreshRate` (1.0f), `leadBrightness` (1.5f)
- Modify `src/config/effect_config.h`:
  - Add `#include "matrix_rain_config.h"`
  - Add `TRANSFORM_MATRIX_RAIN` enum before `TRANSFORM_EFFECT_COUNT`
  - Add case in `TransformEffectName()` returning `"Matrix Rain"`
  - Add to `TransformOrderConfig::order` array
  - Add `MatrixRainConfig matrixRain;` member to `EffectConfig`
  - Add case in `IsTransformEnabled()` returning `e->matrixRain.enabled`

**Verify**: `cmake.exe --build build` succeeds with no errors.

**Done when**: `TRANSFORM_MATRIX_RAIN` compiles and `MatrixRainConfig` defaults initialize.

---

## Phase 2: Shader

**Goal**: Implement the fragment shader with rune rendering, rain trails, and overlay compositing.
**Depends on**: —
**Files**: `shaders/matrix_rain.fs`

**Build**:
- Create `shaders/matrix_rain.fs` implementing the full algorithm from Technical Implementation above:
  - Grid division from `resolution` and `cellSize`
  - `hash()` and `hash4()` utility functions
  - `rune_line()` and `rune()` for procedural character rendering
  - Character animation with `time` and `refreshRate`
  - Multi-faller rain trail mask loop using `fallerCount`, `trailLength`, `leadBrightness`
  - Classic green color gradient
  - Overlay compositing with `overlayIntensity`
- Uniforms: `resolution`, `cellSize`, `trailLength`, `fallerCount`, `overlayIntensity`, `refreshRate`, `leadBrightness`, `time`

**Verify**: File exists and GLSL syntax is valid (checked at runtime in Phase 3).

**Done when**: Shader file contains complete algorithm matching Technical Implementation section.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader, declare uniform locations, wire into render pipeline.
**Depends on**: Phase 1, Phase 2
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader matrixRainShader;`
  - Add uniform location ints: `matrixRainResolutionLoc`, `matrixRainCellSizeLoc`, `matrixRainTrailLengthLoc`, `matrixRainFallerCountLoc`, `matrixRainOverlayIntensityLoc`, `matrixRainRefreshRateLoc`, `matrixRainLeadBrightnessLoc`, `matrixRainTimeLoc`
  - Add `float matrixRainTime;` for CPU accumulation
- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get uniform locations in `GetShaderUniformLocations()`
  - Add resolution uniform in `SetResolutionUniforms()`
  - Unload shader in `PostEffectUninit()`

**Verify**: `cmake.exe --build build` succeeds. Application starts without shader load failure.

**Done when**: Shader loads and uniform locations resolve.

---

## Phase 4: Shader Setup

**Goal**: Implement per-frame uniform upload with CPU time accumulation.
**Depends on**: Phase 3
**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupMatrixRain(PostEffect* pe);`
- Modify `src/render/shader_setup.cpp`:
  - Add dispatch case in `GetTransformEffect()`:
    ```
    case TRANSFORM_MATRIX_RAIN:
        return { &pe->matrixRainShader, SetupMatrixRain, &pe->effects.matrixRain.enabled };
    ```
  - Implement `SetupMatrixRain()`:
    - Accumulate `pe->matrixRainTime += pe->currentDeltaTime * cfg->rainSpeed`
    - Upload all uniforms: `cellSize`, `trailLength`, `fallerCount`, `overlayIntensity`, `refreshRate`, `leadBrightness`, `time` (accumulated)

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → enable Matrix Rain in UI → green falling characters overlay the visualizer.

**Done when**: Rain animation renders correctly with procedural rune glyphs falling in columns.

---

## Phase 5: UI Panel

**Goal**: Add Matrix Rain controls to the Style category panel.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects_style.cpp`, `src/ui/imgui_effects.cpp`

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add `TRANSFORM_MATRIX_RAIN` case in `GetTransformCategory()` returning `{"STY", 4}`
- Modify `src/ui/imgui_effects_style.cpp`:
  - Add `static bool sectionMatrixRain = false;` with other section states
  - Add `DrawStyleMatrixRain()` function:
    - Checkbox for enabled (with `MoveTransformToEnd` on first enable)
    - `ImGui::SliderFloat` for `cellSize` (4.0–32.0)
    - `ModulatableSlider` for `rainSpeed` (0.1–5.0)
    - `ModulatableSlider` for `trailLength` (5.0–40.0)
    - `ImGui::SliderInt` for `fallerCount` (1–20)
    - `ModulatableSlider` for `overlayIntensity` (0.0–1.0)
    - `ImGui::SliderFloat` for `refreshRate` (0.1–5.0)
    - `ModulatableSlider` for `leadBrightness` (0.5–3.0)
  - Add call in `DrawStyleCategory()` after Pencil Sketch with `ImGui::Spacing()`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Style section shows "Matrix Rain" with all sliders. Controls modify effect in real-time.

**Done when**: All parameters adjustable via UI, effect responds to slider changes.

---

## Phase 6: Preset Serialization

**Goal**: Save/load Matrix Rain settings in preset JSON.
**Depends on**: Phase 1
**Files**: `src/config/preset.cpp`

**Build**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MatrixRainConfig, enabled, cellSize, rainSpeed, trailLength, fallerCount, overlayIntensity, refreshRate, leadBrightness)`
- Add `to_json` entry: `if (e.matrixRain.enabled) { j["matrixRain"] = e.matrixRain; }`
- Add `from_json` entry: `e.matrixRain = j.value("matrixRain", e.matrixRain);`

**Verify**: Save a preset with Matrix Rain enabled, reload it → settings persist.

**Done when**: Round-trip preset save/load preserves all MatrixRainConfig fields.

---

## Phase 7: Parameter Registration

**Goal**: Register modulatable parameters for LFO/audio-reactive control.
**Depends on**: Phase 1
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add to `PARAM_TABLE` (4 entries, matching indices with targets):
  - `{"matrixRain.rainSpeed", {0.1f, 5.0f}}`
  - `{"matrixRain.overlayIntensity", {0.0f, 1.0f}}`
  - `{"matrixRain.trailLength", {5.0f, 40.0f}}`
  - `{"matrixRain.leadBrightness", {0.5f, 3.0f}}`
- Add to `targets` array (same 4 entries, same order):
  - `&effects->matrixRain.rainSpeed`
  - `&effects->matrixRain.overlayIntensity`
  - `&effects->matrixRain.trailLength`
  - `&effects->matrixRain.leadBrightness`

**Verify**: Assign modulation route to `matrixRain.rainSpeed` → rain speed responds to audio.

**Done when**: All four parameters respond to modulation sources.

---

## Implementation Notes

Post-plan changes applied after initial 7-phase implementation:

### Fall Direction Fix

The plan's rain trail formula `fract(time * fSpeed + fOffset)` produces upward motion in screen space (fragTexCoord.y=0 at top). Negated time to reverse:

```glsl
float headPos = fract(-time * fSpeed + fOffset) * (numRows + trailLength);
```

### Sample Mode

Added `bool sampleMode` toggle to `MatrixRainConfig`. When enabled, glyphs sample their color from the source texture and gaps render black — instead of overlaying green on the visible image.

Compositing branch in shader:
```glsl
if (sampleMode != 0) {
    result = original.rgb * rainAlpha;
} else {
    result = mix(original.rgb, rainColor, rainAlpha);
}
```

Wired through: config field → `int sampleMode` uniform → `post_effect.h` location → `shader_setup.cpp` upload → UI checkbox → preset serialization.
