# Impressionist

Painterly post-process that renders the scene as overlapping circular brush dabs with internal stroke hatching, outlines, edge darkening, and paper grain. Two-pass approach (large then small splats) builds coverage. Sits in the Style category alongside oil paint, watercolor, and pencil sketch.

## Current State

- `docs/research/impressionist.md` - Algorithm reference with GLSL snippets
- `src/config/watercolor_config.h` - Closest existing config pattern (simple float params)
- `src/render/shader_setup.cpp:632` - Watercolor setup function (reference for integration)
- `src/ui/imgui_effects_style.cpp:354` - Watercolor UI placement (new effect goes after it)
- `src/config/effect_config.h:96` - `TRANSFORM_EFFECT_COUNT` sentinel (new enum goes before it)

## Technical Implementation

- **Source**: User-provided Shadertoy code (cherry blossom painterly post-process), documented in `docs/research/impressionist.md`
- **Pipeline Position**: Transform stage, Style category

### Core Algorithm

Two-pass grid-scattered circular splats. Each pass divides the screen into cells of size `maxr`, places `splatCount` random splats per cell, checks 3x3 neighbors for boundary coverage.

#### Pass Structure

```glsl
for (int pass = 0; pass < 2; ++pass) {
    float maxr = mix(splatSizeMin, splatSizeMax, 1.0 - float(pass));
    ivec2 cell = ivec2(floor(uv / maxr));

    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
            for (int k = 0; k < splatCount; ++k) {
                // hash-based RNG seeded from (cell + ivec2(i,j), k, pass)
                vec2 splatPos = (vec2(cell + ivec2(i, j)) + vec2(rand(), rand())) * maxr;
                // render splat at splatPos
            }
}
```

#### Splat Rendering

<!-- Intentional deviation from research: integrates brightness-weighted opacity (pow(brightness, 0.8)) into the main loop, multiplied with strokeOpacity. Research mentions this only in a trailing note but keeps opacity separate from stroke visibility. -->

```glsl
vec3 splatColor = texture(texture0, splatPos).rgb;
float r = mix(0.25, 0.99, pow(rand(), 4.0)) * maxr;
float d = length(uv - splatPos);

// Random rotation for stroke direction
float ang = rand() * 6.283185;
vec2 rotated = mat2(cos(ang), -sin(ang), sin(ang), cos(ang)) * (uv - splatPos);

// Internal stroke hatching
float stroke = cos(rotated.x * strokeFreq) * 0.5 + 0.5;

// Soft circle with feathered edge
float feather = maxr * 0.1;
float fill = clamp((r - d) / feather, 0.0, 1.0);

// Brightness-weighted opacity
float brightness = dot(splatColor, vec3(0.299, 0.587, 0.114));
float opacity = pow(brightness, 0.8) * strokeOpacity;

col = mix(col, splatColor, opacity * stroke * fill);

// Circle outline
float outline = clamp(1.0 - abs(r - d) / 0.002, 0.0, 1.0);
col = mix(col, splatColor * 0.5, outlineStrength * outline);
```

#### Edge Darkening

<!-- Intentional deviation from research: uses resolution-dependent epsilon (1.0/resolution.x) instead of hardcoded 0.001 for correct scaling across screen sizes. -->

```glsl
vec2 eps = vec2(1.0 / resolution.x, 0.0);
vec2 jittered = uv + (noise(uv * 18.0) - 0.5) * 0.01;

float c0 = dot(texture(texture0, jittered).rgb, vec3(0.299, 0.587, 0.114));
float c1 = dot(texture(texture0, jittered + eps.xy).rgb, vec3(0.299, 0.587, 0.114));
float c2 = dot(texture(texture0, jittered + eps.yx).rgb, vec3(0.299, 0.587, 0.114));

float edge = max(abs(c1 - c0), abs(c2 - c0));
col *= mix(0.1, 1.0, 1.0 - clamp(edge * edgeStrength, 0.0, edgeMaxDarken));
```

#### Paper Grain

<!-- Intentional deviation from research: parameterizes grain lower bound (1.0 - grainAmount) instead of hardcoded 0.9, allowing user control of paper texture intensity. -->

