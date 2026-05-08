# Quadtree

A Texture-category generator that draws a recursive quadtree subdivision around N moving source points. Each pixel walks the tree, halving its bounding box and choosing the quadrant containing the pixel; subdivision continues only while at least one source lies inside the current cell. Final cell depth (or a per-cell hash) drives `t`, which feeds both the gradient LUT and the standard generator FFT band lookup. Cells render as pixel-thin outlines with an optional fill amount.

**Research**: `docs/research/quadtree.md`

## Design

### Types

`QuadtreeConfig` (in `src/effects/quadtree.h`):

```cpp
struct QuadtreeConfig {
  bool enabled = false;

  // Audio (standard generator FFT)
  float baseFreq = 55.0f;    // 27.5-440
  float maxFreq = 14000.0f;  // 1000-16000
  float gain = 2.0f;         // 0.1-10
  float curve = 1.5f;        // 0.1-3
  float baseBright = 0.15f;  // 0-1

  // Geometry
  int maxIterations = 8;       // 1-8 (quadtree depth cap)
  float lineWidth = 2.0f;      // 0.5-4.0 (pixels)
  float cellFillAmount = 0.0f; // 0.0-1.0 (interior fill brightness)

  // Sources
  int pointCount = 6;            // 1-8
  float baseRadius = 0.5f;       // 0.0-1.0
  DualLissajousConfig lissajous; // Shared Lissajous controls (motion)

  // Color
  int colorMode = 0; // 0=depth, 1=hash

  // Color/Output (standard generator)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // 0.0-5.0
};

#define QUADTREE_CONFIG_FIELDS                                                 \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, maxIterations,          \
      lineWidth, cellFillAmount, pointCount, baseRadius, lissajous, colorMode, \
      gradient, blendMode, blendIntensity
```

`QuadtreeEffect` (typedef struct in same header):

```cpp
typedef struct QuadtreeEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  int resolutionLoc;
  int sourcesLoc;
  int pointCountLoc;
  int maxIterationsLoc;
  int lineWidthLoc;
  int cellFillAmountLoc;
  int colorModeLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} QuadtreeEffect;
```

Public functions (declared in header):

```cpp
bool QuadtreeEffectInit(QuadtreeEffect *e, const QuadtreeConfig *cfg);
void QuadtreeEffectSetup(QuadtreeEffect *e, QuadtreeConfig *cfg, float deltaTime, const Texture2D &fftTexture);
void QuadtreeEffectUninit(QuadtreeEffect *e);
void QuadtreeRegisterParams(QuadtreeConfig *cfg);
QuadtreeEffect *GetQuadtreeEffect(PostEffect *pe);
```

`Setup` is non-const on `cfg` because `DualLissajousUpdateCircular` mutates the embedded Lissajous phase each frame (same reason as `RippleTankEffectSetup`).

### Algorithm (full shader source — single source of truth)

Save as `shaders/quadtree.fs`:

```glsl
// Based on "Quadtree Random Points" by Bingle
// https://www.shadertoy.com/view/wc2XRh
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced parametric/mouse points with CPU-driven Lissajous source
// array, replaced grayscale outline with gradient LUT + FFT band lookup,
// exposed maxIterations, lineWidth, cellFillAmount, colorMode as uniforms
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec2 sources[8];
uniform int pointCount;
uniform int maxIterations;
uniform float lineWidth;
uniform float cellFillAmount;
uniform int colorMode; // 0=depth, 1=hash
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

bool inBox(vec2 lo, vec2 hi, vec2 p) {
    return lo.x <= p.x && lo.y <= p.y && hi.x > p.x && hi.y > p.y;
}

void main() {
    // toUv: center fragCoord, divide by resolution.y so cells are square
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 uv = 2.0 * (fragCoord - 0.5 * resolution) / resolution.y;

    vec2 lo = floor(uv);
    vec2 hi = ceil(uv);
    uv = mod(uv, vec2(1.0));

    int iters = 0;
    while (iters < maxIterations) {
        iters++;
        bool sub = false;
        for (int i = 0; i < pointCount; ++i) {
            if (inBox(lo, hi, sources[i])) {
                sub = true;
                break;
            }
        }
        if (sub) {
            vec2 center = (hi + lo) * 0.5;
            if (uv.x > 0.5) { lo.x = center.x; uv.x = uv.x * 2.0 - 1.0; }
            else            { hi.x = center.x; uv.x *= 2.0; }
            if (uv.y > 0.5) { lo.y = center.y; uv.y = uv.y * 2.0 - 1.0; }
            else            { hi.y = center.y; uv.y *= 2.0; }
        } else {
            break;
        }
    }

    // Outline: smoothstep on distance to cell edge in cell-local UV.
    // The pow(2.0, iters)/resolution.y factor converts pixels to cell-local
    // UV at the current depth so lineWidth stays a screen-pixel quantity.
    float edge = max(abs(uv.x - 0.5), abs(uv.y - 0.5)) * 2.0;
    float outlineThreshold = 1.0 - lineWidth * pow(2.0, float(iters)) / resolution.y;
    float outline = smoothstep(outlineThreshold, 1.0, edge);

    float coverage = outline + cellFillAmount * (1.0 - outline);

    float t;
    if (colorMode == 0) {
        t = float(iters) / float(maxIterations);
    } else {
        t = fract(sin(dot(lo, vec2(12.9898, 78.233))) * 43758.5453);
    }

    vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; ++s) {
        float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) / float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 col = baseColor * brightness * coverage;
    finalColor = vec4(col, 1.0);
}
```

### CPU source-position computation (in `QuadtreeEffectSetup`)

Mirror Ripple Tank's pattern exactly:

```cpp
float sources[16]; // 8 * vec2
int count = cfg->pointCount;
if (count < 1) { count = 1; }
else if (count > 8) { count = 8; }
DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius,
                            0.0f, 0.0f, count, sources);
SetShaderValueV(e->shader, e->sourcesLoc, sources, SHADER_UNIFORM_VEC2, count);
SetShaderValue(e->shader, e->pointCountLoc, &count, SHADER_UNIFORM_INT);
```

`(centerX, centerY) = (0, 0)` matches the shader's centered `toUv` space; `baseRadius` controls the outer orbit radius. The Lissajous offsets ride on top of the circular arrangement.

### Setup body (full content for `QuadtreeEffectSetup`)

```cpp
void QuadtreeEffectSetup(QuadtreeEffect *e, QuadtreeConfig *cfg,
                         float deltaTime, const Texture2D &fftTexture) {
  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Source array
  float sources[16];
  int count = cfg->pointCount;
  if (count < 1) { count = 1; }
  else if (count > 8) { count = 8; }
  DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius,
                              0.0f, 0.0f, count, sources);
  SetShaderValueV(e->shader, e->sourcesLoc, sources, SHADER_UNIFORM_VEC2, count);
  SetShaderValue(e->shader, e->pointCountLoc, &count, SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->maxIterationsLoc, &cfg->maxIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lineWidthLoc, &cfg->lineWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellFillAmountLoc, &cfg->cellFillAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);

  // FFT (standard generator)
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}
```

### Bridge functions (non-static, at bottom of `quadtree.cpp` above the macro)

```cpp
QuadtreeEffect *GetQuadtreeEffect(PostEffect *pe) {
  return (QuadtreeEffect *)pe->effectStates[TRANSFORM_QUADTREE];
}

void SetupQuadtree(PostEffect *pe) {
  QuadtreeEffectSetup(GetQuadtreeEffect(pe), &pe->effects.quadtree,
                      pe->currentDeltaTime, pe->fftTexture);
}

void SetupQuadtreeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.quadtree.blendIntensity,
                       pe->effects.quadtree.blendMode);
}
```

