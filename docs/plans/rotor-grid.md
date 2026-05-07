# Rotor Grid

Generator effect that draws a spinning radial cell-grid with growing angular subdivisions per ring. Three coloring modes share one scaffold: SMOOTH paints every cell from the gradient LUT by angle (matching the reference's rainbow), WEDGE highlights a sweeping angular slice and dims the rest, RANDOM hashes each cell to a unique gradient position with optional drift. FFT reactivity is per-cell — the same `t` that picks the gradient hue picks the FFT band.

**Research**: `docs/research/rotor_grid.md`

## Design

### Types

`src/effects/rotor_grid.h`:

```cpp
#ifndef ROTOR_GRID_H
#define ROTOR_GRID_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

enum RotorGridMode {
  ROTOR_GRID_MODE_SMOOTH = 0,
  ROTOR_GRID_MODE_WEDGE = 1,
  ROTOR_GRID_MODE_RANDOM = 2,
};

struct RotorGridConfig {
  bool enabled = false;

  // Coloring mode
  int mode = ROTOR_GRID_MODE_SMOOTH;

  // FFT mapping
  float baseFreq = 55.0f;     // (27.5-440.0)
  float maxFreq = 14000.0f;   // (1000-16000)
  float gain = 2.0f;          // (0.1-10.0)
  float curve = 1.5f;         // (0.1-3.0)
  float baseBright = 0.15f;   // (0.0-1.0)

  // Geometry
  float ringSpacing = 0.1f;     // (0.05-0.5) radial scale; smaller = more rings
  int baseDivisions = 4;        // (1-8) base angular subdivisions
  float ringFrequency = 3.14f;  // (1.0-6.283) ring spacing in cos(ringFrequency*l)
  float radialDrift = 0.0f;     // (-PI..PI) ring phase shift for breathing

  // Animation
  float spinSpeed = 5.0f;          // (-ROTATION_SPEED_MAX..ROTATION_SPEED_MAX) rad/s
  float differentialTwist = 0.0f;  // (-2.0..2.0) outer-vs-inner rotation differential
  float driftRate = 0.0f;          // (0.0..1.0) RANDOM mode hash drift rate

  // Mode-specific
  float wedgeWidth = 0.4f;     // (0.0..PI) WEDGE mode angular half-width

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define ROTOR_GRID_CONFIG_FIELDS                                               \
  enabled, mode, baseFreq, maxFreq, gain, curve, baseBright, ringSpacing,      \
      baseDivisions, ringFrequency, radialDrift, spinSpeed, differentialTwist, \
      driftRate, wedgeWidth, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct RotorGridEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float spinPhase;   // CPU-accumulated rotation phase
  float driftPhase;  // CPU-accumulated random-mode drift phase

  int resolutionLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int modeLoc;
  int ringSpacingLoc;
  int baseDivisionsLoc;
  int ringFrequencyLoc;
  int radialDriftLoc;
  int spinPhaseLoc;
  int differentialTwistLoc;
  int driftPhaseLoc;
  int wedgeWidthLoc;
} RotorGridEffect;

bool RotorGridEffectInit(RotorGridEffect *e, const RotorGridConfig *cfg);
void RotorGridEffectSetup(RotorGridEffect *e, const RotorGridConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);
void RotorGridEffectUninit(RotorGridEffect *e);
void RotorGridRegisterParams(RotorGridConfig *cfg);
RotorGridEffect *GetRotorGridEffect(PostEffect *pe);

#endif // ROTOR_GRID_H
```

### Algorithm

Centered coordinates, radial length, angular index with growth-per-ring, cell field, AA — all preserved verbatim from the reference. Differential twist scales the spin phase per ring index. Per-cell `t` selects gradient color; same `t` drives FFT lookup using the standard `BAND_SAMPLES=4` averaging.

`shaders/rotor_grid.fs`:

```glsl
// Based on "rrotationnal rotation [207ch]" and "irrotationnal rotation [208ch]"
// by FabriceNeyret2
// https://www.shadertoy.com/view/sXf3WH
// https://www.shadertoy.com/view/sXX3WH
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized scaffold (ringSpacing, baseDivisions, ringFrequency,
//   radialDrift, differentialTwist), three coloring modes (SMOOTH/WEDGE/RANDOM)
//   feeding through gradient LUT, per-cell FFT brightness via BAND_SAMPLES=4
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform vec2 resolution;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int mode;
uniform float ringSpacing;
uniform int baseDivisions;
uniform float ringFrequency;
uniform float radialDrift;
uniform float spinPhase;
uniform float differentialTwist;
uniform float driftPhase;
uniform float wedgeWidth;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const int BAND_SAMPLES = 4;

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float computeFftMag(float t) {
    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) /
                   float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    return pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
}

void main() {
    vec2 fc = fragTexCoord * resolution;
    vec2 U = fc - resolution * 0.5;

    float l = length(U) / ringSpacing / resolution.y;
    float ringIdx = round(l);
    float ringSpin = spinPhase * (1.0 + differentialTwist * ringIdx);

    float a = atan(U.y, U.x) * float(baseDivisions) * ringIdx + ringSpin;
    float d = cos(a) * cos(ringFrequency * l + radialDrift);
    float aa = min(abs(d / fwidth(d)), 1.0);

    vec3 col;
    float brightness;

    if (mode == 0) {
        float t = fract(a / (TWO_PI * float(baseDivisions)));
        col = texture(gradientLUT, vec2(t, 0.5)).rgb;
        brightness = baseBright + computeFftMag(t);
    } else if (mode == 1) {
        bool inWedge = abs(mod(a / float(baseDivisions), TWO_PI) - PI) < wedgeWidth;
        if (inWedge) {
            float t = fract(a / (TWO_PI * float(baseDivisions)));
            col = texture(gradientLUT, vec2(t, 0.5)).rgb;
            brightness = baseBright + computeFftMag(t);
        } else {
            col = vec3(1.0);
            brightness = baseBright;
        }
    } else {
        float angularSlot = round(a / TWO_PI * float(baseDivisions) * ringIdx);
        float t = fract(hash2(vec2(ringIdx, angularSlot)) + driftPhase);
        col = texture(gradientLUT, vec2(t, 0.5)).rgb;
        brightness = baseBright + computeFftMag(t);
    }

    finalColor = vec4(col * brightness * aa, 1.0);
}
```

CPU-side phase accumulation in `RotorGridEffectSetup()`:
```
e->spinPhase += cfg->spinSpeed * deltaTime;
e->driftPhase += cfg->driftRate * deltaTime;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| mode | int (enum) | 0..2 | 0 (SMOOTH) | No | Mode (Combo: Smooth/Wedge/Random) |
| baseFreq | float | 27.5..440 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000..16000 | 14000 | Yes | Max Freq (Hz) |
| gain | float | 0.1..10 | 2.0 | Yes | Gain |
| curve | float | 0.1..3 | 1.5 | Yes | Contrast |
| baseBright | float | 0..1 | 0.15 | Yes | Base Bright |
| ringSpacing | float | 0.05..0.5 | 0.1 | Yes | Ring Spacing |
| baseDivisions | int | 1..8 | 4 | No | Base Divisions |
| ringFrequency | float | 1.0..6.283 | 3.14 | Yes | Ring Frequency |
| radialDrift | float (rad) | -ROTATION_OFFSET_MAX..ROTATION_OFFSET_MAX | 0.0 | Yes | Radial Drift |
| spinSpeed | float (rad/s) | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 5.0 | Yes | Spin Speed |
| differentialTwist | float | -2.0..2.0 | 0.0 | Yes | Differential Twist |
| driftRate | float | 0.0..1.0 | 0.0 | Yes | Drift Rate |
| wedgeWidth | float (rad) | 0.0..PI | 0.4 | Yes | Wedge Width |
| blendIntensity | float | 0.0..5.0 | 1.0 | Yes | (output panel) |
| gradient | ColorConfig | — | gradient mode | No | (output panel) |
| blendMode | EffectBlendMode | — | EFFECT_BLEND_SCREEN | No | (output panel) |

### Constants

- Enum: `TRANSFORM_ROTOR_GRID_BLEND`
- Display name: `"Rotor Grid"`
- Field name: `rotorGrid`
- Modulation prefix: `"rotorGrid."`
- Section index: `10` (Geometric)
- Badge: `"GEN"` (auto via `REGISTER_GENERATOR`)
- Flags: `EFFECT_FLAG_BLEND` (auto via `REGISTER_GENERATOR`)

---

## Tasks

### Wave 1: Foundation (parallel)

#### Task 1.1: Create header

**Files**: `src/effects/rotor_grid.h`
**Creates**: `RotorGridConfig`, `RotorGridEffect`, `RotorGridMode` enum, `ROTOR_GRID_CONFIG_FIELDS` macro, public function declarations

**Do**: Create new header. Use the type definitions from the Design > Types section verbatim. Layout follows `src/effects/spectral_arcs.h` (config struct first with bool enabled, grouped fields, ColorConfig at end, blend fields last; effect struct with shader, gradientLUT pointer, accumulated phases, uniform locations; public function decls).

**Verify**: `cmake.exe --build build` — compiles.

#### Task 1.2: Create shader

**Files**: `shaders/rotor_grid.fs`
**Creates**: Fragment shader implementing the mode-dispatched radial cell-grid

**Do**: Create new file. Begin with the attribution comment block from the Algorithm section (cites both Shadertoy URLs and CC BY-NC-SA license). Then `#version 330`, varying/uniform declarations, `hash2` helper, `computeFftMag` helper, `main()` body — all verbatim from the Algorithm section.

**Verify**: Build succeeds. Shader compilation is checked at runtime when the effect is enabled.

---

### Wave 2: Implementation and Integration (parallel)

#### Task 2.1: Create source file with registration

**Files**: `src/effects/rotor_grid.cpp`
**Depends on**: Wave 1 (Task 1.1)

**Do**: Create new file. Follow `src/effects/spectral_arcs.cpp` for structure end-to-end:
- Includes: own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.
- `RotorGridEffectInit()`: load shader, fail with cascade unload if `shader.id == 0`; cache all uniform locations; init `gradientLUT` via `ColorLUTInit(&cfg->gradient)` and unload shader on fail; zero `spinPhase` and `driftPhase`; return true.
- `RotorGridEffectSetup()`: accumulate `e->spinPhase += cfg->spinSpeed * deltaTime` and `e->driftPhase += cfg->driftRate * deltaTime`. Call `ColorLUTUpdate()`. Bind every uniform in the Effect struct (resolution, fftTexture, gradientLUT, sampleRate from `AUDIO_SAMPLE_RATE`, all config floats, mode as int, baseDivisions as int, both phases).
- `RotorGridEffectUninit()`: `UnloadShader`; `ColorLUTUninit`.
- `RotorGridRegisterParams()`: register every modulatable param in the parameters table. Use `ROTATION_SPEED_MAX` for spinSpeed, `ROTATION_OFFSET_MAX` for radialDrift, `0.0f, 5.0f` for blendIntensity. Do NOT register `mode` or `baseDivisions` (non-modulatable).
- `GetRotorGridEffect(PostEffect *pe)`: return `(RotorGridEffect*)pe->effectStates[TRANSFORM_ROTOR_GRID_BLEND]`.
- `SetupRotorGrid(PostEffect *pe)`: bridge to `RotorGridEffectSetup` (non-static).
- `SetupRotorGridBlend(PostEffect *pe)`: call `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.rotorGrid.blendIntensity, pe->effects.rotorGrid.blendMode)` (non-static).
- `// === UI ===` section with `static void DrawRotorGridParams(EffectConfig*, const ModSources*, ImU32)`. Sections in this order: Audio (`SeparatorText("Audio")` then Base Freq, Max Freq, Gain, Contrast, Base Bright via `ModulatableSlider` with format strings per FFT convention), Mode (`SeparatorText("Mode")` then `ImGui::Combo` with `"Smooth\0Wedge\0Random\0"`), Geometry (`SeparatorText("Geometry")` then Ring Spacing, Base Divisions via `ImGui::SliderInt` 1-8, Ring Frequency, Radial Drift via `ModulatableSliderAngleDeg`), Mode-Specific (`SeparatorText("Mode Options")` then Wedge Width via `ModulatableSlider` and Drift Rate via `ModulatableSlider`), Animation (`SeparatorText("Animation")` then Spin Speed via `ModulatableSliderSpeedDeg`, Differential Twist via `ModulatableSlider`).
- Bottom of file: `STANDARD_GENERATOR_OUTPUT(rotorGrid)` then `REGISTER_GENERATOR(TRANSFORM_ROTOR_GRID_BLEND, RotorGrid, rotorGrid, "Rotor Grid", SetupRotorGridBlend, SetupRotorGrid, 10, DrawRotorGridParams, DrawOutput_rotorGrid)` wrapped in `// clang-format off` / `// clang-format on`.

**Verify**: Compiles. Effect appears under the Geometric generator section labeled "Rotor Grid" with badge "GEN". Mode combo switches the visual between three styles. Modulation routes resolve for every param in the registry.

#### Task 2.2: Register config in EffectConfig

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 (Task 1.1)

**Do**:
- Add `#include "effects/rotor_grid.h"` in alphabetical position with other effect headers (after `risograph.h`, before `scan_bars.h`).
- Add `TRANSFORM_ROTOR_GRID_BLEND,` to the `TransformEffectType` enum just before `TRANSFORM_ACCUM_COMPOSITE`.
- Add `RotorGridConfig rotorGrid;` member to the `EffectConfig` struct, placed near other generators (e.g., after `frameRecursion`).
- Do NOT touch `TransformOrderConfig::order` — the constructor populates it automatically by iterating the enum.

**Verify**: Compiles.

#### Task 2.3: Register serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 (Task 1.1)

**Do**:
- Add `#include "effects/rotor_grid.h"` in alphabetical position with other effect includes.
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RotorGridConfig, ROTOR_GRID_CONFIG_FIELDS)` in the alphabetical block (R section, after `RisographConfig`).
- Add `X(rotorGrid)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro list at the end (near other recently added generators).