```glsl
float grain = mix(1.0 - grainAmount, 1.0, valueNoise(uv * grainScale));
col = sqrt(col * grain) * exposure;
```

### Hash-Based RNG

Use integer hash (no texture lookups) for reproducible per-splat randomness:

```glsl
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
```

Seed each `rand()` call from `vec2(float(cell.x + i) * 131.0 + float(k) * 17.0, float(cell.y + j) * 97.0 + float(pass) * 53.0)` to avoid visible grid patterns.

### Parameters

| Uniform | Type | Default | Range | Effect |
|---------|------|---------|-------|--------|
| resolution | vec2 | screen | - | Texel size for edge detect |
| splatCount | int | 8 | 4-16 | Splats per grid cell |
| splatSizeMin | float | 0.02 | 0.01-0.1 | Small-pass splat radius |
| splatSizeMax | float | 0.12 | 0.05-0.25 | Large-pass splat radius |
| strokeFreq | float | 800.0 | 400-2000 | Internal hatching density |
| strokeOpacity | float | 0.7 | 0-1 | Hatching visibility |
| outlineStrength | float | 0.15 | 0-0.5 | Splat outline darkness |
| edgeStrength | float | 4.0 | 0-8 | Edge darkening intensity |
| edgeMaxDarken | float | 0.15 | 0-0.3 | Max edge darkening amount |
| grainScale | float | 400.0 | 100-800 | Paper texture frequency |
| grainAmount | float | 0.1 | 0-0.2 | Paper texture intensity |
| exposure | float | 1.0 | 0.5-2.0 | Final brightness multiplier |

### Modulation Candidates (4 params registered)

- `impressionist.splatSizeMax` (0.05-0.25) — larger = looser/more abstract
- `impressionist.strokeFreq` (400-2000) — higher = finer hatching texture
- `impressionist.edgeStrength` (0-8) — emphasizes contours on transients
- `impressionist.strokeOpacity` (0-1) — fades between flat dabs and textured strokes

---

## Phase 1: Config and Registration

**Goal**: Create config struct and register the effect type in the framework.
**Depends on**: —
**Files**: `src/config/impressionist_config.h`, `src/config/effect_config.h`

**Build**:
- Create `src/config/impressionist_config.h` with `ImpressionistConfig` struct: `enabled` (false), `splatCount` (8), `splatSizeMin` (0.02f), `splatSizeMax` (0.12f), `strokeFreq` (800.0f), `strokeOpacity` (0.7f), `outlineStrength` (0.15f), `edgeStrength` (4.0f), `edgeMaxDarken` (0.15f), `grainScale` (400.0f), `grainAmount` (0.1f), `exposure` (1.0f)
- Modify `src/config/effect_config.h`:
  - Add `#include "impressionist_config.h"`
  - Add `TRANSFORM_IMPRESSIONIST` enum before `TRANSFORM_EFFECT_COUNT`
  - Add case in `TransformEffectName()` returning `"Impressionist"`
  - Add to `TransformOrderConfig::order` array
  - Add `ImpressionistConfig impressionist;` member to `EffectConfig`
  - Add case in `IsTransformEnabled()` returning `e->impressionist.enabled`

**Verify**: `cmake.exe --build build` succeeds with no errors.

**Done when**: `TRANSFORM_IMPRESSIONIST` compiles and `ImpressionistConfig` defaults initialize.

---

## Phase 2: Shader

**Goal**: Implement the fragment shader with two-pass splat algorithm, edge darkening, and paper grain.
**Depends on**: —
**Files**: `shaders/impressionist.fs`

**Build**:
- Create `shaders/impressionist.fs` implementing the full algorithm from Technical Implementation above:
  - Hash-based RNG (`hash` function using `fract`/`dot` pattern)
  - Two-pass grid-scattered splat loop (`splatSizeMin`→`splatSizeMax` range, 3x3 neighbor cells)
  - Per-splat rendering: color sample at splat center, random radius, random rotation, stroke hatching (`strokeFreq`), feathered fill, brightness-weighted opacity (`strokeOpacity`), circle outline (`outlineStrength`)
  - Edge darkening: jittered luminance gradient with `edgeStrength` and `edgeMaxDarken`
  - Paper grain: value noise at `grainScale` modulated by `grainAmount`, gamma via `sqrt`
