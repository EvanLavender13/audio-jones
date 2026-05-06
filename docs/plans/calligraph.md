# Calligraph

Two-pass generator with ping-pong mask feedback. State pass evolves an ink-density field by stamping a lissajous-traced curve into the field and advecting prior content with procedural curl-noise. Color pass renders the field via 4-tap edge detection on the mask, colored by gradient-LUT (sampled by mask intensity) and FFT brightness (sampled at the same per-pixel `t = mask`). Glow-on-black aesthetic.

**Research**: `docs/research/calligraph.md`

## Design

### Types

**`src/effects/calligraph.h`**:

```cpp
#ifndef CALLIGRAPH_EFFECT_H
#define CALLIGRAPH_EFFECT_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;
typedef struct ColorLUT ColorLUT;

typedef struct CalligraphConfig {
  bool enabled = false;

  // Curl noise field
  float morphSpeed = 0.25f;   // Noise field morph rate (0.0-2.0)

  // Feedback
  float decayRate = 1.0f;     // Mask fade per second (0.05-5.0)
  float spawnDistort = 0.05f; // Curl distortion of spawn position (0.0-0.2)
  float advectScale = 0.005f; // Curl advection of prior frame (0.0-0.02)

  // Spawn shape (lissajous polyline SDF)
  float lineThickness = 0.02f;  // Spawn-line thickness in centered NDC (0.005-0.1)
  int lissajousSamples = 64;    // Polyline samples for SDF (16-256)
  DualLissajousConfig lissajous = {
      .amplitude = 0.4f,
      .motionSpeed = 0.0f,
      .freqX1 = 1.0f, // Figure-8 default (1, 2)
      .freqY1 = 2.0f,
  };

  // Render
  float edgeFeather = 0.02f; // Edge-detection feather (0.005-0.1)

  // FFT (standard)
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Output (standard generator)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
} CalligraphConfig;

#define CALLIGRAPH_CONFIG_FIELDS                                                  \
  enabled, morphSpeed, decayRate, spawnDistort, advectScale, lineThickness,    \
      lissajousSamples, lissajous, edgeFeather, baseFreq, maxFreq, gain,       \
      curve, baseBright, gradient, blendMode, blendIntensity

typedef struct CalligraphEffect {
  Shader stateShader;          // mask field update shader (Pass A)
  Shader shader;               // color render shader (Pass Image), required name for macro
  ColorLUT *gradientLUT;
  RenderTexture2D maskPingPong[2]; // RGBA32F mask field (red channel used)
  RenderTexture2D colorRT;         // RGBA32F final colored output
  int readIdx;

  float morphPhase; // CPU-accumulated noise time

  // State shader uniform locations
  int stateResolutionLoc;
  int stateMorphPhaseLoc;
  int stateDeltaTimeLoc;
  int stateDecayRateLoc;
  int stateSpawnDistortLoc;
  int stateAdvectScaleLoc;
  int stateLineThicknessLoc;
  int stateLissajousSamplesLoc;
  int stateLissAmplitudeLoc;
  int stateLissPhaseLoc;
  int stateLissFreqX1Loc;
  int stateLissFreqY1Loc;
  int stateLissFreqX2Loc;
  int stateLissFreqY2Loc;
  int stateLissOffsetX2Loc;
  int stateLissOffsetY2Loc;

  // Color shader uniform locations
  int colorResolutionLoc;
  int colorEdgeFeatherLoc;
  int colorFftTextureLoc;
  int colorGradientLUTLoc;
  int colorSampleRateLoc;
  int colorBaseFreqLoc;
  int colorMaxFreqLoc;
  int colorGainLoc;
  int colorCurveLoc;
  int colorBaseBrightLoc;
} CalligraphEffect;

bool CalligraphEffectInit(CalligraphEffect *e, const CalligraphConfig *cfg, int width,
                       int height);
void CalligraphEffectSetup(CalligraphEffect *e, CalligraphConfig *cfg, float deltaTime,
                        const Texture2D &fftTexture);
void CalligraphEffectRender(CalligraphEffect *e, const CalligraphConfig *cfg,
                         int screenWidth, int screenHeight);
void CalligraphEffectResize(CalligraphEffect *e, int width, int height);
void CalligraphEffectUninit(CalligraphEffect *e);
void CalligraphRegisterParams(CalligraphConfig *cfg);
CalligraphEffect *GetCalligraphEffect(PostEffect *pe);

#endif // CALLIGRAPH_EFFECT_H
```

