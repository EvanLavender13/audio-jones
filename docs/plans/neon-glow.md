# Neon Glow

Sobel edge detection with colored glow and additive blending. Creates cyberpunk/Tron wireframe aesthetics.

## Current State

Transform effects follow a consistent pattern:
- `src/config/toon_config.h:6` - config struct template (similar edge-detection effect)
- `src/config/effect_config.h:46` - `TransformEffectType` enum, config member
- `src/render/post_effect.h:33` - shader field, uniform locations
- `src/render/post_effect.cpp:46` - shader loading
- `src/render/shader_setup.h:40` - setup function declaration
- `src/render/shader_setup.cpp:389` - `SetupToon()` reference pattern
- `src/ui/imgui_effects_transforms.cpp:370` - Toon UI in Stylize category
- `src/config/preset.cpp:200` - JSON serialization
- `shaders/toon.fs` - Sobel edge detection shader reference

## Technical Implementation

### Source
- [docs/research/neon-glow.md](../research/neon-glow.md)
- [Shadertoy: edge glow](https://www.shadertoy.com/view/Mdf3zr) - Sobel + additive color

### Core Algorithm

**Luminance extraction:**
```glsl
float getLuminance(vec3 color) {
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}
```

**Sobel edge detection (3×3 kernel):**
```glsl
float sobelEdge(sampler2D tex, vec2 uv, vec2 texelSize) {
    float n[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 offset = vec2(float(i - 1), float(j - 1)) * texelSize;
            n[i * 3 + j] = getLuminance(texture(tex, uv + offset).rgb);
        }
    }
    // Horizontal: [-1 0 1; -2 0 2; -1 0 1]
    // Vertical:   [-1 -2 -1; 0 0 0; 1 2 1]
    float gx = -n[0] - 2.0*n[3] - n[6] + n[2] + 2.0*n[5] + n[8];
    float gy = -n[0] - 2.0*n[1] - n[2] + n[6] + 2.0*n[7] + n[8];
    return sqrt(gx*gx + gy*gy);
}
```

**Edge shaping (threshold + power curve):**
```glsl
float shapeEdge(float edge, float threshold, float power) {
    float shaped = max(edge - threshold, 0.0);
    return pow(shaped, power);
}
```

**Single-pass glow (cross-tap sampling):**
```glsl
float glowEdge(sampler2D tex, vec2 uv, vec2 texelSize, float radius, int samples) {
    float total = 0.0;
    float weight = 0.0;
    for (int i = -samples; i <= samples; i++) {
        float w = 1.0 - abs(float(i)) / float(samples + 1);
        // Horizontal
        total += sobelEdge(tex, uv + vec2(float(i) * radius, 0.0) * texelSize, texelSize) * w;
        // Vertical
        total += sobelEdge(tex, uv + vec2(0.0, float(i) * radius) * texelSize, texelSize) * w;
        weight += w * 2.0;
    }
    return total / weight;
}
```

**Tonemapping (prevent blowout):**
```glsl
vec3 tonemap(vec3 color) {
    return 1.0 - exp(-color);
}
```

**Complete composition:**
```glsl
// Get edge with optional glow spread
float edge = (glowRadius > 0.0)
    ? glowEdge(tex, uv, texelSize, glowRadius, glowSamples)
    : sobelEdge(tex, uv, texelSize);

// Shape and color
edge = shapeEdge(edge, edgeThreshold, edgePower);
vec3 glow = edge * glowColor * glowIntensity;
glow = tonemap(glow);

// Blend with original
vec3 base = original.rgb * originalVisibility;
vec3 result = base + glow;  // Additive
```

### Parameters

| Uniform | Type | Range | Purpose |
|---------|------|-------|---------|
| glowColor | vec3 | 0-1 each | Neon glow color |
| edgeThreshold | float | 0-0.5 | Noise suppression |
| edgePower | float | 0.5-3.0 | Edge intensity curve |
| glowIntensity | float | 0.5-5.0 | Brightness multiplier |
| glowRadius | float | 0-10 | Blur spread in pixels |
| glowSamples | int | 3-9 | Cross-tap quality (odd) |
| originalVisibility | float | 0-1 | Original image blend |

### Modulatable Parameters
- `neonGlow.glowIntensity` - Beat-reactive brightness
- `neonGlow.edgeThreshold` - Bass-reactive edge reveal
- `neonGlow.originalVisibility` - Dynamic fade control

---

## Phase 1: Config and Shader

**Goal**: Create config struct and fragment shader.

**Build**:
- Create `src/config/neon_glow_config.h` with `NeonGlowConfig` struct
  - `enabled`, `glowR/G/B`, `edgeThreshold`, `edgePower`, `glowIntensity`, `glowRadius`, `glowSamples`, `originalVisibility`
- Create `shaders/neon_glow.fs` implementing the algorithm above
  - Uniforms: `resolution`, all config parameters
  - Use toon.fs Sobel pattern as reference

**Done when**: Config compiles, shader file exists with complete algorithm.

---

## Phase 2: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- `src/config/effect_config.h`:
  - Add `#include "neon_glow_config.h"`
  - Add `TRANSFORM_NEON_GLOW` to enum (before `TRANSFORM_EFFECT_COUNT`)
  - Add name in `TransformEffectName()`
  - Add to `TransformOrderConfig::order[]` default
  - Add `NeonGlowConfig neonGlow;` member to `EffectConfig`
- `src/render/post_effect.h`:
  - Add `Shader neonGlowShader;`
  - Add uniform location fields: `neonGlowResolutionLoc`, `neonGlowGlowColorLoc`, `neonGlowEdgeThresholdLoc`, `neonGlowEdgePowerLoc`, `neonGlowGlowIntensityLoc`, `neonGlowGlowRadiusLoc`, `neonGlowGlowSamplesLoc`, `neonGlowOriginalVisibilityLoc`
- `src/render/post_effect.cpp`:
  - Add shader load in `LoadPostEffectShaders()`
  - Add to shader success check
  - Add uniform location caching in `GetShaderUniformLocations()`
  - Add resolution uniform in `SetResolutionUniforms()`
  - Add unload in `PostEffectUninit()`

**Done when**: Build succeeds, shader loads without errors.

---

## Phase 3: Shader Setup and Pipeline

**Goal**: Wire shader into transform effect chain.

**Build**:
- `src/render/shader_setup.h`:
  - Add `void SetupNeonGlow(PostEffect* pe);`
- `src/render/shader_setup.cpp`:
  - Add `SetupNeonGlow()` implementation binding all uniforms
  - Add case in `GetTransformEffect()` for `TRANSFORM_NEON_GLOW`

**Done when**: Effect renders when enabled via debug/code.

---

## Phase 4: UI Panel

**Goal**: Add controls in Stylize category.

**Build**:
- `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionNeonGlow = false;`
  - Add section in `DrawStylizeCategory()` after Toon:
    - Checkbox "Enabled"
    - Color picker for glow color (RGB sliders)
    - `ModulatableSlider` for `glowIntensity`
    - `ModulatableSlider` for `edgeThreshold`
    - `ModulatableSlider` for `originalVisibility`
    - TreeNode "Advanced" with `edgePower`, `glowRadius`, `glowSamples`

**Done when**: UI controls appear and modify effect in real-time.

---

## Phase 5: Serialization and Modulation

**Goal**: Persist to presets, enable audio modulation.

**Build**:
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_INTRUSIVE` for `NeonGlowConfig`
  - Add save: `if (e.neonGlow.enabled) { j["neonGlow"] = e.neonGlow; }`
  - Add load: `e.neonGlow = j.value("neonGlow", e.neonGlow);`
- `src/automation/param_registry.cpp`:
  - Add to `PARAM_TABLE[]`:
    - `{"neonGlow.glowIntensity", {0.5f, 5.0f}}`
    - `{"neonGlow.edgeThreshold", {0.0f, 0.5f}}`
    - `{"neonGlow.originalVisibility", {0.0f, 1.0f}}`
  - Add corresponding pointers in `ParamRegistryInit()`

**Done when**: Preset save/load works, parameters appear in modulation dropdown.
