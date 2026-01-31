# LEGO Bricks

Pixelates image into 3D-styled LEGO bricks with raised studs and edge shadows. Merges adjacent similar-colored cells into larger rectangular bricks (1x2, 2x1, 2x2) using a deterministic local heuristic algorithm.

## Specification

### Types

```cpp
// src/config/lego_bricks_config.h
#ifndef LEGO_BRICKS_CONFIG_H
#define LEGO_BRICKS_CONFIG_H

// LEGO Bricks: 3D-styled brick pixelation with studs and variable sizing
struct LegoBricksConfig {
    bool enabled = false;
    float brickScale = 0.04f;       // Brick size relative to screen (0.01-0.2)
    float studHeight = 0.5f;        // Stud highlight intensity (0.0-1.0)
    float edgeShadow = 0.2f;        // Edge shadow darkness (0.0-1.0)
    float colorThreshold = 0.1f;    // Color similarity for merging (0.0-0.5)
    int maxBrickSize = 2;           // Largest brick dimension (1-2)
    float lightAngle = 0.7854f;     // Light direction in radians (default 45 deg)
};

#endif // LEGO_BRICKS_CONFIG_H
```

### Shader Algorithm

```glsl
// shaders/lego_bricks.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float brickScale;      // Grid cell size as fraction of screen width
uniform float studHeight;      // Stud highlight intensity
uniform float edgeShadow;      // Edge shadow darkness
uniform float colorThreshold;  // Squared color distance for merging
uniform int maxBrickSize;      // 1 = all 1x1, 2 = up to 2x2
uniform float lightAngle;      // Light direction in radians

// Brick sizes to try, largest first (for maxBrickSize=2)
const ivec2 BRICK_SIZES_2[4] = ivec2[](
    ivec2(2, 2), ivec2(2, 1), ivec2(1, 2), ivec2(1, 1)
);

// Sample color at grid cell center
vec3 sampleCell(ivec2 cell, float cellPixels) {
    vec2 cellCenter = (vec2(cell) + 0.5) * cellPixels / resolution;
    return texture(texture0, cellCenter).rgb;
}

// Squared Euclidean color distance
float colorDist(vec3 a, vec3 b) {
    vec3 d = a - b;
    return dot(d, d);
}

// Find brick anchor and size for this grid cell
// Returns: x,y = anchor cell coords, z = brick width, w = brick height
ivec4 findBrick(ivec2 gridPos, float cellPixels, float threshold) {
    int numSizes = (maxBrickSize >= 2) ? 4 : 1;

    for (int i = 0; i < numSizes; i++) {
        ivec2 size = (maxBrickSize >= 2) ? BRICK_SIZES_2[i] : ivec2(1, 1);

        // Try all anchor positions where this cell could be part of this brick
        for (int ay = 0; ay < size.y; ay++) {
            for (int ax = 0; ax < size.x; ax++) {
                ivec2 anchor = gridPos - ivec2(ax, ay);

                // Anchor must align to brick-size grid
                if (anchor.x % size.x != 0 || anchor.y % size.y != 0) continue;
                if (anchor.x < 0 || anchor.y < 0) continue;

                // Check all cells in this potential brick have similar color
                vec3 baseColor = sampleCell(anchor, cellPixels);
                bool valid = true;

                for (int cy = 0; cy < size.y && valid; cy++) {
                    for (int cx = 0; cx < size.x && valid; cx++) {
                        vec3 cellColor = sampleCell(anchor + ivec2(cx, cy), cellPixels);
                        if (colorDist(cellColor, baseColor) > threshold) {
                            valid = false;
                        }
                    }
                }

                if (valid) {
                    return ivec4(anchor, size);
                }
            }
        }
    }

    // Fallback: 1x1 brick at current cell
    return ivec4(gridPos, 1, 1);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    float cellPixels = brickScale * resolution.x;

    // Which grid cell is this pixel in?
    ivec2 gridPos = ivec2(floor(fc / cellPixels));

    // Find which brick this cell belongs to
    ivec4 brick = findBrick(gridPos, cellPixels, colorThreshold);
    ivec2 anchor = brick.xy;
    ivec2 brickSize = brick.zw;

    // Position within the brick (0-brickSize range)
    ivec2 localCell = gridPos - anchor;

    // Brick bounds in pixels
    vec2 brickStart = vec2(anchor) * cellPixels;
    vec2 brickEnd = vec2(anchor + brickSize) * cellPixels;

    // Sample color at brick anchor cell
    vec3 color = sampleCell(anchor, cellPixels);

    // Calculate stud for this grid cell (each cell in a multi-cell brick gets its own stud)
    vec2 cellStart = vec2(gridPos) * cellPixels;
    vec2 cellCenter = cellStart + cellPixels * 0.5;

    // Stud highlight (circular raised bump)
    float studDist = abs(distance(fc, cellCenter) / cellPixels * 2.0 - 0.6);
    float studHighlight = smoothstep(0.1, 0.05, studDist) * studHeight;
    vec2 studDir = normalize(fc - cellCenter);
    vec2 lightDir = vec2(cos(lightAngle), sin(lightAngle));
    color *= studHighlight * dot(lightDir, studDir) * 0.5 + 1.0;

    // Edge shadow (darker near brick edges, not cell edges)
    vec2 brickCenter = (brickStart + brickEnd) * 0.5;
    vec2 brickHalfSize = (brickEnd - brickStart) * 0.5;
    vec2 delta = abs(fc - brickCenter) / brickHalfSize;
    float edgeDist = max(delta.x, delta.y);
    color *= (1.0 - edgeShadow) + smoothstep(0.95, 0.8, edgeDist) * edgeShadow;

    finalColor = vec4(color, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| brickScale | float | 0.01-0.2 | 0.04 | Yes | Brick Scale |
| studHeight | float | 0.0-1.0 | 0.5 | Yes | Stud Height |
| edgeShadow | float | 0.0-1.0 | 0.2 | No | Edge Shadow |
| colorThreshold | float | 0.0-0.5 | 0.1 | No | Color Threshold |
| maxBrickSize | int | 1-2 | 2 | No | Max Brick Size |
| lightAngle | float | 0-2π | 0.7854 (45°) | Yes | Light Angle |

### Constants

- Enum: `TRANSFORM_LEGO_BRICKS`
- Display name: `"LEGO Bricks"`
- Category: `TRANSFORM_CATEGORY_STYLE` (STY, section 4)
- Config field: `legoBricks`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create config header

**Files**: `src/config/lego_bricks_config.h`
**Creates**: LegoBricksConfig struct that other files include

**Build**:
1. Create file with include guard `LEGO_BRICKS_CONFIG_H`
2. Define `LegoBricksConfig` struct with all fields from spec:
   - `bool enabled = false`
   - `float brickScale = 0.04f`
   - `float studHeight = 0.5f`
   - `float edgeShadow = 0.2f`
   - `float colorThreshold = 0.1f`
   - `int maxBrickSize = 2`
   - `float lightAngle = 0.7854f` (45 degrees in radians)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All Wave 2 tasks modify different files and can run in parallel.

#### Task 2.1: Register effect in effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add include: `#include "lego_bricks_config.h"` (alphabetical order near other config includes)
2. Add enum entry `TRANSFORM_LEGO_BRICKS` before `TRANSFORM_EFFECT_COUNT`
3. Add name to `TRANSFORM_EFFECT_NAMES` array: `"LEGO Bricks"` at matching index
4. Add config member to `EffectConfig` struct: `LegoBricksConfig legoBricks;`
5. Add case in `IsTransformEnabled()`: `case TRANSFORM_LEGO_BRICKS: return e->legoBricks.enabled;`