**Verify**: Compiles. Save preset with rotor grid enabled and customized values, restart, load preset; values restore correctly.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Geometric generator section labeled "Rotor Grid" with badge "GEN"
- [ ] Mode combo switches between Smooth, Wedge, and Random — each renders a distinct visual character
- [ ] Smooth mode shows continuous rainbow wrapping around each ring at the reference's color rate (cycles = ringIdx per revolution)
- [ ] Wedge mode shows a narrow gradient-colored angular sweep against a dim grayscale field
- [ ] Random mode shows per-cell hashed gradient colors; with driftRate = 0 the mosaic is static, with driftRate > 0 cells cycle through gradient
- [ ] Differential twist nonzero produces ring-shearing rotation (inner vs outer differential)
- [ ] Radial drift nonzero shifts the ring breathing phase
- [ ] FFT reactivity visible per cell (each cell brightens with its band's energy)
- [ ] Modulation routing works on every registered parameter
- [ ] Preset save/load round-trips all settings including mode
- [ ] No tonemap, no global summed-FFT brightness, no magic scalars

---

## Implementation Notes

The shipped effect diverges from the plan above. Changes made during implementation:

### Three modes collapsed into Density + Randomness sliders

The plan specified a `mode` enum (SMOOTH / WEDGE / RANDOM) with mode-specific params. In play-testing, this read as three separate effects sharing geometry rather than a unified control surface, and "Wedge" was opaque as a name. Replaced with two continuous sliders:

- **Density** (0..1, modulatable): fraction of cells lit. Each cell hashes to a value, lit if `cellHash < density`. `density=1` lights all cells (old SMOOTH/RANDOM behavior); `density<1` produces sparse highlights against a dim field (old WEDGE behavior).
- **Randomness** (0..1, modulatable): blends ordered position-based `t` with per-cell hash `t`. `randomness=0` = ordered rainbow flowing around the ring (old SMOOTH); `randomness=1` = per-cell random color (old RANDOM); intermediate values mix.

Dropped: `mode` field, `wedgeWidth` field, `RotorGridMode` enum.

### Per-cell discrete `t` (not continuous)

The plan's per-mode `t` formulas were continuous in `a` (`t = fract(a / (TWO_PI * baseDivisions))`), which meant `t` varied within a single cell — gradient bled across the cell instead of one color per cell. Fixed by computing discrete cell IDs:

- `radIdx = round(ringFrequency * l / PI)` — radial cell index aligned with `cos(ringFrequency*l)` lobes
- `angIdx = mod(round(a / PI), cellsPerRing)` — angular cell index, wrapped mod `cellsPerRing = 2 * baseDivisions * ringIdx`
- `t_ordered = fract(angIdx / (2 * baseDivisions))` — discrete per-cell, matches reference's `cos(a/baseDivisions)` color cycle rate (one rainbow per `ringIdx` revolutions per ring)
- `t_random = fract(cellHash + driftPhase)` — discrete per-cell hash with optional drift

The `mod(angIdx, cellsPerRing)` wrap fixes a discontinuity bug at the negative x-axis where `atan(U.y, U.x)` jumps from `+PI` to `-PI`. Without the wrap, cells straddling the negative x-axis would split into two halves with different `angIdx` values, hence different hashes — visible as cells fading in/out as if a static mask were rotating through the field.

### Ring frequency now redistributes both radial and angular cells coherently

The plan computed `ringIdx = round(l)` (independent of `ringFrequency`), which decoupled the radial cell boundaries from the angular subdivision count. Changing `ringFrequency` slid the radial cell lines but left the per-ring angular subdivisions unchanged. Fixed by deriving `ringIdx` from `radIdx` so both scale together.

### `radialDrift` removed

The plan included `radialDrift` to shift the radial cell phase for "ring breathing" modulation. With the per-cell-ID system, modulating it reshuffled cell IDs (hence colors and lit states) instead of breathing — undesirable. Dropped entirely.

### Smooth differential twist via separate accumulator

The plan formula `ringSpin = spinPhase * (1 + differentialTwist * ringIdx)` retroactively scaled the already-accumulated `spinPhase` whenever `differentialTwist` changed, producing a visual snap. Replaced with a dedicated CPU-accumulated phase:

- `twistPhase += spinSpeed * differentialTwist * deltaTime`
- `ringSpin = spinPhase + twistPhase * ringIdx`

Slider changes only affect future accumulation. When `differentialTwist` returns to zero, `twistPhase` freezes (rather than snapping back) — kept as a feature: the accumulated offset persists like physical inertia, and the user can drift it back by setting twist negative.

### `spinSpeed` default

Plan default was `5.0` rad/s, which exceeds `ROTATION_SPEED_MAX = PI`. Changed to `1.0` (matches sibling generators).

### Final shipped parameters

| Parameter | Range | Default | Modulatable |
|-----------|-------|---------|-------------|
| baseFreq | 27.5..440 | 55.0 | Yes |
| maxFreq | 1000..16000 | 14000 | Yes |
| gain | 0.1..10 | 2.0 | Yes |
| curve | 0.1..3 | 1.5 | Yes |
| baseBright | 0..1 | 0.15 | Yes |
| density | 0..1 | 1.0 | Yes |
| randomness | 0..1 | 0.0 | Yes |
| ringSpacing | 0.05..0.5 | 0.1 | Yes |
| baseDivisions | 1..8 | 4 | No |
| ringFrequency | 1.0..6.283 | 3.14 | Yes |
| spinSpeed | -PI..PI | 1.0 | Yes |
| differentialTwist | -2.0..2.0 | 0.0 | Yes |
| driftRate | 0..1 | 0.0 | Yes |
| blendIntensity | 0..5 | 1.0 | Yes |
