# Constellation

Animated network of glowing points connected by fading lines. Points drift within grid cells via sinusoidal motion with radial wave overlay. Lines connect neighbors and fade by length. A procedural generator that renders on black and blends additively into the pipeline after feedback, before transforms.

## Specification

### Types

```cpp
// src/config/constellation_config.h
#ifndef CONSTELLATION_CONFIG_H
#define CONSTELLATION_CONFIG_H

#include "color_config.h"
#include <stdbool.h>

typedef struct ConstellationConfig {
  bool enabled = false;

  // Grid and animation
  float gridScale = 21.0f;      // Point density: cells across screen (5.0-50.0)
  float animSpeed = 1.0f;       // Wander animation speed multiplier (0.0-5.0)
  float wanderAmp = 0.4f;       // How far points drift from cell center (0.0-0.5)

  // Radial wave overlay
  float radialFreq = 1.0f;      // Radial wave frequency (0.1-5.0)
  float radialAmp = 1.0f;       // Radial wave strength on positions (0.0-2.0)
  float radialSpeed = 0.5f;     // Radial wave propagation speed (0.0-5.0)

  // Point rendering
  float glowScale = 100.0f;     // Inverse-squared glow falloff (50.0-500.0)
  float pointBrightness = 1.0f; // Point glow intensity (0.0-2.0)

  // Line rendering
  float lineThickness = 0.05f;  // Width of connection lines (0.01-0.1)
  float maxLineLen = 1.5f;      // Lines longer than this fade out (0.5-2.0)
  float lineOpacity = 0.5f;     // Overall line brightness (0.0-1.0)

  // Color mode
  bool interpolateLineColor = false; // true: blend endpoint colors; false: sample LUT by length

  // Gradients
  ColorConfig pointGradient;    // Color gradient for points (sampled by cell hash)
  ColorConfig lineGradient;     // Color gradient for lines (sampled by length or interpolated)

} ConstellationConfig;

#endif // CONSTELLATION_CONFIG_H
```

### Shader Algorithm

