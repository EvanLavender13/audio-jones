# Oil Paint (Kuwahara Filter)

Painterly style effect that smooths flat regions while preserving hard edges. Creates characteristic oil paint brush strokes with visible color banding. Basic 4-sector Kuwahara with modulatable radius.

## Current State

Style transforms live in the output stage alongside Toon, Pixelation, etc:
- `src/config/toon_config.h` - Example Style effect config
- `src/render/post_effect.h:39` - Shader and uniform location fields
- `src/render/shader_setup.cpp:389-402` - SetupToon() binding pattern
- `src/ui/imgui_effects_transforms.cpp:370-386` - Toon UI section in Style category
- `shaders/toon.fs` - Example Style shader with Sobel edge detection

## Technical Implementation

- **Source**: [docs/research/oil-paint.md](../research/oil-paint.md)
- **Algorithm**: Basic Kuwahara - divide kernel into 4 overlapping quadrants, output mean color of lowest-variance sector

### Shader Algorithm

```glsl
// Sector layout around pixel X:
// +---+---+
// | 0 | 1 |
// +---X---+
// | 2 | 3 |
// +---+---+

// For each sector, compute:
vec3 mean = colorSum / count;
vec3 variance = (colorSqSum / count) - (mean * mean);
float v = dot(variance, vec3(0.299, 0.587, 0.114));  // luminance-weighted

// Select sector with minimum variance
// Output: mean color of that sector
```

### Parameters

| Uniform | Type | Range | UI |
|---------|------|-------|-----|
| resolution | vec2 | screen size | (auto) |
| radius | int | 2-8 | Slider, modulatable |

---

## Phase 1: Config and Shader

**Goal**: Create config struct and fragment shader.

**Build**:
- `src/config/oil_paint_config.h` - OilPaintConfig with `enabled` (bool), `radius` (float, default 4.0, range 2-8)
- `shaders/oil_paint.fs` - Basic 4-sector Kuwahara implementation

**Shader pseudocode**:
```glsl
#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 resolution;
uniform int radius;
out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    // Calculate 4 sectors: each sector is a (radius+1) x (radius+1) region
    // Sector 0: top-left (x: -radius to 0, y: -radius to 0)
    // Sector 1: top-right (x: 0 to radius, y: -radius to 0)
    // Sector 2: bottom-left (x: -radius to 0, y: 0 to radius)
    // Sector 3: bottom-right (x: 0 to radius, y: 0 to radius)

    vec3 mean[4];
    float variance[4];

    for (int s = 0; s < 4; s++) {
        vec3 colorSum = vec3(0.0);
        vec3 colorSqSum = vec3(0.0);
        float count = 0.0;

        // Determine sector bounds based on index
        int xStart = (s == 0 || s == 2) ? -radius : 0;
        int xEnd   = (s == 0 || s == 2) ? 0 : radius;
        int yStart = (s == 0 || s == 1) ? -radius : 0;
        int yEnd   = (s == 0 || s == 1) ? 0 : radius;

        for (int x = xStart; x <= xEnd; x++) {
            for (int y = yStart; y <= yEnd; y++) {
                vec2 offset = vec2(float(x), float(y)) * texel;
                vec3 c = texture(texture0, uv + offset).rgb;
                colorSum += c;
                colorSqSum += c * c;
                count += 1.0;
            }
        }

        mean[s] = colorSum / count;
        vec3 v = (colorSqSum / count) - (mean[s] * mean[s]);
        variance[s] = dot(v, vec3(0.299, 0.587, 0.114));
    }

    // Find minimum variance sector
    int minIdx = 0;
    float minVar = variance[0];
    for (int s = 1; s < 4; s++) {
        if (variance[s] < minVar) {
            minVar = variance[s];
            minIdx = s;
        }
    }

    finalColor = vec4(mean[minIdx], 1.0);
}
```

**Done when**: Shader compiles, config struct defined.

---

## Phase 2: PostEffect Integration

**Goal**: Load shader, cache uniforms, wire into transform pipeline.

**Build**:
- `src/render/post_effect.h` - Add `Shader oilPaintShader`, `int oilPaintResolutionLoc`, `int oilPaintRadiusLoc`
- `src/render/post_effect.cpp`:
  - `LoadPostEffectShaders()` - Load `shaders/oil_paint.fs`
  - `GetShaderUniformLocations()` - Cache resolution and radius locations
  - `SetResolutionUniforms()` - Set resolution uniform
  - `PostEffectUninit()` - Unload shader
- `src/config/effect_config.h`:
  - Include `oil_paint_config.h`
  - Add `TRANSFORM_OIL_PAINT` to enum (before TRANSFORM_EFFECT_COUNT)
  - Add case to `TransformEffectName()`
  - Add to `TransformOrderConfig::order[]` default
  - Add `OilPaintConfig oilPaint;` member to EffectConfig

**Done when**: Shader loads on startup, uniform locations cached.

---

## Phase 3: Shader Setup

**Goal**: Bind uniforms and register in transform dispatch.

**Build**:
- `src/render/shader_setup.h` - Add `void SetupOilPaint(PostEffect* pe);`
- `src/render/shader_setup.cpp`:
  - Add `SetupOilPaint()` function to set radius uniform
  - Add `TRANSFORM_OIL_PAINT` case to `GetTransformEffect()` returning `{ &pe->oilPaintShader, SetupOilPaint, &pe->effects.oilPaint.enabled }`

**Done when**: Effect renders when enabled=true (hardcode in config temporarily).

---

## Phase 4: UI Panel

**Goal**: Add controls to Style category in Transforms panel.

**Build**:
- `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionOilPaint = false;`
  - In `DrawStyleCategory()`, add section after Toon:
    - Checkbox for enabled
    - ModulatableSlider for radius (float, range 2-8, format "%.0f")

**Done when**: Can toggle effect and adjust radius from UI.

---

## Phase 5: Serialization and Modulation

**Goal**: Save/load presets, register modulatable parameter.

**Build**:
- `src/config/preset.cpp`:
  - `to_json()` - Add `if (e.oilPaint.enabled) { j["oilPaint"] = e.oilPaint; }`
  - `from_json()` - Add `e.oilPaint = j.value("oilPaint", e.oilPaint);`
- `src/automation/param_registry.cpp` - Register `"oilPaint.radius"` with range 2.0-8.0

**Done when**: Presets save/load oil paint settings, radius responds to modulation.