**Verify**: Compiles.

#### Task 2.2: Create fragment shader

**Files**: `shaders/lego_bricks.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create shader file with `#version 330`
2. Declare inputs: `in vec2 fragTexCoord;` and `out vec4 finalColor;`
3. Declare uniforms:
   - `sampler2D texture0`
   - `vec2 resolution`
   - `float brickScale, studHeight, edgeShadow, colorThreshold, lightAngle`
   - `int maxBrickSize`
4. Implement helper functions from spec:
   - `sampleCell(ivec2 cell, float cellPixels)` - sample texture at cell center
   - `colorDist(vec3 a, vec3 b)` - squared Euclidean distance
   - `findBrick(ivec2 gridPos, float cellPixels, float threshold)` - deterministic brick assignment
5. Implement `main()` from spec:
   - Calculate cell pixels from brickScale
   - Find grid position and brick assignment
   - Sample color at brick anchor
   - Apply stud highlight with light direction
   - Apply edge shadow based on brick bounds

**Verify**: File exists. Full verification after shader loading task.

#### Task 2.3: Add shader to PostEffect

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build** (post_effect.h):
1. Add shader member after other shaders (near inkWashShader): `Shader legoBricksShader;`
2. Add uniform location members:
   - `int legoBricksResolutionLoc;`
   - `int legoBricksBrickScaleLoc;`
   - `int legoBricksStudHeightLoc;`
   - `int legoBricksEdgeShadowLoc;`
   - `int legoBricksColorThresholdLoc;`
   - `int legoBricksMaxBrickSizeLoc;`
   - `int legoBricksLightAngleLoc;`