```glsl
// shaders/constellation.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;    // Source texture (passthrough for additive blend)
uniform sampler2D pointLUT;    // 1D gradient for point colors
uniform sampler2D lineLUT;     // 1D gradient for line colors
uniform vec2 resolution;
uniform float time;

// Parameters
uniform float gridScale;
uniform float animSpeed;
uniform float wanderAmp;
uniform float radialFreq;
uniform float radialAmp;
uniform float radialSpeed;
uniform float glowScale;
uniform float pointBrightness;
uniform float lineThickness;
uniform float maxLineLen;
uniform float lineOpacity;
uniform int interpolateLineColor;

// Hash functions
float N21(vec2 p) {
    p = fract(p * vec2(233.34, 851.73));
    p += dot(p, p + 23.456);
    return fract(p.x * p.y);
}

vec2 N22(vec2 p) {
    float n = N21(p);
    return vec2(n, N21(p + n));
}

// Point position within cell
vec2 GetPos(vec2 cellID, vec2 cellOffset) {
    vec2 hash = N22(cellID + cellOffset);
    vec2 n = hash * (time * animSpeed);
    float radial = sin(length(cellID + cellOffset) * radialFreq - time * radialSpeed) * radialAmp;
    return cellOffset + sin(n + vec2(radial)) * wanderAmp;
}

// Signed distance to line segment
float DistLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * t);
}

// Line rendering with color
vec4 Line(vec2 p, vec2 a, vec2 b, float lineLen, vec2 cellIDA) {
    float dist = DistLine(p, a, b);
    float alpha = smoothstep(lineThickness, lineThickness * 0.2, dist);
    alpha *= smoothstep(maxLineLen, maxLineLen * 0.5, lineLen);
    alpha *= lineOpacity;

    vec3 col;
    if (interpolateLineColor != 0) {
        vec3 colA = textureLod(pointLUT, vec2(N21(a + cellIDA), 0.5), 0.0).rgb;
        vec3 colB = textureLod(pointLUT, vec2(N21(b + cellIDA), 0.5), 0.0).rgb;
        float t = clamp(dot(p - a, b - a) / dot(b - a, b - a), 0.0, 1.0);
        col = mix(colA, colB, t);
    } else {
        float lutPos = lineLen / maxLineLen;
        col = textureLod(lineLUT, vec2(lutPos, 0.5), 0.0).rgb;
    }

    return vec4(col, alpha);
}

// Point glow rendering
vec3 Point(vec2 p, vec2 pointPos, vec2 cellID) {
    vec2 delta = pointPos - p;
    float glow = 1.0 / dot(delta * glowScale, delta * glowScale);
    glow = clamp(glow, 0.0, 1.0);

    float lutPos = N21(cellID);
    vec3 col = textureLod(pointLUT, vec2(lutPos, 0.5), 0.0).rgb;

    return col * glow * pointBrightness;
}

// Single layer of constellation
vec3 Layer(vec2 uv) {
    vec3 result = vec3(0.0);

    vec2 cellCoord = uv * gridScale;
    vec2 gv = fract(cellCoord) - 0.5;
    vec2 id = floor(cellCoord);

    // Gather 9 neighbor positions
    vec2 points[9];
    int idx = 0;
    for (float y = -1.0; y <= 1.0; y++) {
        for (float x = -1.0; x <= 1.0; x++) {
            points[idx++] = GetPos(id, vec2(x, y));
        }
    }

    // Center point (index 4) connects to all neighbors
    for (int i = 0; i < 9; i++) {
        if (i == 4) continue;
        float lineLen = length(points[4] - points[i]);
        vec4 line = Line(gv, points[4], points[i], lineLen, id);
        result += line.rgb * line.a;
    }

    // Corner-to-corner edges (indices: 1-3, 1-5, 7-3, 7-5)
    float len13 = length(points[1] - points[3]);
    vec4 line13 = Line(gv, points[1], points[3], len13, id);
    result += line13.rgb * line13.a;

    float len15 = length(points[1] - points[5]);
    vec4 line15 = Line(gv, points[1], points[5], len15, id);
    result += line15.rgb * line15.a;

    float len73 = length(points[7] - points[3]);
    vec4 line73 = Line(gv, points[7], points[3], len73, id);
    result += line73.rgb * line73.a;

    float len75 = length(points[7] - points[5]);
    vec4 line75 = Line(gv, points[7], points[5], len75, id);
    result += line75.rgb * line75.a;

    // Render all 9 points
    for (int i = 0; i < 9; i++) {
        vec2 cellID = id + vec2(float(i % 3) - 1.0, float(i / 3) - 1.0);
        result += Point(gv, points[i], cellID);
    }

    return result;
}

void main() {
    vec2 uv = fragTexCoord;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    vec3 constellation = Layer(uv);

    // Additive blend with source
    vec3 source = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(source + constellation, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| gridScale | float | 5.0 - 50.0 | 21.0 | Yes | Grid Scale |
| animSpeed | float | 0.0 - 5.0 | 1.0 | Yes | Anim Speed |
| wanderAmp | float | 0.0 - 0.5 | 0.4 | Yes | Wander |
| radialFreq | float | 0.1 - 5.0 | 1.0 | No | Radial Freq |
| radialAmp | float | 0.0 - 2.0 | 1.0 | Yes | Radial Amp |
| radialSpeed | float | 0.0 - 5.0 | 0.5 | Yes | Radial Speed |
| glowScale | float | 50.0 - 500.0 | 100.0 | No | Glow Scale |
| pointBrightness | float | 0.0 - 2.0 | 1.0 | Yes | Point Bright |
| lineThickness | float | 0.01 - 0.1 | 0.05 | No | Line Width |
| maxLineLen | float | 0.5 - 2.0 | 1.5 | Yes | Max Line Len |
| lineOpacity | float | 0.0 - 1.0 | 0.5 | Yes | Line Opacity |
| interpolateLineColor | bool | - | false | No | Interpolate |
| pointGradient | ColorConfig | - | cyan-magenta | No | Point Colors |
| lineGradient | ColorConfig | - | white-blue | No | Line Colors |

### Deviations from Research

<!-- Intentional: radialFreq and glowScale used in research algorithm but undocumented - added with sensible defaults -->
<!-- Intentional: Skip center-to-self line (i=4) to avoid zero-length line artifacts -->
<!-- Intentional: Add cellID to Line hash for unique colors across cells -->
<!-- Intentional: Complete alpha placeholder with .a usage -->

### Pipeline Integration

Constellation is a generator, not a transform. No enum, no transform ordering.

1. Executes after feedback pass, before transform chain
2. Renders procedurally on black background
3. Blends additively onto the ping-pong buffer
4. Has its own `ApplyConstellationPass()` function in render_pipeline.cpp
5. Controlled by `constellation.enabled` checkbox only

---

## Tasks

### Wave 1: Config and Shader Foundation

#### Task 1.1: Create constellation_config.h

**Files**: `src/config/constellation_config.h`
**Creates**: ConstellationConfig struct that other files include

**Build**:
1. Create header with include guard `CONSTELLATION_CONFIG_H`
2. Include `color_config.h` and `<stdbool.h>`
3. Define ConstellationConfig struct per specification above
4. Initialize pointGradient and lineGradient with default ColorConfig values

**Verify**: Header compiles when included.

---

#### Task 1.2: Create constellation.fs shader

**Files**: `shaders/constellation.fs`
**Creates**: Fragment shader for constellation rendering

**Build**:
1. Create shader file with GLSL 330
2. Implement all uniforms per specification
3. Implement hash functions N21, N22
4. Implement GetPos, DistLine, Line, Point, Layer functions
5. Main function: aspect-correct UV, render layer, additive blend with source

**Verify**: Shader file exists and has valid GLSL syntax.

---

### Wave 2: Core Integration (Parallel)

#### Task 2.1: Add to effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "constellation_config.h"` in alphabetical position
2. Add `ConstellationConfig constellation;` field to EffectConfig struct (after circuitBoard, before colorGrade alphabetically)

