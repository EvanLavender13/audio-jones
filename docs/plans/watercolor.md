# Watercolor: Gradient-Flow Stroke Tracing

Replace the existing Sobel-edge-darkening watercolor shader with Flockaroo's gradient-flow stroke tracing algorithm. Two trace pairs per pixel produce outline strokes along edges and color wash along gradients. Paper granulation provides the final texture.

## Current State

The watercolor effect exists with full pipeline integration but uses a superseded algorithm (Sobel edges + box blur + directional bleed). All infrastructure stays; only the shader, config, setup, UI, and param registration change.

- `src/config/watercolor_config.h` - Config struct (replace fields)
- `shaders/watercolor.fs` - Fragment shader (rewrite entirely)
- `src/render/post_effect.h:256-263` - Uniform location members (replace)
- `src/render/post_effect.cpp:351-357` - GetShaderLocation calls (replace)
- `src/render/shader_setup.cpp:657-672` - SetupWatercolor function (rewrite)
- `src/ui/imgui_effects_style.cpp:144-165` - DrawStyleWatercolor UI (rewrite)
- `src/config/preset.cpp:163` - NLOHMANN macro (update fields)
- `src/automation/param_registry.cpp:131-133,360-362` - PARAM_TABLE + targets (replace)

## Technical Implementation

- **Source**: Flockaroo (Florian Berger) via `docs/research/watercolor.md`, KinoAqua (Keijiro Takahashi)
- **Pipeline position**: Single-pass style transform (no multi-pass needed)

### Gradient Computation

Central-difference luminance gradient. Points from dark toward bright:

```glsl
vec2 getGrad(vec2 pos, float delta)
{
    vec2 d = vec2(delta, 0.0);
    return vec2(
        dot((texture(texture0, (pos + d.xy) / resolution).rgb -
             texture(texture0, (pos - d.xy) / resolution).rgb), vec3(0.333)),
        dot((texture(texture0, (pos + d.yx) / resolution).rgb -
             texture(texture0, (pos - d.yx) / resolution).rgb), vec3(0.333))
    ) / delta;
}
```

### Stroke Tracing Loop

Four trace positions per pixel. `posA`/`posB` trace perpendicular to gradient (outline strokes). `posC`/`posD` trace along gradient (color wash):

```glsl
// SAMPLES is a uniform int (8-32)
vec3 outlineAccum = vec3(0.0);
vec3 washAccum = vec3(0.0);
float outlineWeight = 0.0;
float washWeight = 0.0;

vec2 posA = pixelPos;
vec2 posB = pixelPos;
vec2 posC = pixelPos;
vec2 posD = pixelPos;

for (int i = 0; i < samples; i++)
{
    float falloff = 1.0 - float(i) / float(samples);

    vec2 grA = getGrad(posA, 2.0) + noiseAmount * (noise(posA) - 0.5);
    vec2 grB = getGrad(posB, 2.0) + noiseAmount * (noise(posB) - 0.5);
    vec2 grC = getGrad(posC, 2.0) + noiseAmount * (noise(posC) - 0.5);
    vec2 grD = getGrad(posD, 2.0) + noiseAmount * (noise(posD) - 0.5);

    // Outlines: step perpendicular to gradient (tangent direction)
    posA += strokeStep * normalize(vec2(grA.y, -grA.x));
    posB -= strokeStep * normalize(vec2(grB.y, -grB.x));

    float edgeStrength = clamp(10.0 * length(grA), 0.0, 1.0);
    float threshold = smoothstep(0.9, 1.1, luminance(posA) * 0.9 + noisePattern(posA));
    outlineAccum += falloff * mix(vec3(1.2), vec3(threshold * 2.0), edgeStrength);
    outlineWeight += falloff;

    // Color wash: step along gradient (dark areas lose color, bright areas gain)
    posC += 0.25 * normalize(grC) + 0.5 * (noise(pixelPos * 0.07) - 0.5);
    posD -= 0.50 * normalize(grD) + 0.5 * (noise(pixelPos * 0.07) - 0.5);

    float w1 = 3.0 * falloff;
    float w2 = 4.0 * (0.7 - falloff);
    washAccum += w1 * (sampleColor(posC) + 0.25 + 0.4 * noise(posC));
    washAccum += w2 * (sampleColor(posD) + 0.25 + 0.4 * noise(posD));
    washWeight += w1 + w2;
}

vec3 outline = outlineAccum / (outlineWeight * 2.5);
vec3 wash = washAccum / (washWeight * 1.65);

// washStrength blends between pure outline (0.0) and full wash coloring (1.0)
vec3 combined = clamp(outline * 0.9 + 0.1, 0.0, 1.0) * mix(vec3(1.0), wash, washStrength);
vec3 result = clamp(combined, 0.0, 1.0);
```

