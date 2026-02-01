# Synthwave

Transforms input visuals into an 80s retrofuturism aesthetic with neon-on-dark color treatment, perspective grid overlay, and horizontal sun stripe bands. Bright areas take on the striped sun character while dark regions reveal the laser grid.

## Specification

### Types

```cpp
// src/config/synthwave_config.h
#ifndef SYNTHWAVE_CONFIG_H
#define SYNTHWAVE_CONFIG_H

// Synthwave: 80s retrofuturism aesthetic with cosine palette color remap,
// perspective grid overlay, and horizontal sun stripes
struct SynthwaveConfig {
    bool enabled = false;

    // Horizon and color
    float horizonY = 0.5f;           // Vertical position of horizon (0.3-0.7)
    float colorMix = 0.7f;           // Blend between original and palette (0-1)
    float palettePhaseR = 0.0f;      // Cosine palette phase R (0-1)
    float palettePhaseG = 0.1f;      // Cosine palette phase G (0-1)
    float palettePhaseB = 0.2f;      // Cosine palette phase B (0-1)

    // Perspective grid
    float gridSpacing = 8.0f;        // Distance between grid lines (2-20)
    float gridThickness = 0.03f;     // Width of grid lines (0.01-0.1)
    float gridOpacity = 0.5f;        // Overall grid visibility (0-1)
    float gridGlow = 1.5f;           // Neon bloom intensity on grid (1-3)
    float gridR = 0.0f;              // Grid color R (0-1)
    float gridG = 0.8f;              // Grid color G (0-1)
    float gridB = 1.0f;              // Grid color B (0-1)

    // Sun stripes
    float stripeCount = 8.0f;        // Number of horizontal sun bands (4-20)
    float stripeSoftness = 0.1f;     // Edge softness of stripes (0-0.3)
    float stripeIntensity = 0.6f;    // Overall stripe visibility (0-1)
    float sunR = 1.0f;               // Sun stripe color R (0-1)
    float sunG = 0.4f;               // Sun stripe color G (0-1)
    float sunB = 0.8f;               // Sun stripe color B (0-1)

    // Horizon glow
    float horizonIntensity = 0.3f;   // Glow at horizon line (0-1)
    float horizonFalloff = 10.0f;    // Horizon glow decay rate (5-30)
    float horizonR = 1.0f;           // Horizon glow color R (0-1)
    float horizonG = 0.6f;           // Horizon glow color G (0-1)
    float horizonB = 0.0f;           // Horizon glow color B (0-1)
};

#endif // SYNTHWAVE_CONFIG_H
```

### Shader Algorithm