**Verify**: Compiles.

---

#### Task 2.2: Add shader and LUTs to post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `Shader constellationShader;` field (alphabetical among shaders)
2. Add uniform location fields:
   - `int constellationResolutionLoc;`
   - `int constellationTimeLoc;`
   - `int constellationGridScaleLoc;`
   - `int constellationAnimSpeedLoc;`
   - `int constellationWanderAmpLoc;`
   - `int constellationRadialFreqLoc;`
   - `int constellationRadialAmpLoc;`
   - `int constellationRadialSpeedLoc;`
   - `int constellationGlowScaleLoc;`
   - `int constellationPointBrightnessLoc;`
   - `int constellationLineThicknessLoc;`
   - `int constellationMaxLineLenLoc;`
   - `int constellationLineOpacityLoc;`
   - `int constellationInterpolateLineColorLoc;`
   - `int constellationPointLUTLoc;`
   - `int constellationLineLUTLoc;`
3. Add LUT pointers:
   - `ColorLUT *constellationPointLUT;`
   - `ColorLUT *constellationLineLUT;`
4. Add time accumulator:
   - `float constellationTime;`

**Verify**: Compiles.

---

#### Task 2.3: Load shader and init LUTs in post_effect.cpp

**Files**: `src/render/post_effect.cpp`
**Depends on**: Task 2.2 complete

**Build**:
1. In PostEffectInit, load shader:
   - `pe->constellationShader = LoadShader(0, "shaders/constellation.fs");`
2. Add to shader validation check
3. Get all uniform locations using GetShaderLocation
4. Initialize LUTs:
   - `pe->constellationPointLUT = ColorLUTInit(&pe->effects.constellation.pointGradient);`
   - `pe->constellationLineLUT = ColorLUTInit(&pe->effects.constellation.lineGradient);`