### UI (colocated, static `DrawQuadtreeParams` above bridges)

Section order per ui-guide: Audio, Geometry, Sources, Color. Output handled by `STANDARD_GENERATOR_OUTPUT(quadtree)`.

```cpp
static void DrawQuadtreeParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  QuadtreeConfig *cfg = &e->quadtree;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##quadtree", &cfg->baseFreq,
                    "quadtree.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##quadtree", &cfg->maxFreq,
                    "quadtree.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##quadtree", &cfg->gain, "quadtree.gain", "%.1f", ms);
  ModulatableSlider("Contrast##quadtree", &cfg->curve, "quadtree.curve",
                    "%.2f", ms);
  ModulatableSlider("Base Bright##quadtree", &cfg->baseBright,
                    "quadtree.baseBright", "%.2f", ms);

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##quadtree", &cfg->maxIterations, 1, 8);
  ModulatableSlider("Line Width##quadtree", &cfg->lineWidth,
                    "quadtree.lineWidth", "%.2f", ms);
  ModulatableSlider("Fill##quadtree", &cfg->cellFillAmount,
                    "quadtree.cellFillAmount", "%.2f", ms);

  ImGui::SeparatorText("Sources");
  ImGui::SliderInt("Source Count##quadtree", &cfg->pointCount, 1, 8);
  ModulatableSlider("Base Radius##quadtree", &cfg->baseRadius,
                    "quadtree.baseRadius", "%.2f", ms);
  DrawLissajousControls(&cfg->lissajous, "quadtree_liss",
                        "quadtree.lissajous", ms, 0.2f);

  ImGui::SeparatorText("Color");
  ImGui::Combo("Color Mode##quadtree", &cfg->colorMode, "Depth\0Hash\0");
}
```

### Registration macro (bottom of `quadtree.cpp`)

```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(quadtree)
REGISTER_GENERATOR(TRANSFORM_QUADTREE, Quadtree, quadtree,
                   "Quadtree", SetupQuadtreeBlend,
                   SetupQuadtree, 12, DrawQuadtreeParams, DrawOutput_quadtree)
// clang-format on
```

### RegisterParams body

```cpp
void QuadtreeRegisterParams(QuadtreeConfig *cfg) {
  ModEngineRegisterParam("quadtree.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("quadtree.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("quadtree.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("quadtree.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("quadtree.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("quadtree.lineWidth", &cfg->lineWidth, 0.5f, 4.0f);
  ModEngineRegisterParam("quadtree.cellFillAmount", &cfg->cellFillAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("quadtree.baseRadius", &cfg->baseRadius, 0.0f, 1.0f);
  ModEngineRegisterParam("quadtree.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("quadtree.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("quadtree.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}
```

`pointCount`, `maxIterations`, `colorMode` are int and not registered as modulation targets.

### Init body