**Build** (post_effect.cpp):
1. In `LoadPostEffectShaders()`: Add `pe->legoBricksShader = LoadShader(0, "shaders/lego_bricks.fs");`
2. Add to success check: `&& pe->legoBricksShader.id != 0`
3. In `GetShaderUniformLocations()`: Add location lookups for all uniforms
4. In `SetResolutionUniforms()`: Add resolution uniform set
5. In `PostEffectUninit()`: Add `UnloadShader(pe->legoBricksShader);`

**Verify**: Compiles.

#### Task 2.4: Implement shader setup function

**Files**: `src/render/shader_setup_style.h`, `src/render/shader_setup_style.cpp`
**Depends on**: Wave 1 complete

**Build** (shader_setup_style.h):
1. Add declaration: `void SetupLegoBricks(PostEffect *pe);`

**Build** (shader_setup_style.cpp):
1. Add implementation:
```cpp
void SetupLegoBricks(PostEffect *pe) {
    const LegoBricksConfig *cfg = &pe->effects.legoBricks;
    SetShaderValue(pe->legoBricksShader, pe->legoBricksBrickScaleLoc,
                   &cfg->brickScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->legoBricksShader, pe->legoBricksStudHeightLoc,
                   &cfg->studHeight, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->legoBricksShader, pe->legoBricksEdgeShadowLoc,
                   &cfg->edgeShadow, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->legoBricksShader, pe->legoBricksColorThresholdLoc,
                   &cfg->colorThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->legoBricksShader, pe->legoBricksMaxBrickSizeLoc,
                   &cfg->maxBrickSize, SHADER_UNIFORM_INT);
    SetShaderValue(pe->legoBricksShader, pe->legoBricksLightAngleLoc,
                   &cfg->lightAngle, SHADER_UNIFORM_FLOAT);
}
```

**Verify**: Compiles.

#### Task 2.5: Add shader dispatch

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add case in `GetTransformEffect()` switch (after TRANSFORM_INK_WASH or similar Style effect):
```cpp
case TRANSFORM_LEGO_BRICKS:
    return {&pe->legoBricksShader, SetupLegoBricks, &pe->effects.legoBricks.enabled};
```

**Verify**: Compiles.

#### Task 2.6: Add UI panel