```glsl
// shaders/synthwave.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;

// Horizon and color
uniform float horizonY;
uniform float colorMix;
uniform vec3 palettePhase;

// Grid
uniform float gridSpacing;
uniform float gridThickness;
uniform float gridOpacity;
uniform float gridGlow;
uniform vec3 gridColor;

// Stripes
uniform float stripeCount;
uniform float stripeSoftness;
uniform float stripeIntensity;
uniform vec3 sunColor;

// Horizon glow
uniform float horizonIntensity;
uniform float horizonFalloff;
uniform vec3 horizonColor;

// Cosine palette: cyan -> magenta -> orange gradient
vec3 synthwavePalette(float t, vec3 phase) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    return a + b * cos(6.283185 * (c * t + phase));
}

// Perspective grid with vanishing point at horizon
float perspectiveGrid(vec2 uv) {
    float depth = (uv.y - horizonY) / (1.0 - horizonY);
    if (depth <= 0.0) return 0.0;

    float z = 1.0 / depth;
    float x = (uv.x - 0.5) * z;

    vec2 gridUV = vec2(x, z) * gridSpacing;
    vec2 fw = fwidth(gridUV);
    vec2 grid = smoothstep(gridThickness + fw, gridThickness - fw,
                           abs(fract(gridUV) - 0.5));

    return max(grid.x, grid.y);
}

// Horizontal sun stripes in sky region
float sunStripes(vec2 uv) {
    if (uv.y > horizonY) return 0.0;

    float skyPos = uv.y / horizonY;
    float stripePhase = skyPos * stripeCount;
    float stripe = smoothstep(0.4 - stripeSoftness, 0.4 + stripeSoftness,
                              abs(fract(stripePhase) - 0.5));
    float fade = smoothstep(0.0, 0.3, skyPos) * smoothstep(1.0, 0.7, skyPos);

    return stripe * fade;
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 inputColor = texture(texture0, uv);

    // Luminance for masking and palette lookup
    float luma = dot(inputColor.rgb, vec3(0.299, 0.587, 0.114));

    // Cosine palette color remap
    vec3 remapped = synthwavePalette(luma, palettePhase);
    vec3 result = mix(inputColor.rgb, remapped, colorMix);

    // Grid on dark areas
    float gridMask = perspectiveGrid(uv);
    float darknessMask = 1.0 - luma;
    gridMask *= darknessMask * gridOpacity;
    result += gridColor * gridMask * gridGlow;

    // Sun stripes on bright areas
    float stripeMask = sunStripes(uv);
    stripeMask *= luma * stripeIntensity;
    result = mix(result, sunColor, stripeMask);

    // Horizon glow
    float horizonGlow = exp(-abs(uv.y - horizonY) * horizonFalloff);
    result += horizonColor * horizonGlow * horizonIntensity;

    finalColor = vec4(result, inputColor.a);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| horizonY | float | 0.3-0.7 | 0.5 | Yes | Horizon |
| colorMix | float | 0-1 | 0.7 | Yes | Color Mix |
| palettePhaseR | float | 0-1 | 0.0 | No | Phase R |
| palettePhaseG | float | 0-1 | 0.1 | No | Phase G |
| palettePhaseB | float | 0-1 | 0.2 | No | Phase B |
| gridSpacing | float | 2-20 | 8.0 | No | Grid Spacing |
| gridThickness | float | 0.01-0.1 | 0.03 | No | Line Width |
| gridOpacity | float | 0-1 | 0.5 | Yes | Grid Opacity |
| gridGlow | float | 1-3 | 1.5 | Yes | Grid Glow |
| gridR/G/B | float | 0-1 | (0, 0.8, 1) | No | Grid Color |
| stripeCount | float | 4-20 | 8.0 | No | Stripe Count |
| stripeSoftness | float | 0-0.3 | 0.1 | No | Stripe Softness |
| stripeIntensity | float | 0-1 | 0.6 | Yes | Stripe Intensity |
| sunR/G/B | float | 0-1 | (1, 0.4, 0.8) | No | Sun Color |
| horizonIntensity | float | 0-1 | 0.3 | Yes | Horizon Glow |
| horizonFalloff | float | 5-30 | 10.0 | No | Horizon Falloff |
| horizonR/G/B | float | 0-1 | (1, 0.6, 0) | No | Horizon Color |

### Constants

- Enum name: `TRANSFORM_SYNTHWAVE`
- Display name: `"Synthwave"`
- Category: `TRANSFORM_CATEGORY_GRAPHIC` (GFX badge, section 5)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create SynthwaveConfig

**Files**: `src/config/synthwave_config.h`
**Creates**: Config struct that other files include

**Build**:
1. Create file with include guard `SYNTHWAVE_CONFIG_H`
2. Copy struct definition from Specification > Types section exactly

**Verify**: File exists with correct content.

---

### Wave 2: Core Integration (Parallel)

#### Task 2.1: Register Effect Type

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1 complete

**Build**:
1. Add include `#include "synthwave_config.h"` (alphabetical, after `surface_warp_config.h`)
2. Add enum `TRANSFORM_SYNTHWAVE,` before `TRANSFORM_EFFECT_COUNT`
3. Add name to `TRANSFORM_EFFECT_NAMES[]`: `"Synthwave",` with comment `// TRANSFORM_SYNTHWAVE`
4. Add config member to `EffectConfig`: `SynthwaveConfig synthwave;` with comment `// Synthwave (80s retrofuturism)`
5. Add case to `IsTransformEnabled()`: `case TRANSFORM_SYNTHWAVE: return e->synthwave.enabled;`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Create Fragment Shader

**Files**: `shaders/synthwave.fs`
**Depends on**: None (parallel with 2.1)

**Build**:
1. Create file with exact content from Specification > Shader Algorithm section

**Verify**: File exists with correct content.

#### Task 2.3: PostEffect Shader Members

**Files**: `src/render/post_effect.h`
**Depends on**: None (parallel with 2.1)

