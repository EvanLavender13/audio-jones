# Watercolor Effect

Simulates watercolor painting through edge darkening (pigment pooling), procedural paper granulation, and soft color bleeding. Single-pass shader following existing style effect patterns.

## Current State

Style effects live in TRANSFORMS category with consistent patterns:
- `src/config/oil_paint_config.h` - simple config struct template
- `src/config/toon_config.h` - config with noise parameters
- `shaders/toon.fs:20-51` - hash-based FBM noise (reusable pattern)
- `shaders/toon.fs:72-89` - Sobel edge detection (reusable pattern)
- `src/ui/imgui_effects_transforms.cpp:299` - Style category UI

## Technical Implementation

### Procedural Paper Texture (FBM Noise)

Adapted from Toon shader's proven noise implementation:

```glsl
// Hash-based 3D noise (no texture dependency)
float hash(vec3 p)
{
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

float gnoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash(i + vec3(0.0, 0.0, 0.0));
    float n001 = hash(i + vec3(0.0, 0.0, 1.0));
    float n010 = hash(i + vec3(0.0, 1.0, 0.0));
    float n011 = hash(i + vec3(0.0, 1.0, 1.0));
    float n100 = hash(i + vec3(1.0, 0.0, 0.0));
    float n101 = hash(i + vec3(1.0, 0.0, 1.0));
    float n110 = hash(i + vec3(1.0, 1.0, 0.0));
    float n111 = hash(i + vec3(1.0, 1.0, 1.0));

    float n00 = mix(n000, n001, f.z);
    float n01 = mix(n010, n011, f.z);
    float n10 = mix(n100, n101, f.z);
    float n11 = mix(n110, n111, f.z);

    float n0 = mix(n00, n01, f.y);
    float n1 = mix(n10, n11, f.y);

    return mix(n0, n1, f.x);  // Returns 0-1 (not centered)
}

// Multi-octave FBM for paper texture
float paperTexture(vec2 uv, float scale)
{
    float paper = 0.0;
    float amplitude = 0.5;
    float frequency = scale;

    for (int i = 0; i < 4; i++) {
        paper += gnoise(vec3(uv * frequency, 0.0)) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return paper;
}
```

### Sobel Edge Detection

Adapted from Toon shader pattern:

```glsl
float sobelEdge(sampler2D tex, vec2 uv, vec2 texel)
{
    // Sample 3x3 neighborhood
    float n[9];
    n[0] = getLuminance(texture(tex, uv + vec2(-texel.x, -texel.y)).rgb);
    n[1] = getLuminance(texture(tex, uv + vec2(    0.0, -texel.y)).rgb);
    n[2] = getLuminance(texture(tex, uv + vec2( texel.x, -texel.y)).rgb);
    n[3] = getLuminance(texture(tex, uv + vec2(-texel.x,     0.0)).rgb);
    n[4] = getLuminance(texture(tex, uv).rgb);
    n[5] = getLuminance(texture(tex, uv + vec2( texel.x,     0.0)).rgb);
    n[6] = getLuminance(texture(tex, uv + vec2(-texel.x,  texel.y)).rgb);
    n[7] = getLuminance(texture(tex, uv + vec2(    0.0,  texel.y)).rgb);
    n[8] = getLuminance(texture(tex, uv + vec2( texel.x,  texel.y)).rgb);

    // Sobel horizontal and vertical gradients
    float sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    float sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    return sqrt(sobelH * sobelH + sobelV * sobelV);
}

float getLuminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}
```

### Combined Pipeline

```glsl
void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;
    vec4 original = texture(texture0, uv);
    vec3 color = original.rgb;

    // 1. Optional soft blur for paint diffusion (box filter approximation)
    if (softness > 0.0) {
        vec3 blurred = vec3(0.0);
        float samples = 0.0;
        int radius = int(softness);
        for (int x = -radius; x <= radius; x++) {
            for (int y = -radius; y <= radius; y++) {
                blurred += texture(texture0, uv + vec2(x, y) * texel).rgb;
                samples += 1.0;
            }
        }
        color = blurred / samples;
    }

    // 2. Edge detection
    float edge = sobelEdge(texture0, uv, texel);

    // 3. Edge darkening (pigment pooling)
    color *= 1.0 - edge * edgeDarkening;

    // 4. Paper granulation
    float paper = paperTexture(uv, paperScale);
    // Apply more granulation in flat areas, less at edges
    float granulationMask = 1.0 - edge;
    color *= mix(1.0, paper, granulationStrength * granulationMask);

    // 5. Color bleeding at edges
    if (bleedStrength > 0.0 && edge > 0.01) {
        // Directional blur perpendicular to edge
        vec2 edgeDir = normalize(vec2(
            getLuminance(texture(texture0, uv + vec2(texel.x, 0.0)).rgb) -
            getLuminance(texture(texture0, uv - vec2(texel.x, 0.0)).rgb),
            getLuminance(texture(texture0, uv + vec2(0.0, texel.y)).rgb) -
            getLuminance(texture(texture0, uv - vec2(0.0, texel.y)).rgb)
        ) + 0.001);

        vec3 bleed = vec3(0.0);
        for (int i = -2; i <= 2; i++) {
            bleed += texture(texture0, uv + edgeDir * float(i) * bleedRadius * texel).rgb;
        }
        bleed /= 5.0;
        color = mix(color, bleed, edge * bleedStrength);
    }

    // 6. Optional color quantization
    if (colorLevels > 0) {
        float fLevels = float(colorLevels);
        color = floor(color * fLevels + 0.5) / fLevels;
    }

    finalColor = vec4(color, original.a);
}
```