- Uniforms: `resolution`, `splatCount`, `splatSizeMin`, `splatSizeMax`, `strokeFreq`, `strokeOpacity`, `outlineStrength`, `edgeStrength`, `edgeMaxDarken`, `grainScale`, `grainAmount`, `exposure`

**Verify**: File exists and GLSL syntax is valid (checked at runtime in Phase 3).

**Done when**: Shader file contains complete algorithm matching Technical Implementation section.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader, declare uniform locations, wire into render pipeline.
**Depends on**: Phase 1, Phase 2
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader impressionistShader;`
  - Add uniform location ints: `impressionistResolutionLoc`, `impressionistSplatCountLoc`, `impressionistSplatSizeMinLoc`, `impressionistSplatSizeMaxLoc`, `impressionistStrokeFreqLoc`, `impressionistStrokeOpacityLoc`, `impressionistOutlineStrengthLoc`, `impressionistEdgeStrengthLoc`, `impressionistEdgeMaxDarkenLoc`, `impressionistGrainScaleLoc`, `impressionistGrainAmountLoc`, `impressionistExposureLoc`
- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get uniform locations in `GetShaderUniformLocations()`
  - Add resolution uniform in `SetResolutionUniforms()`
  - Unload shader in `PostEffectUninit()`

**Verify**: `cmake.exe --build build` succeeds. Application starts without shader load failure.

**Done when**: Shader loads and all uniform locations resolve.

---

## Phase 4: Shader Setup

**Goal**: Implement per-frame uniform upload dispatching config values to the shader.
**Depends on**: Phase 3
**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupImpressionist(PostEffect* pe);`
- Modify `src/render/shader_setup.cpp`:
  - Add dispatch case in `GetTransformEffect()`:
    ```
    case TRANSFORM_IMPRESSIONIST:
        return { &pe->impressionistShader, SetupImpressionist, &pe->effects.impressionist.enabled };
    ```
  - Implement `SetupImpressionist()`:
    - Upload all uniforms: `splatCount`, `splatSizeMin`, `splatSizeMax`, `strokeFreq`, `strokeOpacity`, `outlineStrength`, `edgeStrength`, `edgeMaxDarken`, `grainScale`, `grainAmount`, `exposure`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → enable Impressionist → painterly brush dab effect renders over the visualizer.

**Done when**: Effect renders correctly with circular splats, stroke hatching, edge darkening, and paper grain.

---

## Phase 5: UI Panel

**Goal**: Add Impressionist controls to the Style category panel.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects_style.cpp`, `src/ui/imgui_effects.cpp`

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add `TRANSFORM_IMPRESSIONIST` case in `GetTransformCategory()` returning `{"STY", 4}`
- Modify `src/ui/imgui_effects_style.cpp`:
  - Add `static bool sectionImpressionist = false;` with other section states
  - Add `DrawStyleImpressionist()` function:
    - Checkbox for enabled (with `MoveTransformToEnd` on first enable)
    - `ModulatableSlider` for `splatSizeMax` (0.05–0.25)
    - `ModulatableSlider` for `strokeFreq` (400.0–2000.0)
    - `ModulatableSlider` for `edgeStrength` (0.0–8.0)
    - `ModulatableSlider` for `strokeOpacity` (0.0–1.0)
    - `ImGui::SliderInt` for `splatCount` (4–16)
    - `ImGui::SliderFloat` for `splatSizeMin` (0.01–0.1)
    - `ImGui::SliderFloat` for `outlineStrength` (0.0–0.5)
    - `ImGui::SliderFloat` for `edgeMaxDarken` (0.0–0.3)
    - `ImGui::SliderFloat` for `grainScale` (100.0–800.0)
    - `ImGui::SliderFloat` for `grainAmount` (0.0–0.2)
    - `ImGui::SliderFloat` for `exposure` (0.5–2.0)
  - Add call in `DrawStyleCategory()` after the last Style entry with `ImGui::Spacing()`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Style section shows "Impressionist" with all sliders. Controls modify effect in real-time.

**Done when**: All parameters adjustable via UI, effect responds to slider changes.

---

## Phase 6: Preset Serialization

**Goal**: Save/load Impressionist settings in preset JSON.
**Depends on**: Phase 1
**Files**: `src/config/preset.cpp`

**Build**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ImpressionistConfig, enabled, splatCount, splatSizeMin, splatSizeMax, strokeFreq, strokeOpacity, outlineStrength, edgeStrength, edgeMaxDarken, grainScale, grainAmount, exposure)`
- Add `to_json` entry: `if (e.impressionist.enabled) { j["impressionist"] = e.impressionist; }`
- Add `from_json` entry: `e.impressionist = j.value("impressionist", e.impressionist);`