### Algorithm

Two GPU passes plus CPU phase accumulation. The state pass accumulates the mask field with ping-pong feedback. The color pass renders the field through edge detection + gradient LUT + FFT brightness.

**CPU per frame (in `CalligraphEffectSetup`):**

```cpp
e->morphPhase += cfg->morphSpeed * deltaTime;
cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime;
ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
// Bind all state-shader uniforms (resolution, morphPhase, deltaTime,
// decayRate, spawnDistort, advectScale, lineThickness, lissajousSamples,
// and all lissajous fields).
// Bind all color-shader uniforms (resolution, edgeFeather, fftTexture,
// gradientLUT, sampleRate, baseFreq, maxFreq, gain, curve, baseBright).
```

**`shaders/calligraph_state.fs` (Pass A — mask field update):**

```glsl
// Based on "Inkelly" by leon
// https://www.shadertoy.com/view/4fl3Wl
// License: CC BY-NC-SA 3.0 Unported
// Modified: continuous-morph time (replaces stepped delay), lissajous-curve
// spawn shape (replaces horizontal bar), parameterized decay/distortion/advection
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // previous mask field (auto-bound by DrawTextureRec)
uniform vec2 resolution;
uniform float morphPhase;         // CPU-accumulated noise time
uniform float deltaTime;
uniform float decayRate;
uniform float spawnDistort;
uniform float advectScale;
uniform float lineThickness;
uniform int lissajousSamples;

uniform float lissAmplitude;
uniform float lissPhase;
uniform float lissFreqX1;
uniform float lissFreqY1;
uniform float lissFreqX2;
uniform float lissFreqY2;
uniform float lissOffsetX2;
uniform float lissOffsetY2;

const float TWO_PI = 6.28318530718;

float gyroid(vec3 seed) { return dot(sin(seed), cos(seed.yzx)); }

float fbm(vec2 pos) {
    float t = morphPhase;
    float t2 = t * 1.354;
    vec3 p = vec3(pos, t);
    float result = 0.0;
    float a = 0.5;
    for (int i = 0; i < 3; ++i) {
        result += abs(gyroid(p / a) * a);
        a /= 2.0;
    }
    result = sin(result * 6.283 + t2 - pos.x);
    return result;
}

vec2 lissajous(float t) {
    float x = sin(lissFreqX1 * t + lissPhase);
    float y = cos(lissFreqY1 * t + lissPhase);
    float nx = 1.0;
    float ny = 1.0;
    if (lissFreqX2 != 0.0) {
        x += sin(lissFreqX2 * t + lissOffsetX2 + lissPhase);
        nx = 2.0;
    }
    if (lissFreqY2 != 0.0) {
        y += cos(lissFreqY2 * t + lissOffsetY2 + lissPhase);
        ny = 2.0;
    }
    return vec2((x / nx) * lissAmplitude, (y / ny) * lissAmplitude);
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 uv = fragTexCoord;
    // Centered, height-normalized (matches reference's (2*fragCoord - res)/res.y)
    vec2 p = (fragTexCoord * resolution * 2.0 - resolution) / resolution.y;

    // Curl noise: finite-difference of fbm scalar potential, 90-deg rotated
    vec2 e = vec2(4.0 / resolution.y, 0.0);
    vec2 curl = vec2(fbm(p + e.xy) - fbm(p - e.xy),
                     fbm(p + e.yx) - fbm(p - e.yx)) / (2.0 * e.x);
    curl = vec2(curl.y, -curl.x);

    // Distort spawn position by curl (literal 0.05 in reference)
    p += curl * spawnDistort;

    // SDF to closed lissajous polyline (sample N segments around 2pi)
    float minDist = 1e9;
    for (int i = 0; i < lissajousSamples; ++i) {
        float t0 = TWO_PI * float(i) / float(lissajousSamples);
        float t1 = TWO_PI * float(i + 1) / float(lissajousSamples);
        vec2 A = lissajous(t0);
        vec2 B = lissajous(t1);
        float d = sdSegment(p, A, B);
        minDist = min(minDist, d);
    }
    float spawnMask = smoothstep(lineThickness, 0.0, minDist);

    // Read previous mask, displaced by curl (literal 0.005 in reference)
    float prevMask = texture(texture0, uv + curl * advectScale).r;

    // Saturating accumulation: spawn over decayed prior
    float mask = max(spawnMask, prevMask - decayRate * deltaTime);

    finalColor = vec4(mask, 0.0, 0.0, 1.0);
}
```

