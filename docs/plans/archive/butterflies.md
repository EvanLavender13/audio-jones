# Butterflies

A scrolling field of procedurally unique butterflies tiled in a brick pattern. Each butterfly has Fourier-curve wing edges, voronoi detail texturing, and per-tile gradient LUT coloring. Wings flap continuously, patterns periodically mutate, and each butterfly's brightness reacts to its own FFT frequency band.

**Research**: `docs/research/butterflies.md`

## Design

### Types

`ButterfliesConfig` (`src/effects/butterflies.h`):

```cpp
struct ButterfliesConfig {
  bool enabled = false;

  // Audio
  float baseFreq = 55.0f;    // Lowest mapped pitch Hz (27.5-440)
  float maxFreq = 14000.0f;  // Highest mapped pitch Hz (1000-16000)
  float gain = 2.0f;         // FFT magnitude amplification (0.1-10)
  float curve = 1.5f;        // Contrast exponent (0.1-3)
  float baseBright = 0.15f;  // Floor brightness (0-1)

  // Geometry
  float zoom = 1.5f;          // Tile-space scale (0.5-5.0)
  float patternDetail = 1.0f; // Voronoi density multiplier (0.1-3.0)
  float wingShape = 1.0f;     // Wing curvature exponent (0.2-3.0)

  // Animation
  float scrollSpeed = 0.3f; // Vertical scroll rate tiles/s (-2.0-2.0)
  float driftSpeed = 0.0f;  // Horizontal drift rate tiles/s (-1.0-1.0)
  float flapSpeed = 0.05f;  // Wing flap frequency cycles/s (0.0-1.0)
  float shiftSpeed = 0.5f;  // Pattern mutation rate steps/s (0.1-4.0)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

`BUTTERFLIES_CONFIG_FIELDS` macro lists all serializable fields:
`enabled, baseFreq, maxFreq, gain, curve, baseBright, zoom, patternDetail, wingShape, scrollSpeed, driftSpeed, flapSpeed, shiftSpeed, gradient, blendMode, blendIntensity`

`ButterfliesEffect` (`src/effects/butterflies.h`):

```cpp
typedef struct ColorLUT ColorLUT;

typedef struct ButterfliesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  // CPU-accumulated phases (avoid jumps when speed slider moves)
  float scrollPhase;
  float driftPhase;
  float flapPhase;
  float shiftPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int scrollPhaseLoc;
  int driftPhaseLoc;
  int flapPhaseLoc;
  int shiftPhaseLoc;
  int flapSpeedLoc;
  int zoomLoc;
  int patternDetailLoc;
  int wingShapeLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ButterfliesEffect;