**Verify**: Save a preset with Impressionist enabled, reload it → settings persist.

**Done when**: Round-trip preset save/load preserves all ImpressionistConfig fields.

---

## Phase 7: Parameter Registration

**Goal**: Register modulatable parameters for LFO/audio-reactive control.
**Depends on**: Phase 1
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Add to `PARAM_TABLE` (4 entries, matching indices with targets):
  - `{"impressionist.splatSizeMax", {0.05f, 0.25f}}`
  - `{"impressionist.strokeFreq", {400.0f, 2000.0f}}`
  - `{"impressionist.edgeStrength", {0.0f, 8.0f}}`
  - `{"impressionist.strokeOpacity", {0.0f, 1.0f}}`
- Add to `targets` array (same 4 entries, same order):
  - `&effects->impressionist.splatSizeMax`
  - `&effects->impressionist.strokeFreq`
  - `&effects->impressionist.edgeStrength`
  - `&effects->impressionist.strokeOpacity`

**Verify**: Assign modulation route to `impressionist.splatSizeMax` → splat size responds to audio.

**Done when**: All four parameters respond to modulation sources.

---

## Implementation Notes (Post-Phase Shader Rewrite)

Initial implementation produced visible grid artifacts and obvious circles rather than a painterly result. Root cause: the research doc extracted algorithm structure correctly but missed coordinate space implications and per-splat randomness that makes the effect organic.

### Problems Fixed

1. **Coordinate space**: Original operates in centered, aspect-corrected `[-aspect/2, aspect/2] × [-0.5, 0.5]` space. Initial implementation used raw `[0,1]` UV, causing all distance thresholds (outline width, feather, stroke frequency) to produce wrong visual density.

2. **Canvas vs source bleed-through**: Original starts from a dark canvas (`vec3(0.133, 0.133, 0.167)`) and paints the image entirely via splats. Initial implementation started from `texture(texture0, uv).rgb`, making the effect a partial overlay rather than a full repaint.

3. **Staggered grid**: Original uses `((cell.x+i)&1)==1 ? -j0 : j0` to alternate neighbor iteration per row. Missing this caused visible lattice patterns.

4. **Per-splat randomness**: Original varies opacity (`mix(0.1, 0.4, rand()) * 3.0` = 0.3–1.2 range) and feather width (`mix(0.001, 0.004, rand())`) per splat. Initial implementation used fixed values, producing mechanical uniformity.

5. **RNG**: Original uses `fract(sin(seed++) * 43758.545)` with seed from cell coords. Initial implementation used a different hash function with different seeding, producing different distribution patterns.

6. **Stroke modulation range**: Original modulates stroke in `[0.8, 1.0]` (subtle texture). Initial implementation applied it as a full `[0, 1]` factor, making hatching lines overpowering.

### Config Default Changes

| Parameter | Before | After | Reason |
|-----------|--------|-------|--------|
| splatCount | 8 | 11 | Matches original loop count |
| splatSizeMin | 0.02 | 0.018 | Matches `1/22 * 0.4` |
| splatSizeMax | 0.12 | 0.1 | Matches `1/4 * 0.4` |
| strokeFreq | 800 | 1200 | Matches original hardcoded value |
| outlineStrength | 0.15 | 1.0 | Now acts as multiplier on per-splat random alpha |
| edgeMaxDarken | 0.15 | 0.13 | Matches original |
| exposure | 1.0 | 1.28 | Matches original final multiply |

### UI Changes

- Outline Strength slider range: `[0, 0.5]` → `[0, 1.0]`