### Parameters

| Uniform | Type | Range | Default | Modulatable |
|---------|------|-------|---------|-------------|
| edgeDarkening | float | 0.0-1.0 | 0.3 | Yes |
| granulationStrength | float | 0.0-1.0 | 0.3 | Yes |
| paperScale | float | 1.0-20.0 | 8.0 | No |
| softness | float | 0.0-5.0 | 0.0 | No |
| bleedStrength | float | 0.0-1.0 | 0.2 | Yes |
| bleedRadius | float | 1.0-10.0 | 3.0 | No |
| colorLevels | int | 0-16 | 0 | No |

---

## Phase 1: Config and Shader

**Goal**: Create watercolor config struct and fragment shader.

**Build**:
- Create `src/config/watercolor_config.h` with WatercolorConfig struct containing all 7 parameters
- Create `shaders/watercolor.fs` implementing the combined pipeline above

**Done when**: Shader compiles, config struct exists with documented parameters.

---

## Phase 2: PostEffect Integration

**Goal**: Load shader and wire uniform locations.

**Build**:
- Modify `src/render/post_effect.h`: Add `watercolorShader` member and 8 uniform location ints (resolution + 7 params)
- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get all uniform locations in `GetShaderUniformLocations()`
  - Add to `SetResolutionUniforms()`
  - Unload in `PostEffectUninit()`

**Done when**: Shader loads without errors, uniform locations retrieved.

---

## Phase 3: Effect Registration

**Goal**: Register watercolor in the transform effect system.

**Build**:
- Modify `src/config/effect_config.h`:
  - Add `#include "watercolor_config.h"`
  - Add `TRANSFORM_WATERCOLOR` to enum before `TRANSFORM_EFFECT_COUNT`
  - Add case in `TransformEffectName()`
  - Add `TRANSFORM_WATERCOLOR` to `TransformOrderConfig::order` array
  - Add `WatercolorConfig watercolor;` member to `EffectConfig`

**Done when**: Effect appears in transform order list (though not yet functional).

---

## Phase 4: Shader Setup

**Goal**: Connect config values to shader uniforms.

**Build**:
- Modify `src/render/shader_setup.h`: Declare `SetupWatercolor()`
- Modify `src/render/shader_setup.cpp`:
  - Add dispatch case for `TRANSFORM_WATERCOLOR` in `GetTransformEffect()`
  - Implement `SetupWatercolor()` function setting all 7 uniforms

**Done when**: Effect renders correctly when enabled.

---

## Phase 5: UI and Serialization

**Goal**: Add UI controls and preset save/load.

**Build**:
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionWatercolor = false;` with other section states
  - Add watercolor section in `DrawStyleCategory()` after Oil Paint with all controls
- Modify `src/ui/imgui_effects.cpp`:
  - Add `case TRANSFORM_WATERCOLOR: isEnabled = e->watercolor.enabled; break;` in isEnabled switch
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for WatercolorConfig
  - Add to_json entry for watercolor
  - Add from_json entry for watercolor

**Done when**: UI controls work, presets save/load watercolor settings.

---

## Phase 6: Parameter Registration

**Goal**: Enable audio modulation for core parameters.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add 3 entries to PARAM_TABLE:
    - `{"watercolor.edgeDarkening", {0.0f, 1.0f}}`
    - `{"watercolor.granulationStrength", {0.0f, 1.0f}}`
    - `{"watercolor.bleedStrength", {0.0f, 1.0f}}`
  - Add 3 matching entries to targets array:
    - `&effects->watercolor.edgeDarkening`
    - `&effects->watercolor.granulationStrength`
    - `&effects->watercolor.bleedStrength`

**Done when**: Parameters appear in modulation routing, respond to audio/LFO sources.