**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_style.cpp`
**Depends on**: Wave 1 complete

**Build** (imgui_effects.cpp):
1. Add case in `GetTransformCategory()` switch, in Style section (section 4):
```cpp
case TRANSFORM_LEGO_BRICKS:
```
   (Add to existing Style case list that returns `{"STY", 4}`)

**Build** (imgui_effects_style.cpp):
1. Add static bool at top with others: `static bool sectionLegoBricks = false;`
2. Add helper function before `DrawStyleCategory()`:
```cpp
static void DrawStyleLegoBricks(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
    if (DrawSectionBegin("LEGO Bricks", categoryGlow, &sectionLegoBricks)) {
        const bool wasEnabled = e->legoBricks.enabled;
        ImGui::Checkbox("Enabled##legobricks", &e->legoBricks.enabled);
        if (!wasEnabled && e->legoBricks.enabled) {
            MoveTransformToEnd(&e->transformOrder, TRANSFORM_LEGO_BRICKS);
        }
        if (e->legoBricks.enabled) {
            ModulatableSlider("Brick Scale##legobricks", &e->legoBricks.brickScale,
                              "legoBricks.brickScale", "%.3f", modSources);
            ModulatableSlider("Stud Height##legobricks", &e->legoBricks.studHeight,
                              "legoBricks.studHeight", "%.2f", modSources);
            ImGui::SliderFloat("Edge Shadow##legobricks", &e->legoBricks.edgeShadow,
                               0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Color Threshold##legobricks", &e->legoBricks.colorThreshold,
                               0.0f, 0.5f, "%.3f");
            ImGui::SliderInt("Max Brick Size##legobricks", &e->legoBricks.maxBrickSize, 1, 2);
            ModulatableSliderAngleDeg("Light Angle##legobricks", &e->legoBricks.lightAngle,
                                      "legoBricks.lightAngle", modSources);
        }
        DrawSectionEnd();
    }
}
```
3. Add call in `DrawStyleCategory()` with spacing:
```cpp
ImGui::Spacing();
DrawStyleLegoBricks(e, modSources, categoryGlow);
```

**Verify**: Compiles.

#### Task 2.7: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add JSON macro with other config macros (alphabetical):
```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LegoBricksConfig, enabled,
    brickScale, studHeight, edgeShadow, colorThreshold, maxBrickSize, lightAngle)
```
2. Add to_json entry in `to_json(json& j, const EffectConfig& e)`:
```cpp
if (e.legoBricks.enabled) { j["legoBricks"] = e.legoBricks; }
```
3. Add from_json entry in `from_json(const json& j, EffectConfig& e)`:
```cpp
e.legoBricks = j.value("legoBricks", e.legoBricks);
```

**Verify**: Compiles.

#### Task 2.8: Register modulatable parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add to `PARAM_TABLE` (maintain alphabetical groups):
```cpp
{"legoBricks.brickScale", {0.01f, 0.2f}},
{"legoBricks.studHeight", {0.0f, 1.0f}},
{"legoBricks.lightAngle", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
```
2. Add to targets array at matching indices (search for `float* targets[]`):
```cpp
&effects->legoBricks.brickScale,
&effects->legoBricks.studHeight,
&effects->legoBricks.lightAngle,
```

**Verify**: Compiles. PARAM_TABLE and targets indices must match.

---

## Final Verification

After all tasks complete:

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] No compiler warnings
- [ ] Effect appears in Transform Order pipeline when enabled
- [ ] Effect shows "STY" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to the pipeline list
- [ ] UI controls modify effect in real-time:
  - Brick Scale changes brick density
  - Stud Height adjusts 3D appearance
  - Edge Shadow controls darkness at brick edges
  - Color Threshold affects brick merging (more/fewer merged bricks)
  - Max Brick Size toggles variable sizing (1 = all 1x1)
  - Light Angle rotates stud highlights
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to: brickScale, studHeight, lightAngle

---

## Implementation Notes

**Performance**: With maxBrickSize=2, worst case is checking 4 brick sizes × 4 positions × 4 cells = 64 texture samples per pixel. Acceptable for real-time.

**Grid alignment**: Larger bricks align to their size grid (2x2 anchors at even coords). This ensures deterministic assignment.

**Stud placement**: Each cell in a multi-cell brick gets its own stud centered on that cell, not one stud for the entire brick.

**Edge shadow**: Applied to brick bounds, not cell bounds. A 2x2 brick has shadows only at its outer edges.

<!-- Intentional deviations from research:
  - maxBrickSize range limited to 1-2 (research allows 1-4) per user preference for simpler implementation
  - Edge shadow at brick boundaries vs cell boundaries per user preference for authentic LEGO look
  - lightAngle parameterizes hardcoded 45° from research base shader
-->