```cpp
bool QuadtreeEffectInit(QuadtreeEffect *e, const QuadtreeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/quadtree.fs");
  if (e->shader.id == 0) { return false; }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->sourcesLoc = GetShaderLocation(e->shader, "sources");
  e->pointCountLoc = GetShaderLocation(e->shader, "pointCount");
  e->maxIterationsLoc = GetShaderLocation(e->shader, "maxIterations");
  e->lineWidthLoc = GetShaderLocation(e->shader, "lineWidth");
  e->cellFillAmountLoc = GetShaderLocation(e->shader, "cellFillAmount");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }
  return true;
}

void QuadtreeEffectUninit(QuadtreeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | - | false | - | (auto) |
| baseFreq | float | 27.5-440 | 55.0 | yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000.0 | yes | "Max Freq (Hz)" |
| gain | float | 0.1-10 | 2.0 | yes | "Gain" |
| curve | float | 0.1-3 | 1.5 | yes | "Contrast" |
| baseBright | float | 0-1 | 0.15 | yes | "Base Bright" |
| maxIterations | int | 1-8 | 8 | no | "Iterations" |
| lineWidth | float | 0.5-4.0 | 2.0 | yes | "Line Width" |
| cellFillAmount | float | 0.0-1.0 | 0.0 | yes | "Fill" |
| pointCount | int | 1-8 | 6 | no | "Source Count" |
| baseRadius | float | 0.0-1.0 | 0.5 | yes | "Base Radius" |
| lissajous.amplitude | float | 0.0-0.5 | (struct default) | yes | (DrawLissajousControls) |
| lissajous.motionSpeed | float | 0.0-5.0 | (struct default) | yes | (DrawLissajousControls) |
| colorMode | int | 0-1 | 0 | no | "Color Mode" |
| gradient | ColorConfig | - | `{.mode = COLOR_MODE_GRADIENT}` | - | (gradient widget via STANDARD_GENERATOR_OUTPUT) |
| blendMode | EffectBlendMode | - | EFFECT_BLEND_SCREEN | - | (combo via STANDARD_GENERATOR_OUTPUT) |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | (slider via STANDARD_GENERATOR_OUTPUT) |

### Constants

- Enum name: `TRANSFORM_QUADTREE` (no `_BLEND` suffix, matches recent generators like `TRANSFORM_LICHEN`, `TRANSFORM_RANDOM_VOLUMETRIC`)
- Display name: `"Quadtree"`
- Field name: `quadtree`
- Section index: `12` (Texture)
- Macro: `REGISTER_GENERATOR` (bakes `"GEN"` badge and `EFFECT_FLAG_BLEND`)
- Param prefix: `"quadtree."`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create `src/effects/quadtree.h`

**Files**: `src/effects/quadtree.h`
**Creates**: `QuadtreeConfig`, `QuadtreeEffect`, `QUADTREE_CONFIG_FIELDS` macro, public function declarations.

**Do**: Follow the structure of `src/effects/subdivide.h`. Include `"config/dual_lissajous_config.h"`, `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Include guard `QUADTREE_H`. Forward-declare `struct PostEffect;`. Define `QuadtreeConfig` and `QuadtreeEffect` exactly as specified in the Design > Types section. Define `QUADTREE_CONFIG_FIELDS` macro listing every field in the order shown. Declare the five public functions in the order shown.

Per conventions, do NOT add inline comments restating field-meaning unless the comment names a constraint not visible from the type. Range comments (e.g., `// 27.5-440`) are conventional for config struct fields and should be kept.

**Verify**: `cmake.exe --build build` compiles. Header parses standalone.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: Create `src/effects/quadtree.cpp`

**Files**: `src/effects/quadtree.cpp`
**Depends on**: Wave 1 complete

**Do**: Follow the structure of `src/effects/subdivide.cpp`. Includes (clang-format will sort within groups):

- `"quadtree.h"`
- `"audio/audio.h"` (for `AUDIO_SAMPLE_RATE`)
- `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`
- `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`
- `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`
- `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
- `<stddef.h>`

Implement the bodies in the Design section verbatim:

- `QuadtreeEffectInit` (Init body)
- `QuadtreeEffectSetup` (Setup body — note non-const `cfg` because `DualLissajousUpdateCircular` mutates phase)
- `QuadtreeEffectUninit`
- `QuadtreeRegisterParams` (RegisterParams body)
- `GetQuadtreeEffect` (bridge accessor — non-static)
- `SetupQuadtree` (bridge — non-static)
- `SetupQuadtreeBlend` (bridge — non-static)
- `// === UI ===` section with `DrawQuadtreeParams` (static)
- `STANDARD_GENERATOR_OUTPUT(quadtree)` + `REGISTER_GENERATOR(...)` block at bottom, wrapped in `// clang-format off` / `// clang-format on`

Place `// === UI ===` comment after the bridges and before `DrawQuadtreeParams`. Confirm bridges are non-static; only `DrawQuadtreeParams` is static.