```

Function declarations follow the marble.h pattern:
```cpp
bool ButterfliesEffectInit(ButterfliesEffect *e, const ButterfliesConfig *cfg);
void ButterfliesEffectSetup(ButterfliesEffect *e, const ButterfliesConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);
void ButterfliesEffectUninit(ButterfliesEffect *e);
void ButterfliesRegisterParams(ButterfliesConfig *cfg);
```

### Algorithm

The shader is "The Butterfly Effect" by Martijn Steinrucken (Shadertoy `XsVGRV`) with the substitutions below applied verbatim. Reference code is in `docs/research/butterflies.md`.

**Drop entirely from reference**: `iMouse`, `songLength`, `loopNum`, `camT`, `camDive`, `smooth90()`, `BlockWave()`, `B()`, `dist2()`, fade-in/out smoothsteps, `SHAPESHIFT` global toggle.

**Substitutions**:

| Reference | Replacement |
|-----------|------------|
| `iTime` | (no longer needed; phases replace all uses) |
| `iResolution` | `uniform vec2 resolution` |
| `SHAPESHIFT` global | constant `1.0` (always on) |
| `p = vec2(p.x*1.777, mix(p.y, smooth90(p.y+.5, .7)-.5, camDive))` | `p.x *= resolution.x / resolution.y;` (no barrel distortion) |
| `p *= mix(.85, 5., camDive)` | `p *= zoom` |
| `p.y -= 8.*(camT+sin(camT+PI))/TWOPI` | `p.y -= scrollPhase` |
| (none; new) | `p.x += driftPhase` |
| `floor(t/8.+.5)*floor(t/2.+.5)*SHAPESHIFT + loopNum` | `floor(shiftPhase)` |
| `pow(S01(t + hash2(id.xy).x*20., 10., .05), 60.)*camDive` | `pow(sin((flapPhase + (hash2(id.xy).x*20. + 10.) * flapSpeed) * TWOPI) * .5 + .5, 60.)` (drop `*camDive`; `flapPhase` accumulates `flapSpeed*dt` so phase stays continuous when slider moves, but per-butterfly offset still scales with current `flapSpeed` to match the reference's frequency-dependent spread) |
| `mix(.5, 3., misc.x)` (Wing curvature exponent) | `wingShape` |
| `40.*detail.z` (voronoi density inside Wing) | `40.*detail.z*patternDetail` |
| `vec3 edgeCol = hash4(colId.x+t).rgb; vec3 mainCol = hash4(colId.y+t).rgb;` and surrounding `colId`/inner `t` block in Wing | `vec3 mainCol = texture(gradientLUT, vec2(t_base, 0.5)).rgb; vec3 edgeCol = texture(gradientLUT, vec2(fract(t_base + 0.3), 0.5)).rgb;` |
| `col *= smoothstep(0., 3., t) * smoothstep(268., 250., t)` (final fade) | `col.rgb *= brightness` (per-butterfly FFT, formula below) |

**Wing() signature change**: insert `float t_base` as the third parameter. New signature:
`vec4 Wing(vec2 st, vec2 id, float t_base, float radius, vec2 center, vec4 misc, vec4 offs, vec4 amp, vec4 pattern1, vec4 global, vec4 detail)`

**Keep verbatim from reference** (only `iResolution` swapped to `resolution` where it appears): `S01`, `S`, `saturate` macros; `hash`, `hash2`, `hash4`, `voronoi`, `skewcirc`, `curve`, `body`; the body of `Wing()` apart from the four substitutions above; brick tiling `p.y += floor(p.x+.5)*.5+.05`; `vec2 id = floor(p+.5); p = fract(p+.5)-.5;`; wing mirror `p.x = abs(p.x)`; `p.x -= .01*n1.x`; polar conversion `vec2 st = vec2(atan(p.x, p.y), length(p)); st.x /= PI;`; `if(st.x<.6) top = Wing(...); if(st.x>.4) bottom = Wing(...);` overlap dispatch; `wings = mix(bottom, top, top.a)`; `wings.rgb *= (1. - wingFlap*0.9)`; `col = mix(bodyCol*bodyCol.a, wings, wings.a)`; the per-tile seed cascade `it = hash2(id+shifter).x*10.+.25; pattern1 = hash4(it+.345); n1 = hash4(it); n2 = hash4(it+.3); n3 = hash4(n1.x); global = hash4(it*12.); detail = hash4(it*-12.); nBody = hash4(it*.1425);`.

**Shader header** (uniforms, includes, macros):

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float scrollPhase;
uniform float driftPhase;
uniform float flapPhase;
uniform float shiftPhase;
uniform float flapSpeed;
uniform float zoom;
uniform float patternDetail;
uniform float wingShape;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define PI 3.141592653589793238
#define TWOPI 6.283185307179586
#define S01(x, offset, frequency) (sin((x+offset)*frequency*TWOPI)*.5+.5)
#define S(x, offset, frequency) sin((x+offset)*frequency*TWOPI)
#define saturate(x) clamp(x,0.,1.)
```

**main() body** (replaces reference `mainImage`):