**Build**:
1. Add shader member near other shaders (around line 43): `Shader synthwaveShader;`
2. Add uniform location members (near line 260):
```cpp
int synthwaveResolutionLoc;
int synthwaveHorizonYLoc;
int synthwaveColorMixLoc;
int synthwavePalettePhaseLoc;
int synthwaveGridSpacingLoc;
int synthwaveGridThicknessLoc;
int synthwaveGridOpacityLoc;
int synthwaveGridGlowLoc;
int synthwaveGridColorLoc;
int synthwaveStripeCountLoc;
int synthwaveStripeSoftnessLoc;
int synthwaveStripeIntensityLoc;
int synthwaveSunColorLoc;
int synthwaveHorizonIntensityLoc;
int synthwaveHorizonFalloffLoc;
int synthwaveHorizonColorLoc;
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: PostEffect Shader Loading

**Files**: `src/render/post_effect.cpp`
**Depends on**: Task 2.3 complete

**Build**:
1. In `LoadPostEffectShaders()` (around line 87): Add `pe->synthwaveShader = LoadShader(0, "shaders/synthwave.fs");`
2. Add to success check chain: `&& pe->synthwaveShader.id != 0`
3. In `GetShaderUniformLocations()` (around line 440): Add all uniform locations:
```cpp
pe->synthwaveResolutionLoc = GetShaderLocation(pe->synthwaveShader, "resolution");
pe->synthwaveHorizonYLoc = GetShaderLocation(pe->synthwaveShader, "horizonY");
pe->synthwaveColorMixLoc = GetShaderLocation(pe->synthwaveShader, "colorMix");
pe->synthwavePalettePhaseLoc = GetShaderLocation(pe->synthwaveShader, "palettePhase");
pe->synthwaveGridSpacingLoc = GetShaderLocation(pe->synthwaveShader, "gridSpacing");
pe->synthwaveGridThicknessLoc = GetShaderLocation(pe->synthwaveShader, "gridThickness");
pe->synthwaveGridOpacityLoc = GetShaderLocation(pe->synthwaveShader, "gridOpacity");
pe->synthwaveGridGlowLoc = GetShaderLocation(pe->synthwaveShader, "gridGlow");
pe->synthwaveGridColorLoc = GetShaderLocation(pe->synthwaveShader, "gridColor");
pe->synthwaveStripeCountLoc = GetShaderLocation(pe->synthwaveShader, "stripeCount");
pe->synthwaveStripeSoftnessLoc = GetShaderLocation(pe->synthwaveShader, "stripeSoftness");
pe->synthwaveStripeIntensityLoc = GetShaderLocation(pe->synthwaveShader, "stripeIntensity");
pe->synthwaveSunColorLoc = GetShaderLocation(pe->synthwaveShader, "sunColor");
pe->synthwaveHorizonIntensityLoc = GetShaderLocation(pe->synthwaveShader, "horizonIntensity");
pe->synthwaveHorizonFalloffLoc = GetShaderLocation(pe->synthwaveShader, "horizonFalloff");
pe->synthwaveHorizonColorLoc = GetShaderLocation(pe->synthwaveShader, "horizonColor");
```
4. In `SetResolutionUniforms()` (around line 927): Add resolution uniform:
```cpp
SetShaderValue(pe->synthwaveShader, pe->synthwaveResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
```
5. In `PostEffectUninit()` (around line 1120): Add `UnloadShader(pe->synthwaveShader);`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 3: Setup and Dispatch (Parallel)

#### Task 3.1: Shader Setup Declaration

**Files**: `src/render/shader_setup_graphic.h`
**Depends on**: Wave 2 complete

**Build**:
1. Add function declaration: `void SetupSynthwave(PostEffect *pe);`

**Verify**: File compiles.

#### Task 3.2: Shader Setup Implementation

**Files**: `src/render/shader_setup_graphic.cpp`
**Depends on**: Task 3.1 complete

**Build**:
1. Add setup function at end of file:
```cpp
void SetupSynthwave(PostEffect *pe) {
    const SynthwaveConfig *sw = &pe->effects.synthwave;

    SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonYLoc,
                   &sw->horizonY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveColorMixLoc,
                   &sw->colorMix, SHADER_UNIFORM_FLOAT);

    float palettePhase[3] = {sw->palettePhaseR, sw->palettePhaseG, sw->palettePhaseB};
    SetShaderValue(pe->synthwaveShader, pe->synthwavePalettePhaseLoc,
                   palettePhase, SHADER_UNIFORM_VEC3);

    SetShaderValue(pe->synthwaveShader, pe->synthwaveGridSpacingLoc,
                   &sw->gridSpacing, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveGridThicknessLoc,
                   &sw->gridThickness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveGridOpacityLoc,
                   &sw->gridOpacity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveGridGlowLoc,
                   &sw->gridGlow, SHADER_UNIFORM_FLOAT);

    float gridColor[3] = {sw->gridR, sw->gridG, sw->gridB};
    SetShaderValue(pe->synthwaveShader, pe->synthwaveGridColorLoc,
                   gridColor, SHADER_UNIFORM_VEC3);

    SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeCountLoc,
                   &sw->stripeCount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeSoftnessLoc,
                   &sw->stripeSoftness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeIntensityLoc,
                   &sw->stripeIntensity, SHADER_UNIFORM_FLOAT);

    float sunColor[3] = {sw->sunR, sw->sunG, sw->sunB};
    SetShaderValue(pe->synthwaveShader, pe->synthwaveSunColorLoc,
                   sunColor, SHADER_UNIFORM_VEC3);

    SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonIntensityLoc,
                   &sw->horizonIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonFalloffLoc,
                   &sw->horizonFalloff, SHADER_UNIFORM_FLOAT);

    float horizonColor[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
    SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonColorLoc,
                   horizonColor, SHADER_UNIFORM_VEC3);
}
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 3.3: Shader Dispatch

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Task 3.2 complete