5. Set resolution uniform in PostEffectResize
6. Initialize `pe->constellationTime = 0.0f;`
7. In PostEffectUninit:
   - `UnloadShader(pe->constellationShader);`
   - `ColorLUTUninit(pe->constellationPointLUT);`
   - `ColorLUTUninit(pe->constellationLineLUT);`

**Verify**: Compiles.

---

#### Task 2.4: Create shader setup function

**Files**: `src/render/shader_setup_generators.cpp`, `src/render/shader_setup_generators.h`
**Creates**: New shader setup module for generators category

**Build**:
1. Create header with include guard, declare `void SetupConstellation(PostEffect *pe);`
2. Create cpp file:
   - Include header, `post_effect.h`, `color_lut.h`
   - Implement SetupConstellation:
     - Update both ColorLUTs from config
     - Set all uniform values from `pe->effects.constellation`
     - Bind both LUT textures

**Verify**: Compiles.

---

#### Task 2.5: Add generator pass to render_pipeline.cpp

**Files**: `src/render/render_pipeline.cpp`
**Depends on**: Tasks 2.3, 2.4 complete

**Build**:
1. Include `shader_setup_generators.h`
2. In RenderPipelineApplyOutput, after chromatic pass and before the transform loop:
   - Accumulate time: `pe->constellationTime += deltaTime;` (add deltaTime param to function signature)
   - If `pe->effects.constellation.enabled`:
     ```cpp
     RenderPass(pe, src, &pe->pingPong[writeIdx], pe->constellationShader, SetupConstellation);
     src = &pe->pingPong[writeIdx];
     writeIdx = 1 - writeIdx;
     ```
3. Update RenderPipelineApplyOutput signature to take deltaTime, pass from caller

**Verify**: Compiles.

---

### Wave 3: UI and Modulation (Parallel)

#### Task 3.1: Create UI panel for generators

**Files**: `src/ui/imgui_effects_generators.cpp`, `src/ui/imgui_effects_generators.h`
**Depends on**: Wave 2 complete

**Build**:
1. Create header declaring `void DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources);`
2. Create cpp implementing Constellation UI section:
   - Enabled checkbox
   - Modulatable sliders for: gridScale, animSpeed, wanderAmp, radialAmp, radialSpeed, pointBrightness, maxLineLen, lineOpacity
   - Regular sliders for: radialFreq, glowScale, lineThickness
   - Checkbox for interpolateLineColor
   - Gradient editors for pointGradient and lineGradient (use ImGuiDrawGradientEditor pattern from false color)
3. Use category glow index 10 (after simulations at 9)

**Verify**: Compiles.

---

#### Task 3.2: Register in imgui_effects.cpp

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Task 3.1 complete

**Build**:
1. Include `imgui_effects_generators.h`
2. Add GENERATORS group header after SIMULATIONS, before TRANSFORMS:
   ```cpp
   ImGui::Spacing();
   ImGui::Spacing();
   DrawGroupHeader("GENERATORS", Theme::ACCENT_ORANGE_U32);
   DrawGeneratorsCategory(e, modSources);
   ```

**Verify**: Compiles.

---

#### Task 3.3: Register modulatable parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add to PARAM_TABLE:
   ```cpp
   {"constellation.gridScale", {5.0f, 50.0f}, offsetof(EffectConfig, constellation.gridScale)},
   {"constellation.animSpeed", {0.0f, 5.0f}, offsetof(EffectConfig, constellation.animSpeed)},
   {"constellation.wanderAmp", {0.0f, 0.5f}, offsetof(EffectConfig, constellation.wanderAmp)},
   {"constellation.radialAmp", {0.0f, 2.0f}, offsetof(EffectConfig, constellation.radialAmp)},
   {"constellation.radialSpeed", {0.0f, 5.0f}, offsetof(EffectConfig, constellation.radialSpeed)},
   {"constellation.pointBrightness", {0.0f, 2.0f}, offsetof(EffectConfig, constellation.pointBrightness)},
   {"constellation.maxLineLen", {0.5f, 2.0f}, offsetof(EffectConfig, constellation.maxLineLen)},
   {"constellation.lineOpacity", {0.0f, 1.0f}, offsetof(EffectConfig, constellation.lineOpacity)},
   ```