**`shaders/calligraph_color.fs` (Pass Image — render):**

```glsl
// Based on "Inkelly" by leon
// https://www.shadertoy.com/view/4fl3Wl
// License: CC BY-NC-SA 3.0 Unported
// Modified: glow-on-black render replacing white-paper background; gradient LUT
// + FFT brightness (BAND_SAMPLES=4) replacing constant white line color
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // mask field (auto-bound)
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float sampleRate;
uniform float edgeFeather;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    vec2 uv = fragTexCoord;

    // 4-tap edge detection on the mask field (verbatim from reference)
    vec2 ep = 1.0 / resolution;
    float mr = smoothstep(0.0, edgeFeather, texture(texture0, uv).r);
    float mE = smoothstep(0.0, edgeFeather, texture(texture0, uv + vec2(ep.x, 0.0)).r);
    float mW = smoothstep(0.0, edgeFeather, texture(texture0, uv - vec2(ep.x, 0.0)).r);
    float mN = smoothstep(0.0, edgeFeather, texture(texture0, uv + vec2(0.0, ep.y)).r);
    float mS = smoothstep(0.0, edgeFeather, texture(texture0, uv - vec2(0.0, ep.y)).r);
    float edge = abs(mE - mr) + abs(mW - mr) + abs(mN - mr) + abs(mS - mr);
    edge = clamp(edge / 2.0, 0.0, 1.0);

    // Same per-pixel t drives gradient LUT and FFT lookup
    float t = texture(texture0, uv).r;
    vec3 lineColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) /
                   float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 result = lineColor * brightness * edge;
    finalColor = vec4(result, 1.0);
}
```

**Render flow (`CalligraphEffectRender`):**

```
writeIdx = 1 - readIdx;

// Pass 1: state update -> maskPingPong[writeIdx]
BeginTextureMode(maskPingPong[writeIdx])
  BeginShaderMode(stateShader)
    DrawFullscreenQuad(maskPingPong[readIdx].texture)  // texture0 = prev mask
  EndShaderMode
EndTextureMode

// Pass 2: color render -> colorRT
BeginTextureMode(colorRT)
  BeginShaderMode(shader)
    DrawFullscreenQuad(maskPingPong[writeIdx].texture)  // texture0 = current mask
  EndShaderMode
EndTextureMode

readIdx = writeIdx;
```

**Blend bridge (`SetupCalligraphBlend`):**