**Verify**: `cmake.exe --build build` compiles. Effect appears under TRANSFORMS > Texture in the UI. Toggling enabled and tweaking sliders affects the rendered output. Modulation engine accepts the registered IDs.

---

#### Task 2.2: Create `shaders/quadtree.fs`

**Files**: `shaders/quadtree.fs`
**Depends on**: Wave 1 complete (uniform set is determined by Wave 1 header but does not require it to be present)

**Do**: Implement the Algorithm section verbatim. Begin with the four-line attribution comment block before `#version 330`. Do not paraphrase, reorder, or inline-merge any of the algorithm steps — copy the GLSL exactly as written in the Design > Algorithm section.

**Verify**: `cmake.exe --build build` succeeds (shader is loaded at runtime, not compiled at build, so verification is a runtime check). Run the app, enable Quadtree from the UI, confirm cells render and FFT energy modulates brightness on active sources.

---

#### Task 2.3: Modify `src/config/effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete (needs `quadtree.h` to define `QuadtreeConfig`)

**Do**: Three additions:

1. Include `"effects/quadtree.h"` alphabetically among the existing `effects/` includes (insert between `puzzle.h` and `radial_blur.h`, or wherever clang-format places it — the pre-commit hook will sort the group).
2. Add `TRANSFORM_QUADTREE` to the `TransformEffectType` enum just before `TRANSFORM_ACCUM_COMPOSITE`. Match the placement style of recent additions like `TRANSFORM_PILLAR_GRID` and `TRANSFORM_PUZZLE`.
3. Add `QuadtreeConfig quadtree;` member to `EffectConfig` struct in the same alphabetical band as the existing config members.

Do NOT modify `TransformOrderConfig`. Its constructor populates `order[]` from the enum loop, so the new enum value flows through automatically.

Do NOT modify `src/render/post_effect.h`. The descriptor system stores `QuadtreeEffect` state via the `effectStates[TRANSFORM_QUADTREE]` slot; no named member is needed (matches Subdivide).

**Verify**: `cmake.exe --build build` compiles. Enum count increases by 1. Existing effects continue to function.

---

#### Task 2.4: Modify `src/config/effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete (needs `quadtree.h` for `QuadtreeConfig` and `QUADTREE_CONFIG_FIELDS`)

**Do**: Three additions:

1. Include `"effects/quadtree.h"` among the other `effects/` includes (clang-format sorts).
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(QuadtreeConfig, QUADTREE_CONFIG_FIELDS)` in alphabetical order with the other config macros.
3. Add `X(quadtree) \` to the `EFFECT_CONFIG_FIELDS(X)` table in alphabetical order with the other entries (between `puzzle` and `radialBlur` if those are siblings; clang-format does not sort macro continuations, so place manually).

**Verify**: `cmake.exe --build build` compiles. Save a preset with Quadtree enabled and a few non-default values; reload it and confirm fields round-trip.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Quadtree appears under TRANSFORMS > Texture (section 12) with `"GEN"` badge
- [ ] Enabling Quadtree adds it to the pipeline list and renders cells
- [ ] Source Count slider 1-8 changes the number of orbiting points; Base Radius scales orbit
- [ ] Lissajous controls modulate point motion live
- [ ] Iterations slider 1-8 caps subdivision depth visibly
- [ ] Line Width slider 0.5-4.0 scales outline thickness in pixels
- [ ] Fill slider raises interior cell brightness from 0 to full
- [ ] Color Mode = Depth: cells at deeper recursion sample later in the gradient and react to higher FFT bands
- [ ] Color Mode = Hash: cells get scattered colors; FFT activity spreads across the canvas
- [ ] All registered modulation IDs (`quadtree.baseFreq`, `quadtree.lineWidth`, `quadtree.lissajous.motionSpeed`, etc.) accept route assignments
- [ ] Preset save/load round-trips all `QUADTREE_CONFIG_FIELDS` values
- [ ] Disabling the effect clears the modulation routes for `quadtree.*` IDs