```glsl
void main() {
    vec2 p = gl_FragCoord.xy / resolution.xy;
    p -= 0.5;
    p.x *= resolution.x / resolution.y;
    p *= zoom;

    p.y += floor(p.x + 0.5) * 0.5 + 0.05;
    p.y -= scrollPhase;
    p.x += driftPhase;

    vec2 id = floor(p + 0.5);
    p = fract(p + 0.5) - 0.5;
    p.x = abs(p.x);

    float shifter = floor(shiftPhase);
    float t_base = hash2(id + shifter).x;
    float it = t_base * 10.0 + 0.25;

    vec4 pattern1 = hash4(it + 0.345);
    vec4 n1 = hash4(it);
    vec4 n2 = hash4(it + 0.3);
    vec4 n3 = hash4(n1.x);
    vec4 global = hash4(it * 12.);
    vec4 detail = hash4(it * -12.);
    vec4 nBody = hash4(it * 0.1425);

    p.x -= 0.01 * n1.x;

    vec4 col = vec4(1.);
    vec4 bodyCol = body(p, nBody);

    float wingFlap = pow(sin((flapPhase + (hash2(id.xy).x * 20.0 + 10.0) * flapSpeed) * TWOPI) * 0.5 + 0.5, 60.0);
    p.x *= mix(1., 20., wingFlap);

    vec2 st = vec2(atan(p.x, p.y), length(p));
    st.x /= PI;

    vec4 top = vec4(0.);
    if (st.x < 0.6) top = Wing(st, id, t_base, 0.5, vec2(0.25, 0.4), n1, n2, n3, pattern1, global, detail);
    vec4 bottom = vec4(0.);
    if (st.x > 0.4) bottom = Wing(st, id, t_base, 0.4, vec2(0.5, 0.75), n2, n3, n1, pattern1, global, detail);

    wingFlap = (1. - wingFlap * 0.9);
    vec4 wings = mix(bottom, top, top.a);
    wings.rgb *= wingFlap;

    col = mix(bodyCol * bodyCol.a, wings, wings.a);

    float freq = baseFreq * pow(maxFreq / baseFreq, t_base);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) energy = texture(fftTexture, vec2(bin, 0.5)).r;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;
    col.rgb *= brightness;

    finalColor = col;
}
```

**Wing() body modifications** (only the four substituted regions):

Replace the reference block

```glsl
vec2 colId = hash2(id);
colId.x *= mix( 1., floor(cos(iTime*.125)+.5)+.00015, SHAPESHIFT);
colId.y *= mix(1., floor(cos(iTime*.25)+.5)+.0001, SHAPESHIFT);
```

with deletion (these lines existed only to feed the random colors that the gradient LUT now replaces).

Replace `f = pow(f, mix(.5, 3., misc.x));` with `f = pow(f, wingShape);`.

Replace `vec2 vor = voronoi(vec2(st.x, st.y*.1)*40.*detail.z);` with `vec2 vor = voronoi(vec2(st.x, st.y*.1)*40.*detail.z*patternDetail);`.

Replace

```glsl
float t = floor(iTime*2.)*SHAPESHIFT;
vec3 edgeCol = hash4(colId.x+t).rgb;
vec3 mainCol = hash4(colId.y+t).rgb;
```

with

```glsl
vec3 mainCol = texture(gradientLUT, vec2(t_base, 0.5)).rgb;
vec3 edgeCol = texture(gradientLUT, vec2(fract(t_base + 0.3), 0.5)).rgb;
```

(the `vec3 detailCol = cross(edgeCol, mainCol);` line stays.)

