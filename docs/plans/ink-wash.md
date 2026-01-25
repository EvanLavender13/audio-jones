# Ink Wash

Post-process shader that emphasizes contours through Sobel edge darkening ("ink pooling"), applies FBM paper noise to flat areas (making edges appear brighter by contrast), and blurs color along edge gradients. Coexists alongside the existing Watercolor effect.

## Current State

- `src/config/watercolor_config.h` - Existing watercolor (stroke tracing), unrelated algorithm
- `src/config/effect_config.h:102` - `TRANSFORM_EFFECT_COUNT` end marker for new enum entries
- `src/render/post_effect.h:50` - Shader member pattern (watercolor neighbor)
- `src/render/post_effect.cpp:94` - LoadShader pattern
- `src/render/shader_setup.cpp:58` - Dispatch case pattern (watercolor neighbor)
- `src/ui/imgui_effects_style.cpp:429` - Style category draw calls
- `src/automation/param_registry.cpp:131` - Watercolor param entries (insert after)

## Technical Implementation

- **Source**: `docs/research/ink-wash.md`, derived from old `watercolor.fs` (commit `0abe845`)
- **Core algorithm**: 4-step pipeline — Sobel edge detection → edge darkening → FBM paper granulation → directional color bleed

### Edge Detection (Sobel)

3x3 Sobel on luminance. Produces edge magnitude and gradient direction:

```glsl
float getLuminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void sobelEdge(vec2 uv, vec2 texel, out float edge, out vec2 gradient)
{
    float n[9];
    n[0] = getLuminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
    n[1] = getLuminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
    n[2] = getLuminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
    n[3] = getLuminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
    n[4] = getLuminance(texture(texture0, uv).rgb);
    n[5] = getLuminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
    n[6] = getLuminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
    n[7] = getLuminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
    n[8] = getLuminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

    float sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    float sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    edge = sqrt(sobelH * sobelH + sobelV * sobelV);
    gradient = vec2(sobelH, sobelV);
}
```

### Edge Darkening

```glsl
color *= 1.0 - edge * strength;
```

### Paper Granulation (FBM)

Fixed-scale FBM noise, masked to flat areas only (edges stay clean):

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbmNoise(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Application:
float paper = fbmNoise(uv * 20.0);       // fixed scale
float granMask = 1.0 - edge;             // spare edges from noise
color *= mix(1.0, paper, granulation * granMask);
```

### Color Bleed

Directional blur along luminance gradient at edges. 5 taps, 5-texel radius:

```glsl
vec2 bleedDir = normalize(gradient + 0.001);
vec3 bleed = vec3(0.0);
for (int i = -2; i <= 2; i++) {
    bleed += texture(texture0, uv + bleedDir * float(i) * 5.0 * texel).rgb;
}
bleed /= 5.0;
color = mix(color, bleed, edge * bleedStrength);
```

### Combined Main

```glsl
void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;
    vec3 color = texture(texture0, uv).rgb;

    float edge;
    vec2 gradient;
    sobelEdge(uv, texel, edge, gradient);

    color *= 1.0 - edge * strength;

    float paper = fbmNoise(uv * 20.0);
    color *= mix(1.0, paper, granulation * (1.0 - edge));

    vec2 bleedDir = normalize(gradient + 0.001);
    vec3 bleed = vec3(0.0);
    for (int i = -2; i <= 2; i++) {
        bleed += texture(texture0, uv + bleedDir * float(i) * 5.0 * texel).rgb;
    }
    bleed /= 5.0;
    color = mix(color, bleed, edge * bleedStrength);

    finalColor = vec4(color, texture(texture0, uv).a);
}
```

### Parameters

| Parameter | Type | Range | Default | Uniform name |
|-----------|------|-------|---------|--------------|
| strength | float | 0.0–2.0 | 1.0 | `strength` |
| granulation | float | 0.0–1.0 | 0.5 | `granulation` |
| bleedStrength | float | 0.0–1.0 | 0.5 | `bleedStrength` |

### Modulation Candidates

- `inkWash.strength` (0.0–2.0): Higher on beats pulses ink borders
- `inkWash.granulation` (0.0–1.0): Modulate with high-frequency energy for reactive paper texture

---

## Phase 1: Config + Registration

**Goal**: Ink Wash appears in the transform enum and order array.
**Depends on**: —
**Files**: `src/config/ink_wash_config.h`, `src/config/effect_config.h`

**Build**:
- Create `src/config/ink_wash_config.h` with `InkWashConfig` struct: `enabled` (bool, false), `strength` (float, 1.0), `granulation` (float, 0.5), `bleedStrength` (float, 0.5)
- In `effect_config.h`:
  - Add `#include "ink_wash_config.h"`
  - Add `TRANSFORM_INK_WASH` before `TRANSFORM_EFFECT_COUNT`
  - Add `case TRANSFORM_INK_WASH: return "Ink Wash";` in `TransformEffectName()`
  - Add `TRANSFORM_INK_WASH` at end of `TransformOrderConfig::order` array
  - Add `InkWashConfig inkWash;` member to `EffectConfig`
  - Add `case TRANSFORM_INK_WASH: return e->inkWash.enabled;` in `IsTransformEnabled()`

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: `TRANSFORM_INK_WASH` exists in enum and order array, config struct defined.