```cpp
BlendCompositorApply(pe->blendCompositor, e->colorRT.texture,
                     pe->effects.calligraph.blendIntensity,
                     pe->effects.calligraph.blendMode);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| morphSpeed | float | 0.0 - 2.0 | 0.25 | yes | Morph Speed |
| decayRate | float | 0.05 - 5.0 | 1.0 | yes | Decay |
| spawnDistort | float | 0.0 - 0.2 | 0.05 | yes | Spawn Distort |
| advectScale | float | 0.0 - 0.02 | 0.005 | yes | Advect |
| lineThickness | float | 0.005 - 0.1 | 0.02 | yes | Thickness |
| lissajousSamples | int | 16 - 256 | 64 | no | Samples |
| lissajous.amplitude | float | 0.05 - 2.0 | 0.4 | yes | Amplitude (DrawLissajousControls) |
| lissajous.motionSpeed | float | 0.0 - 5.0 | 0.0 | yes | Motion Speed (DrawLissajousControls) |
| lissajous.freqX1/Y1/X2/Y2 | float | 0 - 5 | 1, 2, 0, 0 | no | DrawLissajousControls |
| lissajous.offsetX2/Y2 | float | -PI - PI | 0.3, 2.09 | yes | DrawLissajousControls |
| edgeFeather | float | 0.005 - 0.1 | 0.02 | yes | Edge Feather |
| baseFreq | float | 27.5 - 440 | 55 | yes | Base Freq (Hz) |
| maxFreq | float | 1000 - 16000 | 14000 | yes | Max Freq (Hz) |
| gain | float | 0.1 - 10 | 2.0 | yes | Gain |
| curve | float | 0.1 - 3 | 1.5 | yes | Contrast |
| baseBright | float | 0.0 - 1.0 | 0.15 | yes | Base Bright |
| gradient | ColorConfig | - | GRADIENT mode | - | (STANDARD_GENERATOR_OUTPUT) |
| blendIntensity | float | 0.0 - 5.0 | 1.0 | yes | Blend Intensity |
| blendMode | EffectBlendMode | enum | SCREEN | - | Blend Mode |

### Constants

- Enum name: `TRANSFORM_CALLIGRAPH` (no `_BLEND` suffix; this effect uses `REGISTER_GENERATOR_FULL`)
- Display name: `"Calligraph"`
- Category badge: `"GEN"` (auto-set by macro)
- Section index: `12` (Texture)
- Macro: `REGISTER_GENERATOR_FULL` with custom render
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-set by macro)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create `src/effects/calligraph.h`

**Files**: `src/effects/calligraph.h`
**Creates**: `CalligraphConfig`, `CalligraphEffect`, `CALLIGRAPH_CONFIG_FIELDS` macro, public function declarations

**Do**: Implement the header per the Design > Types section. Match the layout pattern of `src/effects/curl_advection.h` (similar generator with ping-pong state). Include `config/dual_lissajous_config.h` for the embedded `DualLissajousConfig`. Forward-declare `PostEffect` and `ColorLUT`.

**Verify**: `cmake.exe --build build` compiles (the header alone, included by nothing yet, must be valid C++).

---

### Wave 2: Parallel implementation

#### Task 2.1: Create `src/effects/calligraph.cpp`

**Files**: `src/effects/calligraph.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Implement the lifecycle functions per the curl_advection.cpp pattern (similar two-pass generator with ping-pong state):
  - `CalligraphEffectInit`: `LoadShader("shaders/calligraph_state.fs")`, then `LoadShader("shaders/calligraph_color.fs")`, then cache locations, then init `gradientLUT`, then init the two ping-pong textures via `RenderUtilsInitTextureHDR` + the `colorRT` via the same. Cascade-cleanup on each failure (unload prior shaders/LUT/textures). Clear ping-pong textures with `RenderUtilsClearTexture`.
  - `CalligraphEffectSetup`: accumulate `e->morphPhase += cfg->morphSpeed * deltaTime`; accumulate `cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime`; call `ColorLUTUpdate`; bind ALL state-shader and color-shader uniforms listed in the Design section. The `fftTexture`, `sampleRate` (from `AUDIO_SAMPLE_RATE`), `baseFreq/maxFreq/gain/curve/baseBright`, and `gradientLUT` go to the color shader.
  - `CalligraphEffectRender`: implement the two-pass flow from the Design section. Use `RenderUtilsDrawFullscreenQuad` for both passes. State pass writes to `maskPingPong[writeIdx]` and reads from `maskPingPong[readIdx].texture`. Color pass writes to `colorRT` and reads from `maskPingPong[writeIdx].texture`. Flip `readIdx` at the end. The state pass output has alpha=1 from the shader so no `rlDisableColorBlend` workaround is needed.
  - `CalligraphEffectResize`: unload + re-init both ping-pong textures and `colorRT` at new dimensions; clear them; reset `readIdx = 0`.
  - `CalligraphEffectUninit`: unload both shaders, free LUT, unload all three render textures.
  - `CalligraphRegisterParams`: register every modulatable param listed in the Parameters table with the bounds shown.
  - `GetCalligraphEffect`: cast `pe->effectStates[TRANSFORM_CALLIGRAPH]`.