All other lines in `Wing()` stay verbatim from the reference, including the `pow(misc, vec4(10.))` reweighting, the `r -= misc.y*curve(...)` adjustment, the `edgeBand` computation, the `clockValue`/`distValue` overlays, and the final `r = st.y - r; ... return vec4(col, o.x);` block.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440 | 55 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000 | yes | Max Freq (Hz) |
| gain | float | 0.1-10 | 2.0 | yes | Gain |
| curve | float | 0.1-3 | 1.5 | yes | Contrast |
| baseBright | float | 0-1 | 0.15 | yes | Base Bright |
| zoom | float | 0.5-5.0 | 1.5 | yes | Zoom |
| patternDetail | float | 0.1-3.0 | 1.0 | yes | Pattern Detail |
| wingShape | float | 0.2-3.0 | 1.0 | yes | Wing Shape |
| scrollSpeed | float | -2.0-2.0 | 0.3 | yes | Scroll Speed |
| driftSpeed | float | -1.0-1.0 | 0.0 | yes | Drift Speed |
| flapSpeed | float | 0.0-1.0 | 0.05 | yes | Flap Rate |
| shiftSpeed | float | 0.1-4.0 | 0.5 | yes | Shift Rate |
| blendIntensity | float | 0-5 | 1.0 | yes | (`STANDARD_GENERATOR_OUTPUT`) |
| blendMode | enum | EffectBlendMode | EFFECT_BLEND_SCREEN | no | (`STANDARD_GENERATOR_OUTPUT`) |
| gradient | ColorConfig | - | gradient mode | no | (`STANDARD_GENERATOR_OUTPUT`) |

### Constants

- Enum entry: `TRANSFORM_BUTTERFLIES_BLEND` (added immediately before `TRANSFORM_ACCUM_COMPOSITE`)
- Display name: `"Butterflies"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `13` (Field)

### UI Layout

`DrawButterfliesParams` uses `SeparatorText` dividers with this section order:

1. **Audio** - baseFreq, maxFreq, gain, curve (label "Contrast"), baseBright. Use `ModulatableSlider` for all five with the formats from the conventions (`%.1f`, `%.0f`, `%.1f`, `%.2f`, `%.2f`).
2. **Geometry** - zoom (`%.2f`), patternDetail (`%.2f`), wingShape (`%.2f`).
3. **Animation** - scrollSpeed (`ModulatableSlider`, `%.2f`), driftSpeed (`ModulatableSlider`, `%.2f`), flapSpeed (`ModulatableSliderLog`, `%.3f`; small range benefits from log scaling), shiftSpeed (`ModulatableSlider`, `%.2f`).

`scrollSpeed` and `driftSpeed` are tile-space translations, not angles, so use plain `ModulatableSlider` (not `ModulatableSliderSpeedDeg`).

Append `STANDARD_GENERATOR_OUTPUT(butterflies)` immediately before the `REGISTER_GENERATOR` macro for the Color/Output section.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create butterflies.h

**Files**: `src/effects/butterflies.h`
**Creates**: `ButterfliesConfig`, `ButterfliesEffect`, `BUTTERFLIES_CONFIG_FIELDS`, function declarations.

**Do**: Mirror `src/effects/marble.h`. Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`. Define `ButterfliesConfig` and `ButterfliesEffect` per the Design > Types section. Define the `BUTTERFLIES_CONFIG_FIELDS` macro with the field list above. Declare `ButterfliesEffectInit`, `ButterfliesEffectSetup`, `ButterfliesEffectUninit`, `ButterfliesRegisterParams` with the same signatures as the marble equivalents.

**Verify**: Header compiles when included alone.

---

### Wave 2: Implementation (parallel - no file overlap)

#### Task 2.1: Create butterflies.cpp

**Files**: `src/effects/butterflies.cpp`
**Depends on**: Wave 1.