**Build**:
1. Add case to `GetTransformEffect()` switch (near existing graphic effects):
```cpp
case TRANSFORM_SYNTHWAVE:
    return {&pe->synthwaveShader, SetupSynthwave, &pe->effects.synthwave.enabled};
```

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 4: UI and Serialization (Parallel)

#### Task 4.1: Category Mapping

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 3 complete

**Build**:
1. Add case to `GetTransformCategory()` switch in Graphic section (around line 82):
```cpp
case TRANSFORM_SYNTHWAVE:
```
(Add to existing Graphic case list before the `return {"GFX", 5};`)

**Verify**: `cmake.exe --build build` compiles.

#### Task 4.2: UI Panel

**Files**: `src/ui/imgui_effects_graphic.cpp`
**Depends on**: Wave 3 complete

**Build**:
1. Add section state at top with others: `static bool sectionSynthwave = false;`
2. Add helper function before `DrawGraphicCategory()`:
```cpp
static void DrawGraphicSynthwave(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
    if (DrawSectionBegin("Synthwave", categoryGlow, &sectionSynthwave)) {
        const bool wasEnabled = e->synthwave.enabled;
        ImGui::Checkbox("Enabled##synthwave", &e->synthwave.enabled);
        if (!wasEnabled && e->synthwave.enabled) {
            MoveTransformToEnd(&e->transformOrder, TRANSFORM_SYNTHWAVE);
        }
        if (e->synthwave.enabled) {
            SynthwaveConfig *sw = &e->synthwave;

            ModulatableSlider("Horizon##synthwave", &sw->horizonY,
                              "synthwave.horizonY", "%.2f", modSources);
            ModulatableSlider("Color Mix##synthwave", &sw->colorMix,
                              "synthwave.colorMix", "%.2f", modSources);

            if (TreeNodeAccented("Palette##synthwave", categoryGlow)) {
                ImGui::SliderFloat("Phase R##synthwave", &sw->palettePhaseR, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Phase G##synthwave", &sw->palettePhaseG, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Phase B##synthwave", &sw->palettePhaseB, 0.0f, 1.0f, "%.2f");
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Grid##synthwave", categoryGlow)) {
                ImGui::SliderFloat("Spacing##synthwave", &sw->gridSpacing, 2.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Line Width##synthwave", &sw->gridThickness, 0.01f, 0.1f, "%.3f");
                ModulatableSlider("Opacity##synthwave_grid", &sw->gridOpacity,
                                  "synthwave.gridOpacity", "%.2f", modSources);
                ModulatableSlider("Glow##synthwave", &sw->gridGlow,
                                  "synthwave.gridGlow", "%.2f", modSources);
                float gridCol[3] = {sw->gridR, sw->gridG, sw->gridB};
                if (ImGui::ColorEdit3("Color##synthwave_grid", gridCol)) {
                    sw->gridR = gridCol[0];
                    sw->gridG = gridCol[1];
                    sw->gridB = gridCol[2];
                }
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Sun Stripes##synthwave", categoryGlow)) {
                ImGui::SliderFloat("Count##synthwave", &sw->stripeCount, 4.0f, 20.0f, "%.0f");
                ImGui::SliderFloat("Softness##synthwave", &sw->stripeSoftness, 0.0f, 0.3f, "%.2f");
                ModulatableSlider("Intensity##synthwave_stripe", &sw->stripeIntensity,
                                  "synthwave.stripeIntensity", "%.2f", modSources);
                float sunCol[3] = {sw->sunR, sw->sunG, sw->sunB};
                if (ImGui::ColorEdit3("Color##synthwave_sun", sunCol)) {
                    sw->sunR = sunCol[0];
                    sw->sunG = sunCol[1];
                    sw->sunB = sunCol[2];
                }
                TreeNodeAccentedPop();
            }

            if (TreeNodeAccented("Horizon Glow##synthwave", categoryGlow)) {
                ModulatableSlider("Intensity##synthwave_horizon", &sw->horizonIntensity,
                                  "synthwave.horizonIntensity", "%.2f", modSources);
                ImGui::SliderFloat("Falloff##synthwave", &sw->horizonFalloff, 5.0f, 30.0f, "%.1f");
                float horizonCol[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
                if (ImGui::ColorEdit3("Color##synthwave_horizon", horizonCol)) {
                    sw->horizonR = horizonCol[0];
                    sw->horizonG = horizonCol[1];
                    sw->horizonB = horizonCol[2];
                }
                TreeNodeAccentedPop();
            }
        }
        DrawSectionEnd();
    }
}
```
3. Add call in `DrawGraphicCategory()` after halftone:
```cpp
ImGui::Spacing();
DrawGraphicSynthwave(e, modSources, categoryGlow);
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 4.3: Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add JSON macro with other config macros (around line 210):
```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SynthwaveConfig, enabled,
    horizonY, colorMix, palettePhaseR, palettePhaseG, palettePhaseB,
    gridSpacing, gridThickness, gridOpacity, gridGlow, gridR, gridG, gridB,
    stripeCount, stripeSoftness, stripeIntensity, sunR, sunG, sunB,
    horizonIntensity, horizonFalloff, horizonR, horizonG, horizonB)