**Verify**: Compiles.

---

#### Task 3.4: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. In SavePreset, add constellation section with all fields
2. In LoadPreset, parse constellation section with defaults
3. Serialize pointGradient and lineGradient using existing ColorConfig pattern

**Verify**: Compiles.

---

### Wave 4: Build System

#### Task 4.1: Update CMakeLists.txt

**Files**: `CMakeLists.txt`
**Depends on**: All source files created

**Build**:
1. Add `src/render/shader_setup_generators.cpp` to RENDER_SOURCES
2. Add `src/ui/imgui_effects_generators.cpp` to IMGUI_UI_SOURCES

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release` succeeds.

---

## Final Verification

- [ ] `cmake.exe --build build` compiles with no errors
- [ ] Application launches without crash
- [ ] Constellation appears in GENERATORS section of Effects panel
- [ ] Enabling Constellation renders glowing points and lines
- [ ] Points drift with sinusoidal motion
- [ ] Lines fade based on length
- [ ] Gradient editors control point and line colors
- [ ] Modulatable parameters respond to LFO routing
- [ ] Preset save/load preserves Constellation settings

---

## Implementation Corrections

The following issues were identified and fixed during implementation:

### Animation Phase Accumulation

**Problem:** Plan specified `time * animSpeed` in shader, causing instant position jumps when animSpeed changed.

**Fix:** Accumulate `animPhase` on CPU (`animPhase += deltaTime * animSpeed`), pass to shader. Same for `radialPhase`. Shader uses phases directly without multiplication.

### Bounded Oscillation

**Problem:** Plan's `hash * animPhase` grows unbounded, causing animation to accelerate into chaos.

**Fix:** Match reference behavior: `hash * (sin(animPhase) * 0.5 + 2.2)` oscillates between 1.7-2.7, producing smooth looping motion.

### Point Size Parameter

**Problem:** Plan specified `glowScale` with inverted behavior (higher = smaller glow) and wrong default (100 vs reference's 15).

**Fix:** Renamed to `pointSize`, inverted math (`15.0 / pointSize`), default 1.0, range 0.3-3.0. Higher = bigger glow.

### Grid Scale Origin

**Problem:** Grid scaled from bottom-left corner, making modulation useless.

**Fix:** Center UV before scaling: `uv = fragTexCoord - 0.5`.

### Line Endpoint Fade

**Problem:** Lines drew through point glows, obscuring them.

**Fix:** Added endpoint fade: `alpha *= smoothstep(0.0, pointSize * 0.15, min(distToA, distToB))`.

### Line Color Flickering

**Problem:** Interpolate mode used animated positions for color hash, causing flicker.

**Fix:** Pass stable cell IDs to Line function, hash those instead of animated positions.

### Default Gradient Mode

**Problem:** ColorConfig defaults to SOLID mode (white), not gradient.

**Fix:** Initialize gradients with `{.mode = COLOR_MODE_GRADIENT}`.

### LUT Binding

**Problem:** Used `lut->texture` directly instead of `ColorLUTGetTexture()`.

**Fix:** Use accessor function for proper texture retrieval.

### Group Header Colors

**Problem:** GENERATORS used same hardcoded color as SIMULATIONS.

**Fix:** Use `GetSectionAccent(groupIdx++)` for all group headers to cycle colors.

### Radial Wave Parameters

**Problem:** `radialAmp` default 1.0 too weak (reference uses 2.0).

**Fix:** Default 2.0, range 0-4.

### pointSize Modulation

**Problem:** Not registered as modulatable parameter.

**Fix:** Added to param_registry with range 0.3-3.0.