The `noiseAmount` uniform parameterizes the gradient perturbation magnitude (reference uses hardcoded `0.0001`). Range 0.0-0.001 preserves the original scale while allowing artistic control. The `strokeStep` uniform controls outline trace length per iteration. The `washStrength` uniform blends between pure outline strokes (0.0) and full wash coloring (1.0).

### Paper Texture

FBM noise centered around 1.0 (lightens and darkens). Warm paper tint:

```glsl
float paper = fbmNoise(uv * paperScale);
color *= mix(vec3(1.0), vec3(0.93, 0.93, 0.85) * (paper * 0.6 + 0.7), paperStrength);
```

### Noise Functions

Hash-based procedural noise (no texture dependency). Reuse the existing `hash`/`gnoise` pattern already in the current `watercolor.fs`:

```glsl
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

float gnoise(vec3 p) { /* 8-corner trilinear interpolation */ }

// 2D convenience wrappers for stroke tracing
float noise(vec2 p) { return gnoise(vec3(p, 0.0)); }
float noisePattern(vec2 p) { return gnoise(vec3(p * 0.03, 0.0)); }

// 4-octave FBM for paper texture
float fbmNoise(vec2 p) {
    float value = 0.0, amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += gnoise(vec3(p, 0.0)) * amplitude;
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
```

### Helper Functions

```glsl
float luminance(vec2 pos) {
    return dot(texture(texture0, pos / resolution).rgb, vec3(0.333));
}

vec3 sampleColor(vec2 pos) {
    return texture(texture0, pos / resolution).rgb;
}
```

### Combined Pipeline

```glsl
void main()
{
    vec2 pixelPos = fragTexCoord * resolution;

    // 1. Stroke tracing loop (inline the loop above)
    //    ... accumulate outlineAccum, washAccum, weights ...

    vec3 outline = outlineAccum / (outlineWeight * 2.5);
    vec3 wash = washAccum / (washWeight * 1.65);

    // 2. Combine: washStrength blends between outline-only and full wash
    vec3 combined = clamp(outline * 0.9 + 0.1, 0.0, 1.0) * mix(vec3(1.0), wash, washStrength);
    vec3 strokeColor = clamp(combined, 0.0, 1.0);

    // 3. Paper texture
    float paper = fbmNoise(fragTexCoord * paperScale);
    vec3 result = strokeColor * mix(vec3(1.0), vec3(0.93, 0.93, 0.85) * (paper * 0.6 + 0.7), paperStrength);

    finalColor = vec4(result, 1.0);
}
```

## Parameters

| Parameter | Type | Range | Default | Uniform | Modulatable |
|-----------|------|-------|---------|---------|-------------|
| samples | int | 8-32 | 24 | int | No (SliderInt) |
| strokeStep | float | 0.4-2.0 | 1.0 | float | Yes |
| washStrength | float | 0.0-1.0 | 0.7 | float | Yes |
| paperScale | float | 1.0-20.0 | 8.0 | float | No (SliderFloat) |
| paperStrength | float | 0.0-1.0 | 0.4 | float | Yes |
| noiseAmount | float | 0.0-0.001 | 0.0001 | float | Yes |

---

## Phase 1: Config and Shader

**Goal**: Replace config struct and rewrite the shader with the stroke tracing algorithm.
**Depends on**: —
**Files**: `src/config/watercolor_config.h`, `shaders/watercolor.fs`

**Build**:
- Rewrite `watercolor_config.h` with new fields: `enabled`, `samples` (int, default 24), `strokeStep` (float, 1.0), `washStrength` (float, 0.7), `paperScale` (float, 8.0), `paperStrength` (float, 0.4), `noiseAmount` (float, 0.0001)
- Rewrite `watercolor.fs` implementing the gradient-flow stroke tracing algorithm from the Technical Implementation section above. Uniforms: `resolution` (vec2), `samples` (int), `strokeStep` (float), `washStrength` (float), `paperScale` (float), `paperStrength` (float), `noiseAmount` (float)

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: Config struct has new fields, shader implements stroke tracing with paper texture.

---

## Phase 2: Pipeline Integration

**Goal**: Update uniform locations, shader setup, and resolution binding.
**Depends on**: Phase 1
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/shader_setup.cpp`

**Build**:
- In `post_effect.h`: replace the 7 old uniform location ints (lines 257-263) with 6 new ones: `watercolorSamplesLoc`, `watercolorStrokeStepLoc`, `watercolorWashStrengthLoc`, `watercolorPaperScaleLoc`, `watercolorPaperStrengthLoc`, `watercolorNoiseAmountLoc`. Keep `watercolorResolutionLoc` (line 256).
- In `post_effect.cpp` `GetShaderUniformLocations`: replace the old `GetShaderLocation` calls with new uniform names matching the shader
- In `post_effect.cpp` `SetResolutionUniforms`: already has `watercolorShader` resolution binding — no change needed
- In `shader_setup.cpp` `SetupWatercolor`: rewrite to bind the 6 new uniforms from `WatercolorConfig`. Use `SHADER_UNIFORM_INT` for `samples`, `SHADER_UNIFORM_FLOAT` for the rest.

**Verify**: `cmake.exe --build build` compiles without errors.

**Done when**: All new uniforms bind correctly; dispatch entry unchanged (still returns `watercolorShader`).

---

## Phase 3: UI, Presets, and Param Registration

**Goal**: Update UI controls, preset serialization, and modulation registration.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects_style.cpp`, `src/config/preset.cpp`, `src/automation/param_registry.cpp`