- Bridge functions (non-static, named to match the macro args):
  - `SetupCalligraph(PostEffect *pe)`: calls `CalligraphEffectSetup(e, &pe->effects.calligraph, GetFrameTime(), pe->fftTexture)`. This is the **scratch setup** (binds state+color uniforms each frame).
  - `SetupCalligraphBlend(PostEffect *pe)`: calls `BlendCompositorApply(pe->blendCompositor, e->colorRT.texture, cfg->blendIntensity, cfg->blendMode)`.
  - `RenderCalligraph(PostEffect *pe)`: calls `CalligraphEffectRender(e, cfg, pe->screenWidth, pe->screenHeight)`.
- Colocated UI (`// === UI ===` section, `static void DrawCalligraphParams(EffectConfig *e, const ModSources *ms, ImU32)`):
  - `ImGui::SeparatorText("Field")` — Morph Speed, Decay, Spawn Distort, Advect (modulatable sliders)
  - `ImGui::SeparatorText("Spawn")` — Thickness (modulatable), Samples (`ImGui::SliderInt`), then `DrawLissajousControls(&cfg->lissajous, "calligraph_liss", "calligraph.lissajous", ms, 5.0f)`
  - `ImGui::SeparatorText("Render")` — Edge Feather (modulatable)
  - `ImGui::SeparatorText("Audio")` — Base Freq, Max Freq, Gain, Contrast, Base Bright per the FFT Audio UI Conventions in `docs/conventions.md`
- Use `STANDARD_GENERATOR_OUTPUT(calligraph)` immediately above the registration macro.
- Registration macro:
  ```cpp
  // clang-format off
  REGISTER_GENERATOR_FULL(TRANSFORM_CALLIGRAPH, Calligraph, calligraph,
                          "Calligraph", SetupCalligraphBlend,
                          SetupCalligraph, RenderCalligraph, 12,
                          DrawCalligraphParams, DrawOutput_calligraph)
  // clang-format on
  ```
- Includes (alphabetized within groups, clang-format will sort): own header; `audio/audio.h` (for `AUDIO_SAMPLE_RATE`); `automation/mod_sources.h`, `automation/modulation_engine.h`; `config/constants.h`, `config/dual_lissajous_config.h`, `config/effect_config.h`, `config/effect_descriptor.h`; `imgui.h`; `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `render/render_utils.h`; `rlgl.h` (only if `rlDisableColorBlend` is needed; otherwise omit); `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`; `<math.h>`, `<stddef.h>`.

**Verify**: `cmake.exe --build build` compiles cleanly.

---

#### Task 2.2: Modify `src/config/effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/calligraph.h"` in the alphabetical include block (between `infinity_matrix.h` and `ink_wash.h`).
- Add `TRANSFORM_CALLIGRAPH,` to the `TransformEffectType` enum, immediately before `TRANSFORM_ACCUM_COMPOSITE` (which must remain second-to-last before `TRANSFORM_EFFECT_COUNT`).
- Add `CalligraphConfig calligraph;` member to `EffectConfig` struct in the alphabetical-by-feature block (place near `lichen` since they are sibling generators).

Do NOT add anything to `TransformOrderConfig::order` initialization manually — the constructor loops `i < TRANSFORM_EFFECT_COUNT` and assigns `(TransformEffectType)i`, so adding the enum value is sufficient. Do NOT add a member to `PostEffect` in `src/render/post_effect.h` — new effects use the slot-array pattern (`pe->effectStates[Type]`).

**Verify**: `cmake.exe --build build` compiles cleanly.

---

#### Task 2.3: Create `shaders/calligraph_state.fs`

**Files**: `shaders/calligraph_state.fs`
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section's `calligraph_state.fs` block verbatim. Begin with the attribution comment block per project convention:

```glsl
// Based on "Inkelly" by leon
// https://www.shadertoy.com/view/4fl3Wl
// License: CC BY-NC-SA 3.0 Unported
// Modified: continuous-morph time (replaces stepped delay), lissajous-curve
// spawn shape (replaces horizontal bar), parameterized decay/distortion/advection
```

GLSL 330. The `lissajous(float t)` function and `sdSegment` follow the arc_strobe.fs pattern. Use `for (int i = 0; i < lissajousSamples; ++i)` for the SDF loop directly per `memory/feedback_glsl_loops.md` — GLSL 330 supports dynamic loop bounds.

**Verify**: Effect renders without GLSL compile errors at runtime. (Build does not validate shaders; runtime verification only.)

---

#### Task 2.4: Create `shaders/calligraph_color.fs`

**Files**: `shaders/calligraph_color.fs`
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section's `calligraph_color.fs` block verbatim, with the same attribution comment header as Task 2.3 but `Modified: glow-on-black render replacing white-paper background; gradient LUT + FFT brightness (BAND_SAMPLES=4) replacing constant white line color`.

**Verify**: Effect renders without GLSL compile errors at runtime.

---

#### Task 2.5: Modify `src/config/effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/calligraph.h"` near the other effect-header includes at the top of the file.
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CalligraphConfig, CALLIGRAPH_CONFIG_FIELDS)` in the alphabetical block of effect-config serializers (near `LichenConfig`).
- Append `X(calligraph) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table at the bottom of the list (the entry will appear between existing entries; clang-format does not reorder this macro, so place near `lichen` for readability).

**Verify**: `cmake.exe --build build` compiles cleanly.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `Calligraph` appears in the Texture generator section of the UI with the GEN badge
- [ ] Enabling the effect adds it to the pipeline order
- [ ] Default lissajous (figure-8: freqX1=1, freqY1=2, amplitude=0.4) renders as expected when enabled
- [ ] `morphSpeed > 0` morphs the curl noise field continuously (no stepped freeze-and-jump)
- [ ] Increasing `lineThickness` thickens the inky lines; decreasing makes them faint or disappear
- [ ] `decayRate` slider visibly tunes how long ink trails persist
- [ ] Color follows the gradient LUT — fresh ink reads at one end of the gradient, decayed reads at the other
- [ ] Audio reactivity: per-pixel brightness modulates with FFT energy at the corresponding frequency band
- [ ] Lissajous frequency sliders reshape the seed curve in real time
- [ ] Preset save/load preserves all settings including the embedded `DualLissajousConfig` and `gradient`
- [ ] Modulation routes function for all params registered in `CalligraphRegisterParams`
- [ ] Window resize correctly reallocates the ping-pong + color textures (no crash, no stale resolution)

---

## Implementation Notes

- **Effect renamed Inkelly -> Calligraph.** All symbols, file names, enum, EffectConfig field, display name, and param prefixes carry the new name. Shader attribution headers retain "Inkelly" since that is the source artwork's name.
- **UI section order corrected.** The plan listed Field, Spawn, Render, Audio. The ui-guide skill mandates Audio first; reordered to Audio, Field, Spawn, Glow. The "Render" subsection was renamed to "Glow" to match the ui-guide vocabulary.
- **Color-shader sampler bindings moved into Render.** `SetShaderValueTexture` for `gradientLUT` and `fftTexture` initially lived in `BindColorUniforms` (called from Setup, outside any active shader). Sibling generators (`curl_advection.cpp`, `lichen.cpp`) bind sampler uniforms inside `BeginShaderMode` in their Render functions; binding outside the active shader produced a black colorRT. Moved both binds into the color pass's shader-mode block.
- **`Texture2D fftTexture` cached on `CalligraphEffect`.** Setup receives the FFT texture and stashes it on the effect struct; Render reads it back when binding inside the active color shader.
- **Default lissajous changed to (freqX1=1, freqY1=1).** The plan's (1, 2) default produces `y = 1 - 2x^2` — a parabola, not a curve readable as "lissajous". (1, 1) gives an ellipse/circle that visibly demonstrates the lissajous parameterisation.
