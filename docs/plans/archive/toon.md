# Toon Effect

Cartoon-style post-processing that quantizes luminance to discrete color bands and renders Sobel edge-detected outlines. Noise-varied edge thickness creates organic brush-stroke appearance.

## Current State

Style effects (Pixelation, Glitch) establish the pattern:
- `src/config/pixelation_config.h:1-14` - Config struct with in-class defaults
- `src/config/effect_config.h:28-29` - Enum entry + name function case
- `shaders/pixelation.fs:1-56` - Fragment shader with resolution uniform
- `src/render/post_effect.h:33,119-122` - Shader + uniform location fields
- `src/render/post_effect.cpp:43,148-151` - Shader loading + uniform locations
- `src/render/shader_setup.cpp:29-30,262-271` - GetTransformEffect case + Setup function
- `src/ui/imgui_effects.cpp:24,640-652` - Section state + UI under Style category

## Technical Implementation

**Source**: [docs/research/toon.md](../research/toon.md)

### Luminance Posterization

```glsl
// Compute grayscale from max RGB component
float greyscale = max(color.r, max(color.g, color.b));

// Quantize to nearest level
float lower = floor(greyscale * levels) / levels;
float upper = ceil(greyscale * levels) / levels;
float level = (abs(greyscale - lower) <= abs(upper - greyscale)) ? lower : upper;

// Scale RGB proportionally to preserve hue
float adjustment = level / max(greyscale, 0.001);
color.rgb *= adjustment;
```

### Sobel Edge Detection

**Kernels:**
```
Gx:               Gy:
| -1  0  +1 |     | +1  +2  +1 |
| -2  0  +2 |     |  0   0   0 |
| -1  0  +1 |     | -1  -2  -1 |
```

```glsl
// Sample 3x3 neighborhood with noise-varied texel size for brush effect
float noise = gnoise(vec3(uv * noiseScale, 0.0));
float thicknessMultiplier = 1.0 + noise * thicknessVariation;
vec2 texel = thicknessMultiplier / resolution;

vec4 n[9];
n[0] = texture(tex, uv + vec2(-texel.x, -texel.y));
n[1] = texture(tex, uv + vec2(    0.0, -texel.y));
n[2] = texture(tex, uv + vec2( texel.x, -texel.y));
n[3] = texture(tex, uv + vec2(-texel.x,     0.0));
n[4] = texture(tex, uv);
n[5] = texture(tex, uv + vec2( texel.x,     0.0));
n[6] = texture(tex, uv + vec2(-texel.x,  texel.y));
n[7] = texture(tex, uv + vec2(    0.0,  texel.y));
n[8] = texture(tex, uv + vec2( texel.x,  texel.y));

// Apply Sobel kernels
vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

// Edge magnitude from RGB channels
float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);
```

### Edge Thresholding

```glsl
// Soft threshold to binary outline
float outline = smoothstep(threshold - softness, threshold + softness, edge);

// Mix with posterized color (black outlines)
vec3 result = mix(posterizedColor, vec3(0.0), outline);
```

### Noise Function

Use gradient noise for smooth thickness variation:
```glsl
// 3D gradient noise (include gnoise function from existing shaders or implement)
float gnoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    vec2 uv = (i.xy + vec2(37.0, 239.0) * i.z) + f.xy;
    vec2 rg = texture(noiseTexture, (uv + 0.5) / 256.0).yx;
    return mix(rg.x, rg.y, f.z) * 2.0 - 1.0;
}
```
Alternative: Use hash-based noise if no noise texture available.

## Parameters

| Parameter | Type | Default | Range | UI Label |
|-----------|------|---------|-------|----------|
| enabled | bool | false | - | Enabled |
| levels | int | 4 | 2-16 | Levels |
| edgeThreshold | float | 0.2 | 0.0-1.0 | Edge Threshold |
| edgeSoftness | float | 0.05 | 0.0-0.2 | Edge Softness |
| thicknessVariation | float | 0.0 | 0.0-1.0 | Brush Variation |
| noiseScale | float | 5.0 | 1.0-20.0 | Brush Scale |

---

## Phase 1: Config and Effect Registration

**Goal**: Toon appears in transform order list (disabled, no shader yet).

**Build**:
- Create `src/config/toon_config.h` with ToonConfig struct and default values
- Edit `src/config/effect_config.h`:
  - Add `#include "toon_config.h"`
  - Add `TRANSFORM_TOON` to enum before `TRANSFORM_EFFECT_COUNT`
  - Add case to `TransformEffectName()` returning "Toon"
  - Add `ToonConfig toon;` member to EffectConfig
  - Add `TRANSFORM_TOON` to default order array

**Done when**: Build succeeds, "Toon" appears (grayed out) in effect order listbox.

---

## Phase 2: Shader

**Goal**: Shader compiles and applies posterization + edge detection + brush strokes.

**Build**:
- Create `shaders/toon.fs` implementing:
  - Uniforms: resolution, levels, edgeThreshold, edgeSoftness, thicknessVariation, noiseScale
  - Posterization via max-RGB luminance quantization
  - 3x3 Sobel edge detection with noise-varied sample distance
  - Smoothstep thresholding to black outlines
  - Hash-based 3D noise function (no texture dependency)
- Edit `src/render/post_effect.h`:
  - Add `Shader toonShader;`
  - Add uniform locations: `toonResolutionLoc`, `toonLevelsLoc`, `toonEdgeThresholdLoc`, `toonEdgeSoftnessLoc`, `toonThicknessVariationLoc`, `toonNoiseScaleLoc`
- Edit `src/render/post_effect.cpp`:
  - Add `LoadShader(0, "shaders/toon.fs")` in LoadPostEffectShaders
  - Add shader success check
  - Add `GetShaderLocation` calls in GetShaderUniformLocations
  - Add to SetResolutionUniforms
  - Add `UnloadShader(pe->toonShader)` in PostEffectUninit
- Edit `src/render/shader_setup.h`:
  - Declare `void SetupToon(PostEffect* pe);`
- Edit `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_TOON` case in GetTransformEffect returning toonShader, SetupToon, enabled ptr
  - Implement SetupToon passing all uniforms

**Done when**: Enabling toon in code applies cartoon effect with visible posterization and outlines.

---

## Phase 3: UI and Serialization

**Goal**: Effect is fully controllable via ImGui and saves/loads with presets.

**Build**:
- Edit `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionToon = false;`
  - Add `case TRANSFORM_TOON` to effect order enabled check
  - Add Toon section under Style category (after Glitch):
    - Enabled checkbox
    - Levels slider (int, 2-16)
    - Edge Threshold slider (0.0-1.0)
    - Edge Softness slider (0.0-0.2)
    - TreeNode "Brush Stroke" containing:
      - Thickness Variation slider (0.0-1.0)
      - Noise Scale slider (1.0-20.0)
- Edit `src/config/preset.cpp`:
  - Add ToonConfig nlohmann JSON serialization macro (NLOHMANN_DEFINE_TYPE_INTRUSIVE)
  - Add `if (e.toon.enabled) { j["toon"] = e.toon; }` in to_json
  - Add `e.toon = j.value("toon", e.toon);` in from_json

**Done when**: All parameters adjustable in UI, presets with toon enabled save and restore correctly.