```
2. Add to_json entry (around line 472):
```cpp
if (e.synthwave.enabled) { j["synthwave"] = e.synthwave; }
```
3. Add from_json entry (around line 621):
```cpp
e.synthwave = j.value("synthwave", e.synthwave);
```

**Verify**: `cmake.exe --build build` compiles.

#### Task 4.4: Parameter Registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add to `PARAM_TABLE[]` (maintain alphabetical order by prefix):
```cpp
{"synthwave.horizonY", {0.3f, 0.7f}},
{"synthwave.colorMix", {0.0f, 1.0f}},
{"synthwave.gridOpacity", {0.0f, 1.0f}},
{"synthwave.gridGlow", {1.0f, 3.0f}},
{"synthwave.stripeIntensity", {0.0f, 1.0f}},
{"synthwave.horizonIntensity", {0.0f, 1.0f}},
```
2. Add to `targets[]` array at matching indices:
```cpp
&effects->synthwave.horizonY,
&effects->synthwave.colorMix,
&effects->synthwave.gridOpacity,
&effects->synthwave.gridGlow,
&effects->synthwave.stripeIntensity,
&effects->synthwave.horizonIntensity,
```

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Effect appears in transform order pipeline when enabled
- [ ] Effect shows "GFX" badge in pipeline list
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to the pipeline list at end
- [ ] UI controls modify effect in real-time:
  - [ ] Horizon slider moves horizon line
  - [ ] Color Mix blends original vs palette colors
  - [ ] Palette Phase sliders shift color character
  - [ ] Grid controls adjust perspective grid appearance
  - [ ] Stripe controls adjust sun band visibility
  - [ ] Horizon Glow creates gradient at horizon
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters:
  - [ ] LFO can modulate horizonY, colorMix, gridOpacity, gridGlow, stripeIntensity, horizonIntensity