**Do**: Mirror `src/effects/marble.cpp`. Includes (alphabetical, clang-format will sort): `"butterflies.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `<stddef.h>`.

`ButterfliesEffectInit`: load `shaders/butterflies.fs`; cache every uniform location from the Design > Types `ButterfliesEffect` struct; allocate `gradientLUT` via `ColorLUTInit(&cfg->gradient)` with cascade-cleanup (unload shader if LUT alloc fails); zero all four phases. Return `false` on any failure.

`ButterfliesEffectSetup`: accumulate `e->scrollPhase += cfg->scrollSpeed * deltaTime`, `e->driftPhase += cfg->driftSpeed * deltaTime`, `e->flapPhase += cfg->flapSpeed * deltaTime`, `e->shiftPhase += cfg->shiftSpeed * deltaTime`. Call `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`. Bind uniforms: `resolution` (vec2 from `GetScreenWidth/Height`), `fftTexture`, `sampleRate` (`(float)AUDIO_SAMPLE_RATE`), `scrollPhase`, `driftPhase`, `flapPhase`, `shiftPhase`, `flapSpeed` (also bound as a uniform - the wingFlap formula uses it for the per-butterfly offset spread), `zoom`, `patternDetail`, `wingShape`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `gradientLUT` (via `ColorLUTGetTexture`).

`ButterfliesEffectUninit`: `UnloadShader(e->shader); ColorLUTUninit(e->gradientLUT);`.

`ButterfliesRegisterParams`: register all 13 modulatable params under prefix `"butterflies."` with the ranges from the Design > Parameters table. Include `blendIntensity` with range `0.0f` to `5.0f`.

`static void DrawButterfliesParams(EffectConfig *e, const ModSources *ms, ImU32 categoryGlow)`: implement per the Design > UI Layout section.

`void SetupButterflies(PostEffect *pe)`: bridge - `ButterfliesEffectSetup(&pe->butterflies, &pe->effects.butterflies, pe->currentDeltaTime, pe->fftTexture);`

`void SetupButterfliesBlend(PostEffect *pe)`: bridge - `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.butterflies.blendIntensity, pe->effects.butterflies.blendMode);`

End the file with:

```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(butterflies)
REGISTER_GENERATOR(TRANSFORM_BUTTERFLIES_BLEND, Butterflies, butterflies, "Butterflies",
                   SetupButterfliesBlend, SetupButterflies, 13, DrawButterfliesParams, DrawOutput_butterflies)
// clang-format on
```

**Verify**: file written; final build is the gating check.

---

#### Task 2.2: Create butterflies.fs

**Files**: `shaders/butterflies.fs`
**Depends on**: Wave 1.

**Do**: Implement the Algorithm section verbatim. Begin the file with the attribution block:

```glsl
// Based on "The Butterfly Effect" by Martijn Steinrucken aka BigWings
// https://www.shadertoy.com/view/XsVGRV
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, gradient LUT coloring, per-butterfly FFT
// brightness, removed camera dive and song-timed fades.
```

Then the shader header from Design > Algorithm > "Shader header".

Then, in this order:
1. `hash`, `hash2`, `hash4` - copy verbatim from `docs/research/butterflies.md` reference code.
2. `voronoi` - copy verbatim.
3. `skewcirc` - copy verbatim.
4. `curve` - copy verbatim.
5. `body` - copy verbatim.
6. `Wing` - copy from reference, then apply the four substitutions in Design > Algorithm > "Wing() body modifications". Add `float t_base` as the third parameter.
7. `main()` - exactly the body from Design > Algorithm > "main() body".

Drop reference functions `B`, `dist2`, `smooth90`, `BlockWave` - all unused after the camera dive is removed.

**Verify**: file written; build gates this.

---

#### Task 2.3: Wire into effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1.

**Do**:
1. Add `#include "effects/butterflies.h"` in the alphabetical include block at the top of the file (clang-format will sort).
2. Add `TRANSFORM_BUTTERFLIES_BLEND,` enum entry immediately before `TRANSFORM_ACCUM_COMPOSITE,` in `TransformEffectType`.
3. Add `ButterfliesConfig butterflies;` member to `EffectConfig` near the other generators (next to `MarbleConfig marble;` is fine; clang-format does not sort struct members so place it intentionally).

`TransformOrderConfig::order` does **not** need editing - the constructor auto-fills via `(TransformEffectType)i`.

