# Drekker Paint

Painterly transform effect that slices the input image into a diagonal grid of curved parallelogram cells. Each cell samples one color region from the source and sweeps it into a brushstroke shape using reciprocal curvature functions, producing bold color blocks with visible dark grid lines between them. Adapts Cotterzz's "Max Drekker Paint effect v2" Shadertoy (CC BY-NC-SA 3.0) into a single-pass transform in the Painterly category.

**Research**: `docs/research/drekker_paint.md`

## Design

### Types

`src/effects/drekker_paint.h`:

```cpp
struct DrekkerPaintConfig {
  bool enabled = false;
  float xDiv = 11.0f;          // Horizontal cell count (2-30)
  float yDiv = 7.0f;           // Vertical cell count (2-20)
  float curve = 0.01f;         // Reciprocal curvature intensity at cell edges (0.001-0.1)
  float gapSize = 0.1f;        // Alpha threshold for visible gap between strokes (0.01-0.4)
  float diagSlant = 0.1f;      // Diagonal slant factor, rx multiplier (0.0-0.3)
  float strokeSpread = 200.0f; // Vertical stroke sample spread, pixels at 1080p (50-500)
};

#define DREKKER_PAINT_CONFIG_FIELDS enabled, xDiv, yDiv, curve, gapSize, diagSlant, strokeSpread

typedef struct DrekkerPaintEffect {
  Shader shader;
  int resolutionLoc;
  int xDivLoc;
  int yDivLoc;
  int curveLoc;
  int gapSizeLoc;
  int diagSlantLoc;
  int strokeSpreadLoc;
} DrekkerPaintEffect;

bool DrekkerPaintEffectInit(DrekkerPaintEffect *e);
void DrekkerPaintEffectSetup(const DrekkerPaintEffect *e, const DrekkerPaintConfig *cfg);
void DrekkerPaintEffectUninit(const DrekkerPaintEffect *e);
void DrekkerPaintRegisterParams(DrekkerPaintConfig *cfg);
```

### Algorithm

Single-pass fragment shader. For each output pixel, sample the source texture 4 times at quasi-random R2-sequence offsets, averaging the results. Each sample locates a curved parallelogram cell, picks a representative color, and masks the cell boundaries as transparent so they composite to black in the final output.

**Coordinate flow**: The reference's `mainImage` takes pixel-space `fragCoord`, adds the R2 AA offset in pixel space, and passes into `mainImage0`, which normalizes `fragCoord / iResolution.y` internally. This pixel→UV boundary must be preserved — the AA offset is subpixel jitter, not a UV offset. Main computes `fragCoord = fragTexCoord * resolution`, offsets in pixels, then `sampleCell` normalizes internally.

**Per-sample function `sampleCell(fragCoord)`** returns `vec4(rgb, alphaMask)`. First line: `vec2 uv = fragCoord / resolution.y` (reference verbatim — produces square cells in pixel space, `uv.x` in [0, aspect], `uv.y` in [0, 1]). On a 16:9 viewport with `xDiv=11`, `rx` reaches ~19, which drives the visible cell density and the default diagonal slant. Then:

1. `rx = floor(uv.x * xDiv)` — horizontal cell index
2. `rix = fract(uv.x * xDiv)` — position within cell, [0, 1)
3. `rixl = (curve / rix) - curve` — left-edge reciprocal curvature (diverges as `rix → 0`)
4. `rixr = (curve / (1.0 - rix)) - curve` — right-edge reciprocal curvature
5. `ry = floor((uv.y + rx * diagSlant) * yDiv + rixl - rixr)` — vertical cell index with diagonal offset and curvature
6. `riy = fract((uv.y + rx * diagSlant) * yDiv + rixl - rixr) - 0.5` — position within vertical cell, [-0.5, 0.5]
7. `riy = riy * riy * riy` — cubic curvature stretching
8. Sample: `sampleUV = vec2(rx / (xDiv * aspect), (riy * strokeSpread / REFERENCE_HEIGHT) + (ry / yDiv) - rx * diagSlant)` where `aspect = resolution.x / resolution.y` and `REFERENCE_HEIGHT = 1080.0`. Dividing `rx/xDiv` by aspect keeps horizontal sampling in [0, 1) on non-square screens — the reference relies on Shadertoy's default REPEAT wrap, which produces a horizontal wallpaper tile. Scaling by aspect maps all horizontal cells to distinct source positions instead.
9. `c = textureLod(texture0, sampleUV, 0.0)` — LOD 0 mandatory to avoid mipmap seams at UV discontinuities
10. Alpha mask: if `abs(riy) > 0.125 * (1.0 - gapSize) || rix > (1.0 - gapSize) || rix < gapSize` then `c.a = 0.0`. The `0.125 * (1 - gapSize)` form (rather than the reference's literal `0.1`) makes `gapSize=0` disable the riy mask entirely — matches the rix condition's direction so both cuts scale together. At `gapSize=0.1`, threshold is `0.125 * 0.9 = 0.1125 ≈ 0.1`, close to the reference. Intentional deviation from strict transcription: the literal substitution produces opposing semantics on the two conditions, making the unified `gapSize` uniform unusable.
11. Return `c`

**Main function**: 4-sample anti-aliasing with R2 quasi-random offsets (plastic constant conjugates, kept verbatim). Offset is in pixel space — ±0.5 pixel jitter, not UV space:

```glsl
vec2 fragCoord = fragTexCoord * resolution;
vec2 offset = vec2(0.5);
vec4 accum = vec4(0.0);
for (int k = 0; k < 4; k++) {
    accum += sampleCell(fragCoord + offset - 0.5);
    offset = fract(offset + vec2(0.569840296, 0.754877669));
}
accum /= 4.0;
finalColor = vec4(accum.rgb * accum.a, 1.0);
```

The final `rgb * alpha` premultiply renders gaps as black (matches Shadertoy reference display where `alpha=0` pixels composite onto the black framebuffer).

**Full shader `shaders/drekker_paint.fs`**:

```glsl
// Based on "Max Drekker Paint effect v2" by Cotterzz
// https://www.shadertoy.com/view/33SSDm
// Fork of "Max Drekker Paint effect" by Cotterzz
// https://shadertoy.com/view/WX2SWz
// License: CC BY-NC-SA 3.0

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float xDiv;
uniform float yDiv;
uniform float curve;
uniform float gapSize;
uniform float diagSlant;
uniform float strokeSpread;

const float REFERENCE_HEIGHT = 400.0;

vec4 sampleCell(vec2 fragCoord) {
    vec2 uv = fragCoord / resolution.y;
    float aspect = resolution.x / resolution.y;
    float rx = floor(uv.x * xDiv);
    float rix = fract(uv.x * xDiv);
    float rixl = (curve / rix) - curve;
    float rixr = (curve / (1.0 - rix)) - curve;

    float ry = floor((uv.y + rx * diagSlant) * yDiv + rixl - rixr);
    float riy = fract((uv.y + rx * diagSlant) * yDiv + rixl - rixr) - 0.5;
    riy = riy * riy * riy;

    vec2 sampleUV = vec2(rx / (xDiv * aspect),
                         (riy * strokeSpread / REFERENCE_HEIGHT) + (ry / yDiv) - rx * diagSlant);
    vec4 c = textureLod(texture0, sampleUV, 0.0);

    if (abs(riy) > 0.125 * (1.0 - gapSize) || rix > (1.0 - gapSize) || rix < gapSize) {
        c.a = 0.0;
    }
    return c;
}

void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 offset = vec2(0.5);
    vec4 accum = vec4(0.0);
    for (int k = 0; k < 4; k++) {
        accum += sampleCell(fragCoord + offset - 0.5);
        offset = fract(offset + vec2(0.569840296, 0.754877669));
    }
    accum /= 4.0;
    finalColor = vec4(accum.rgb * accum.a, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| xDiv | float | 2.0 - 30.0 | 11.0 | yes | X Div |
| yDiv | float | 2.0 - 20.0 | 7.0 | yes | Y Div |
| diagSlant | float | 0.0 - 0.3 | 0.1 | yes | Diag Slant |
| curve | float | 0.001 - 0.1 | 0.01 | yes | Curve |
| gapSize | float | 0.01 - 0.4 | 0.1 | yes | Gap Size |
| strokeSpread | float | 50.0 - 500.0 | 200.0 | yes | Stroke Spread |

### UI Layout

Two subsections (6 params triggers sectioning):

```
SeparatorText("Geometry");
  X Div, Y Div, Diag Slant
SeparatorText("Strokes");
  Curve, Gap Size, Stroke Spread
```

All sliders use `ModulatableSlider`. Formats: `"%.0f"` for xDiv/yDiv/strokeSpread; `"%.3f"` for curve; `"%.2f"` for diagSlant/gapSize.

### Constants

- Enum: `TRANSFORM_DREKKER_PAINT` (append to `TransformEffectType` before `TRANSFORM_ACCUM_COMPOSITE`)
- Field name: `drekkerPaint` (camelCase)
- Display name: `"Drekker Paint"`
- Badge: `"ART"`
- Section index: `4` (Painterly)
- Flags: `EFFECT_FLAG_NONE`

### Pipeline Position

Painterly category, after existing Painterly effects. Order is not critical (user can reorder via drag-drop). New effect lands at end of `TransformOrderConfig::order` via default enum order.

---

## Tasks

### Wave 1: Effect Header

#### Task 1.1: Create effect header

**Files**: `src/effects/drekker_paint.h`
**Creates**: `DrekkerPaintConfig`, `DrekkerPaintEffect` types, public function declarations, `DREKKER_PAINT_CONFIG_FIELDS` macro.

**Do**: Create the header per the Types section. Follow `src/effects/kuwahara.h` structure exactly (struct layout, include guards, `#include "raylib.h"`, `#include <stdbool.h>`, one-line file header comment). Do not use emdashes or Unicode.

**Verify**: File exists. No build step (header only).

---

### Wave 2: Parallel Implementation

All Wave 2 tasks touch different files and run in parallel.

#### Task 2.1: Create effect module implementation

**Files**: `src/effects/drekker_paint.cpp`
**Depends on**: Wave 1 complete.

**Do**: Implement `DrekkerPaintEffectInit`, `DrekkerPaintEffectSetup`, `DrekkerPaintEffectUninit`, `DrekkerPaintRegisterParams`, static `SetupDrekkerPaint(PostEffect*)` bridge, static `DrawDrekkerPaintParams(EffectConfig*, const ModSources*, ImU32)`, and `REGISTER_EFFECT` macro at the bottom. Follow `src/effects/kuwahara.cpp` as the structural template.

Init: Load `shaders/drekker_paint.fs`, cache all 7 uniform locations (resolution, xDiv, yDiv, curve, gapSize, diagSlant, strokeSpread). Return false if shader id is 0.

Setup: Push `vec2 resolution = {GetScreenWidth(), GetScreenHeight()}` and all 6 float uniforms via `SetShaderValue`.

Uninit: `UnloadShader(e->shader)`.

RegisterParams: Register all 6 floats with the ranges from the Parameters table using `ModEngineRegisterParam("drekkerPaint.<field>", &cfg->field, min, max)`.

UI (`DrawDrekkerPaintParams`): Two `SeparatorText` subsections per the UI Layout section. All widgets are `ModulatableSlider` with the labels and formats specified. `glow` parameter is unused; `(void)glow;` to suppress warning.

Bridge (`SetupDrekkerPaint`): Delegate to `DrekkerPaintEffectSetup(&pe->drekkerPaint, &pe->effects.drekkerPaint)`.

Registration macro:
```cpp
// clang-format off
REGISTER_EFFECT(TRANSFORM_DREKKER_PAINT, DrekkerPaint, drekkerPaint,
                "Drekker Paint", "ART", 4, EFFECT_FLAG_NONE,
                SetupDrekkerPaint, NULL, DrawDrekkerPaintParams)
// clang-format on
```

Includes per conventions (alphabetized by clang-format within groups): own header; `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`; `"imgui.h"`, `"ui/modulatable_slider.h"`; `<stddef.h>`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Create fragment shader

**Files**: `shaders/drekker_paint.fs`
**Depends on**: None (parallel with other Wave 2 tasks).

**Do**: Create the shader file with the attribution block and full shader source exactly as specified in the Algorithm section. Use `#version 330`. Keep R2 constants `0.569840296, 0.754877669` verbatim. Keep `textureLod(texture0, sampleUV, 0.0)` (LOD 0 is required to avoid mipmap seams). Do not add tonemap. Do not re-center coordinates — grid math is translation-invariant and the reference's `fragCoord/resolution.y` normalization is preserved.

**Verify**: File exists. Shader loads at runtime (caught by Task 2.1's Init check).

#### Task 2.3: Register effect in effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete.

**Do**: Three edits:
1. Add `#include "effects/drekker_paint.h"` in the alphabetical include block (between `dot_matrix.h` and `dream_fractal.h`).
2. Add `TRANSFORM_DREKKER_PAINT` to the `TransformEffectType` enum, placed alphabetically among painterly enums or just before `TRANSFORM_ACCUM_COMPOSITE`. Placement does not affect UI order; only enum position matters for descriptor table indexing.
3. Add `DrekkerPaintConfig drekkerPaint;` member to `EffectConfig` struct in a sensible location near other Painterly effect configs.

Do not edit `TransformOrderConfig::order` — it initializes from enum values in default order.

**Verify**: Compiles.

#### Task 2.4: Add Effect member to PostEffect

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete.

**Do**: Two edits:
1. Add `#include "effects/drekker_paint.h"` in the alphabetical include block (between `dot_matrix.h` and `dream_fractal.h`).
2. Add `DrekkerPaintEffect drekkerPaint;` member inside the `PostEffect` struct near other Painterly effect members (after `KuwaharaEffect kuwahara;` is a reasonable spot).

No `post_effect.cpp` edits required — descriptor table drives lifecycle.

**Verify**: Compiles.

#### Task 2.5: Register effect for preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete.

**Do**: Three edits:
1. Add `#include "effects/drekker_paint.h"` in the alphabetical include block.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrekkerPaintConfig, DREKKER_PAINT_CONFIG_FIELDS)` in the "Effect configs A-G" or "O-Z" section (alphabetical location).
3. Add `X(drekkerPaint) \` entry to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Compiles. Saving a preset with the effect enabled and reloading preserves all 6 field values.

---

## Final Verification

- [ ] Build succeeds with no warnings (`cmake.exe --build build`)
- [ ] Effect appears in the Effects window under the Painterly category with the "ART" badge
- [ ] Enable checkbox adds the effect to the transform pipeline
- [ ] All 6 sliders modify the visual in real-time
- [ ] Cell grid visibly curves at boundaries (curve > 0), stroke shape sweeps with diagonal offset
- [ ] Gap regions render as black (not as underlying image or as solid color)
- [ ] Modulation routing panel shows all 6 `drekkerPaint.*` params as targets
- [ ] Preset save/load preserves all 6 fields
- [ ] No mipmap seams at cell boundaries (textureLod LOD 0 correctly applied)
- [ ] Stroke size looks consistent when window is resized (normalized by 400 reference height)

---

## Implementation Notes

Four issues surfaced during implementation that the original plan and research doc did not catch. Recorded here so the shader reads correctly alongside the plan's Algorithm section.

### 1. R2 AA offset lives in pixel space, not UV space

The reference's call structure is `mainImage(fragCoord) -> mainImage0(fragCoord + j - 0.5)`, where `fragCoord` is pixels and normalization (`/ iResolution.y`) happens inside `mainImage0`. The initial plan collapsed this into a single `sampleCell(uv)` taking pre-normalized uv, which silently moved the `+ j - 0.5` offset from ±0.5 pixels (subpixel AA jitter) to ±0.5 normalized uv units (half the screen). Each of the four "AA samples" pulled from completely different cells.

**Rule**: when the reference passes arguments across a function boundary that contains a normalization step, preserve that boundary in the transcription. Don't hoist normalization into the caller.

### 2. Horizontal wallpaper wrap on non-square viewports

Reference uses `uv = fragCoord / iResolution.y`, producing `uv.x ∈ [0, aspect]`. On 16:9, `rx = floor(uv.x * xDiv)` reaches ~19 (not 10), and `sampleUV.x = rx / xDiv` reaches ~1.73. Shadertoy's default REPEAT texture wrap masks this as an intended horizontal tile. In this pipeline it shows as obvious wallpaper repetition on the right side of the screen.

**Fix applied**: scale `sampleUV.x` by `aspect = resolution.x / resolution.y`, giving `sampleUV.x = rx / (xDiv * aspect)`. Keeps the reference's cell density and diagonal slant (19 cells on 16:9, full `rx * diagSlant` lean range) while mapping every horizontal cell to a unique source-texture position. Do NOT solve this by switching to `fragCoord / resolution` — that halves the cell count and kills the reference's diagonal character.

### 3. `REFERENCE_HEIGHT = 400`, not 1080

The reference's hardcoded `200.` stroke-spread constant was calibrated against Shadertoy preview height (~200-400 px). Normalizing to 1080 on a 1080p display shrinks the effective sweep to ~0.023 uv (16% of cell height), producing visually flat cells with no painterly sweep. Using `REFERENCE_HEIGHT = 400` gives ~0.0625 uv offset at default `strokeSpread = 200` — visible brushstroke sweep within each cell that matches the Shadertoy preview aesthetic.

### 4. `gapSize` uniform needs a direction-unified formula

Reference: `if (abs(riy) > 0.1 || rix > 0.9 || rix < 0.1) a = 0`. Substituting the single uniform `gapSize` into all three literals (as the research doc's substitution table prescribed) produces opposing semantics: decreasing `gapSize` increases the riy-condition cuts (horizontal wavy gaps) while decreasing the rix-condition cuts. No value of a unified uniform produces "no gap" or "reference match."

**Fix applied**: reformulate the riy side as `abs(riy) > 0.125 * (1.0 - gapSize)`. `0.125` is `riy_max = 0.5^3`. At `gapSize = 0`, threshold = 0.125 (never cuts) and rix condition never cuts → solid cells, zero gaps. At `gapSize = 0.1`, threshold = 0.1125 ≈ reference's 0.1 and rix condition matches reference → visually equivalent to the reference's hardcoded mask.

### Files touched beyond the plan's Task list

None. All four fixes live in `shaders/drekker_paint.fs` and the plan's Algorithm block.