---

## Phase 2: Shader

**Goal**: Fragment shader implements the ink wash algorithm.
**Depends on**: —
**Files**: `shaders/ink_wash.fs`

**Build**:
- Create `shaders/ink_wash.fs` implementing the full algorithm from Technical Implementation above
- Uniforms: `resolution` (vec2), `strength` (float), `granulation` (float), `bleedStrength` (float)

**Verify**: File exists and has correct `#version 330` header.

**Done when**: Shader file contains Sobel edge detection, FBM paper granulation, and directional color bleed.

---

## Phase 3: PostEffect + ShaderSetup

**Goal**: Shader loads, uniform locations resolve, setup function dispatches.
**Depends on**: Phase 1, Phase 2
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/shader_setup.h`, `src/render/shader_setup.cpp`

**Build**:
- In `post_effect.h`: Add `Shader inkWashShader;` and uniform location ints: `inkWashResolutionLoc`, `inkWashStrengthLoc`, `inkWashGranulationLoc`, `inkWashBleedStrengthLoc`
- In `post_effect.cpp`:
  - `LoadPostEffectShaders()`: Load `shaders/ink_wash.fs`
  - Success check: `&& pe->inkWashShader.id != 0`
  - `GetShaderUniformLocations()`: Get all 4 uniform locations
  - `SetResolutionUniforms()`: Set resolution on inkWashShader
  - `PostEffectUninit()`: `UnloadShader(pe->inkWashShader)`
- In `shader_setup.h`: Declare `void SetupInkWash(PostEffect* pe);`
- In `shader_setup.cpp`:
  - Add `case TRANSFORM_INK_WASH:` dispatch returning `{ &pe->inkWashShader, SetupInkWash, &pe->effects.inkWash.enabled }`
  - Implement `SetupInkWash()`: set strength, granulation, bleedStrength uniforms

**Verify**: `cmake.exe --build build` compiles. Enable effect in code → shader applies to output.

**Done when**: Shader loads at startup, uniforms resolve, setup function sets all parameters.

---

## Phase 4: UI + Serialization + Modulation

**Goal**: UI controls appear in Style category, presets save/load, modulation routes work.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_style.cpp`, `src/config/preset.cpp`, `src/automation/param_registry.cpp`

**Build**:
- In `imgui_effects.cpp`: Add `case TRANSFORM_INK_WASH:` to `GetTransformCategory()` returning `{"STY", 4}`
- In `imgui_effects_style.cpp`:
  - Add `static bool sectionInkWash = false;`
  - Add `DrawStyleInkWash()` helper with Enabled checkbox + `ModulatableSlider` for strength, granulation, bleedStrength
  - Add call `DrawStyleInkWash(e, modSources, categoryGlow);` with spacing in `DrawStyleCategory()`
- In `preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InkWashConfig, enabled, strength, granulation, bleedStrength)`
  - Add `if (e.inkWash.enabled) { j["inkWash"] = e.inkWash; }` in `to_json`
  - Add `e.inkWash = j.value("inkWash", e.inkWash);` in `from_json`
- In `param_registry.cpp`:
  - Add to `PARAM_TABLE`: `{"inkWash.strength", {0.0f, 2.0f}}`, `{"inkWash.granulation", {0.0f, 1.0f}}`
  - Add matching entries to `targets[]`: `&effects->inkWash.strength`, `&effects->inkWash.granulation`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Ink Wash section in Style category, sliders work, save/load preset preserves values.

**Done when**: UI controls visible, preset round-trips, modulation targets registered.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1, Phase 2 | — |
| 2 | Phase 3, Phase 4 | Wave 1 |

---

## Post-Implementation Notes

Changes made after testing that extend beyond the original plan:

### Added: `bleedRadius` parameter (2026-01-24)

**Reason**: During testing, user needed finer control over color bleed spread to match a reference look. The original Watercolor shader exposed this parameter; Ink Wash had it hardcoded to 5.0 texels.

**Changes**:
- `ink_wash_config.h`: Added `float bleedRadius = 5.0f` (range 1.0–10.0)
- `ink_wash.fs`: Added `uniform float bleedRadius`, replaced hardcoded `5.0` in blur loop
- `post_effect.h/cpp`: Added `inkWashBleedRadiusLoc` uniform location
- `shader_setup.cpp`: Pass `bleedRadius` uniform in `SetupInkWash()`
- `imgui_effects_style.cpp`: Added "Bleed Radius" slider
- `preset.cpp`: Added to serialization macro
- `param_registry.cpp`: Added `inkWash.bleedRadius` (1.0–10.0)