**Verify**: file written; build gates this.

---

#### Task 2.4: Wire into post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1.

**Do**:
1. Add `#include "effects/butterflies.h"` in the alphabetical include block.
2. Add `ButterfliesEffect butterflies;` member to the `PostEffect` struct near the other generator effect members (next to `MarbleEffect marble;`).

No `post_effect.cpp` changes - the descriptor system handles init/uninit/registerParams via the `REGISTER_GENERATOR` macro.

**Verify**: file written; build gates this.

---

#### Task 2.5: Wire into effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1.

**Do**:
1. Add `#include "effects/butterflies.h"` in the alphabetical include block.
2. Add JSON registration after the `MarbleConfig` entry (around line 444):
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ButterfliesConfig,
                                                   BUTTERFLIES_CONFIG_FIELDS)
   ```
3. Add `X(butterflies) \` entry in the `EFFECT_CONFIG_FIELDS(X)` X-macro table, alongside the other generators (a sensible spot is on the same line as `X(marble) \`).

**Verify**: file written; build gates this.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] `Butterflies` appears under the Scatter category (section 15) in the effects panel with the `GEN` badge
- [ ] Enabling the effect produces a scrolling brick-tiled butterfly field
- [ ] Each butterfly's wing colors come from the gradient LUT (offset 0.0 vs offset 0.3 between main and edge)
- [ ] Wings flap continuously; `flapSpeed` controls rate without phase jumps when slider is dragged
- [ ] Per-butterfly brightness reacts to its assigned FFT frequency band
- [ ] Vertical scroll responds to `scrollSpeed`, horizontal drift to `driftSpeed`
- [ ] `shiftSpeed` controls how often patterns mutate (shapeshift)
- [ ] `spread`, `patternDetail`, `wingShape` modify the field as described
- [ ] Modulation routes work for all 13 registered params
- [ ] Preset save/load preserves all settings including gradient and blend mode

---

## Implementation Notes

Post-implementation deltas from the plan above. Future readers should treat the source of truth as the actual code; these notes document why it diverged.

- **GLSL `curve()` -> `fourierCurve()`**: the reference shader's Fourier helper `curve()` collided with the `curve` audio-contrast uniform under GLSL 330 name resolution (`'curve' : function name is redeclaration of existing name`). Renamed the function and all five call sites inside `Wing()` to `fourierCurve`. The uniform retains the name `curve` per the FFT audio convention.
- **Final output forced to opaque alpha**: `finalColor = vec4(col)` was leaving alpha=0 in empty space (no body, no wings), and the SCREEN blend in the compositor preserves the destination when source rgb is 0, which produced persistent trails through the feedback path. Changed to `finalColor = vec4(col.rgb, 1.0)` so empty pixels are solid black and overwrite the destination cleanly. Matches the pattern used by `glyph_field.fs` and `galaxy.fs`.
- **Section index 13 -> 15**: relocated from the Field category to Scatter, which is a better fit for a tile-based sparse generator. Only the literal section index in the `REGISTER_GENERATOR` macro changed.
- **`zoom` parameter renamed to `spread`, formula reverted, range 0.2-5.0, default 1.0**: the original spec used `p *= zoom` (linear, but higher value made butterflies smaller). A first revision flipped to `p /= zoom` for "higher = bigger" semantics but introduced reciprocal non-linearity in slider feel. The final form uses `p *= spread` (linear in slider), and the parameter is named `spread` to make the direction (higher = more, smaller butterflies) read naturally. Field renamed in the header config struct, the `BUTTERFLIES_CONFIG_FIELDS` list, the uniform loc field, the shader uniform, the registration ID `butterflies.spread`, and the UI label "Spread".
- **`ui/ui_units.h` include**: added to `butterflies.cpp` per the conventions' standard generator include set, even though no angular/log slider is ultimately used. Kept for consistency with sibling generators.