**Build**:
- In `imgui_effects_style.cpp` `DrawStyleWatercolor` (lines 144-165): replace body with new controls:
  - `ImGui::SliderInt` for samples (8-32)
  - `ModulatableSlider` for strokeStep ("%.2f")
  - `ModulatableSlider` for washStrength ("%.2f")
  - `ImGui::SliderFloat` for paperScale (1.0-20.0, "%.1f")
  - `ModulatableSlider` for paperStrength ("%.2f")
  - `ModulatableSlider` for noiseAmount ("%.4f")
- In `preset.cpp`: update the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro to list new fields: `enabled, samples, strokeStep, washStrength, paperScale, paperStrength, noiseAmount`
- In `param_registry.cpp`:
  - Replace PARAM_TABLE entries (lines 131-133) with:
    - `{"watercolor.strokeStep", {0.4f, 2.0f}}`
    - `{"watercolor.washStrength", {0.0f, 1.0f}}`
    - `{"watercolor.paperStrength", {0.0f, 1.0f}}`
    - `{"watercolor.noiseAmount", {0.0f, 0.001f}}`
  - Replace targets (lines 360-362) with pointers to corresponding fields. Ensure index alignment: replace 3 old entries with 4 new entries, adjusting all subsequent indices in both PARAM_TABLE and targets arrays.

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Watercolor panel shows new sliders. Enable effect, observe gradient-flow strokes. Save/load preset preserves settings. Route a ModSource to strokeStep and confirm modulation.

**Done when**: UI renders new controls, presets serialize correctly, modulation drives strokeStep/washStrength/paperStrength.

---

## Implementation Notes

Deviations from the original plan, discovered during tuning:

### Removed: `noiseAmount` parameter

The gradient perturbation `noiseAmount * (noise(pos) - 0.5)` adds nothing useful. At the plan's scale (0.0–0.001) it has zero visible effect. At scales large enough to matter (0.0–0.5), the hash-based noise produces blocky static artifacts because it operates at pixel-coordinate granularity. Removed from config, shader, UI, preset serialization, and param registry. The wash traces retain organic variation from their built-in `noise(pixelPos * 0.07)` drift terms.

### Fixed: Wash accumulation weights

The plan's `w2 = 4.0 * (0.7 - falloff)` goes negative for the first 30% of iterations (when `falloff > 0.7`), subtracting color from the accumulator and darkening the result. Fixed with `max(0.0, ...)`.

### Fixed: Wash color bias removed

The plan's `sampleColor(pos) + 0.25 + 0.4 * noise(pos)` adds gray to every sample, desaturating colors before the divisor crushes brightness back down. Removed — wash now accumulates pure sampled colors, preserving vibrancy.

### Fixed: Outline normalization

The plan's `outlineWeight * 2.5` divisor crushes flat-area outline values from 1.2 to 0.48, darkening the entire image. Changed to `outlineWeight * 1.2` which normalizes flat areas to ~1.0 (no brightness change). Added `max(..., vec3(0.7))` floor to prevent harsh black marks at high-contrast edges (e.g., bright ring against dark sphere).

### Fixed: Combination formula

The plan's `clamp(outline * 0.9 + 0.1) * mix(vec3(1.0), wash, washStrength)` shifts outline toward gray then multiplies by wash, compounding two sub-1.0 values. Replaced with `clamp(outline, 0.0, 1.5) * mix(vec3(1.0), wash, washStrength)` — outline passes wash through unchanged in flat areas, darkens at edges only.

### Fixed: Paper texture range

Tightened paper factor from `(paper * 0.6 + 0.7)` (range 0.7–1.3) to `(paper * 0.3 + 0.85)` (range 0.85–1.15). Reduced warm tint from `vec3(0.93, 0.93, 0.85)` to `vec3(0.97, 0.97, 0.93)`.

### Final parameters

| Parameter | Type | Range | Default | Uniform | Modulatable |
|-----------|------|-------|---------|---------|-------------|
| samples | int | 8-32 | 24 | int | No (SliderInt) |
| strokeStep | float | 0.4-2.0 | 1.0 | float | Yes |
| washStrength | float | 0.0-1.0 | 0.7 | float | Yes |
| paperScale | float | 1.0-20.0 | 8.0 | float | No (SliderFloat) |
| paperStrength | float | 0.0-1.0 | 0.4 | float | Yes |
