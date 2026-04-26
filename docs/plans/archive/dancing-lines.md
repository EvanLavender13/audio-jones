# Dancing Lines

A single line segment with Lissajous-driven endpoints snapping to a new position at a configurable rate, leaving a trailing stack of past positions. Each echo holds the line's pose from one tick ago, dimming with age, with per-echo color drawn from a gradient LUT and per-echo brightness driven by an FFT band. The newest echo glows brightest at the lowest band; older echoes fade backward through the gradient and up through the spectrum, like a strobed afterimage of the Qix arcade game.

**Research**: `docs/research/dancing_lines.md`

## Design

### Types

`src/effects/dancing_lines.h`:

```cpp
struct DancingLinesConfig {
  bool enabled = false;

  // Lissajous motion (shared dual-harmonic config)
  // motionSpeed default 0 preserves the original pure-snap character;
  // crank to add continuous figure drift on top of the snap.
  DualLissajousConfig lissajous = {
      .amplitude = 0.5f,
      .motionSpeed = 0.0f,
      .freqX1 = 2.0f,
      .freqY1 = 3.0f,
      .freqX2 = 0.0f,
      .freqY2 = 0.0f,
      .offsetX2 = 0.0f,
      .offsetY2 = 0.0f,
  };

  // Snap clock and trail
  float snapRate = 15.0f;        // Hz at which line snaps (1-60)
  int trailLength = 10;          // Number of echoes (1-32)
  float trailFade = 0.95f;       // Per-echo brightness multiplier (0.5-0.99)
  float colorPhaseStep = 0.3f;   // t-step per echo (0.05-1.0)

  // Geometry
  float lineThickness = 0.002f;  // Inner smoothstep edge in R.y units (0.0005-0.02)
  float endpointOffset = 1.5708f; // Parametric offset between the two endpoints (0.1-PI)

  // Glow
  float baseBright = 0.15f;      // Always-on per-echo floor (0-1)
  float glowIntensity = 1.0f;    // Output multiplier (0-3)

  // FFT mapping
  float baseFreq = 55.0f;        // Newest echo band (27.5-440)
  float maxFreq = 14000.0f;      // Oldest echo band (1000-16000)
  float gain = 2.0f;             // FFT magnitude scalar (0.1-10)
  float curve = 1.5f;            // FFT magnitude exponent (0.1-3)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DANCING_LINES_CONFIG_FIELDS                                            \
  enabled, lissajous, snapRate, trailLength, trailFade, colorPhaseStep,        \
      lineThickness, endpointOffset, baseBright, glowIntensity, baseFreq,      \
      maxFreq, gain, curve, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DancingLinesEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float accumTime; // CPU-accumulated wall clock for snap
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int phaseLoc;
  int amplitudeLoc;
  int freqX1Loc;
  int freqY1Loc;
  int freqX2Loc;
  int freqY2Loc;
  int offsetX2Loc;
  int offsetY2Loc;
  int accumTimeLoc;
  int snapRateLoc;
  int trailLengthLoc;
  int trailFadeLoc;
  int colorPhaseStepLoc;
  int lineThicknessLoc;
  int endpointOffsetLoc;
  int baseBrightLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int gradientLUTLoc;
} DancingLinesEffect;

bool DancingLinesEffectInit(DancingLinesEffect *e, const DancingLinesConfig *cfg);
void DancingLinesEffectSetup(DancingLinesEffect *e, DancingLinesConfig *cfg,
                             float deltaTime, const Texture2D &fftTexture);
void DancingLinesEffectUninit(DancingLinesEffect *e);
void DancingLinesRegisterParams(DancingLinesConfig *cfg);
```

### Algorithm

CPU-side per-frame setup (`DancingLinesEffectSetup`):

1. Advance lissajous phase: `cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime`.
2. Accumulate snap clock: `e->accumTime += deltaTime`. Wrap above 1000.0f to prevent float precision loss.
3. `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`.
4. Bind all uniforms (resolution, fftTexture, sampleRate, lissajous fields, accumTime, snapRate, trailLength, trailFade, colorPhaseStep, lineThickness, endpointOffset, baseBright, glowIntensity, baseFreq, maxFreq, gain, curve, gradientLUT). `trailLength` clamped to >= 1 before binding as int.

Shader (`shaders/dancing_lines.fs`) — full body:

```glsl
// Based on "Dancing lines (Qix)" by FabriceNeyret2
// https://www.shadertoy.com/view/ffSSRt
// License: CC BY-NC-SA 3.0 Unported
// Modified: snap clock + trail length as uniforms, dual-Lissajous endpoints
// from shared config, gradient LUT coloring, per-echo FFT band brightness

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform float phase;

uniform float amplitude;
uniform float freqX1;
uniform float freqY1;
uniform float freqX2;
uniform float freqY2;
uniform float offsetX2;
uniform float offsetY2;

uniform float accumTime;
uniform float snapRate;
uniform int trailLength;
uniform float trailFade;
uniform float colorPhaseStep;

uniform float lineThickness;
uniform float endpointOffset;
uniform float baseBright;
uniform float glowIntensity;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;

const float GLOW_WIDTH = 0.001; // AA outer edge offset above lineThickness (R.y units)

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

vec2 lissajous(float t) {
    float x = sin(freqX1 * t + phase);
    float y = cos(freqY1 * t + phase);

    float nx = 1.0;
    float ny = 1.0;

    if (freqX2 != 0.0) {
        x += sin(freqX2 * t + offsetX2 + phase);
        nx = 2.0;
    }
    if (freqY2 != 0.0) {
        y += cos(freqY2 * t + offsetY2 + phase);
        ny = 2.0;
    }

    float aspect = resolution.x / resolution.y;
    return vec2((x / nx) * amplitude * aspect, (y / ny) * amplitude);
}

void main() {
    vec2 uv = (fragTexCoord * resolution * 2.0 - resolution) / min(resolution.x, resolution.y);

    vec3 result = vec3(0.0);
    float fadeAccum = 1.0;
    float snapTick = floor(accumTime * snapRate);
    float fTrail = float(trailLength);

    for (int i = 0; i < trailLength; i++) {
        float fi = float(i);

        // Snap parameter: index-and-time multiplier baked into the lissajous arg
        float t = colorPhaseStep * (fi + snapTick);

        vec2 P = lissajous(t);
        vec2 Q = lissajous(t + endpointOffset);

        // Smoothstep AA: fixed 1px offset above lineThickness
        float d = sdSegment(uv, P, Q);
        float seg = smoothstep(lineThickness + GLOW_WIDTH, lineThickness, d);

        // FFT band-averaging - newest echo (i=0) maps to baseFreq, oldest to maxFreq
        float t0 = fi / fTrail;
        float t1 = (fi + 1.0) / fTrail;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Color from gradient LUT by normalized echo index
        vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;

        float brightness = baseBright + mag;
        result += color * seg * brightness * fadeAccum;

        // Newest echo (i=0) gets fadeAccum=1.0; subsequent echoes attenuate.
        fadeAccum *= trailFade;
    }

    result *= glowIntensity;
    finalColor = vec4(result, 1.0);
}
```

Notes:
- Snap clock is computed entirely on GPU via `floor(accumTime * snapRate)`. CPU only accumulates `accumTime`. This keeps all echoes on the same tick boundary while indexing different positions on the trail.
- `fadeAccum` is initialized to 1.0 and multiplied by `trailFade` AFTER each iteration's contribution is added. This inverts the original's `O *= 0.95` loop (which fades before adding) to make the newest echo (i=0) brightest.
- Color and FFT band index share `t0 = i / trailLength`. Color cycles through the gradient by echo index, decoupled from the snap clock.
- `lissajous()` is lifted verbatim from arc_strobe.fs. The continuous `phase` term (CPU-accumulated from `motionSpeed`) adds figure drift on top of the snap; with default `motionSpeed=0` the figure is pure-snap.
- `endpointOffset` plays the same role as arc_strobe's `orbitOffset`: how far apart the two endpoints sit on the parametric curve.
- No tonemap - per project rule. `glowIntensity` and `baseBright` shape brightness.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `snapRate` | float | 1-60 | 15 | Yes | `Snap Rate (Hz)` |
| `trailLength` | int | 1-32 | 10 | No (SliderInt) | `Trail Length` |
| `trailFade` | float | 0.5-0.99 | 0.95 | Yes | `Trail Fade` |
| `colorPhaseStep` | float | 0.05-1.0 | 0.3 | Yes | `Color Phase Step` |
| `lineThickness` | float | 0.0005-0.02 | 0.002 | Yes | `Line Thickness` |
| `endpointOffset` | float | 0.1-PI | 1.5708 | Yes | `Endpoint Offset` |
| `baseBright` | float | 0-1 | 0.15 | Yes | `Base Bright` |
| `glowIntensity` | float | 0-3 | 1.0 | Yes | `Glow Intensity` |
| `baseFreq` | float | 27.5-440 | 55 | Yes | `Base Freq (Hz)` |
| `maxFreq` | float | 1000-16000 | 14000 | Yes | `Max Freq (Hz)` |
| `gain` | float | 0.1-10 | 2.0 | Yes | `Gain` |
| `curve` | float | 0.1-3 | 1.5 | Yes | `Contrast` |
| `lissajous.amplitude` | float | 0.05-2.0 | 0.5 | Yes (via DrawLissajousControls) | `Amplitude` |
| `lissajous.motionSpeed` | float | 0-5 | 0.0 | Yes (via DrawLissajousControls) | `Motion Speed` |
| `lissajous.offsetX2` | float | -PI-PI | 0.0 | Yes (via DrawLissajousControls) | `Offset X2` |
| `lissajous.offsetY2` | float | -PI-PI | 0.0 | Yes (via DrawLissajousControls) | `Offset Y2` |
| `blendIntensity` | float | 0-5 | 1.0 | Yes (via STANDARD_GENERATOR_OUTPUT) | `Blend Intensity` |

All four `lissajous.*` modulatable params (amplitude, motionSpeed, offsetX2, offsetY2) MUST be registered because `DrawLissajousControls` references them - per project rule on shared widgets.

### UI Layout

Colocated `DrawDancingLinesParams(EffectConfig*, const ModSources*, ImU32)` sections in order:

1. **Audio** (`SeparatorText`): Base Freq, Max Freq, Gain, Contrast, Base Bright (standard FFT order)
2. **Trail** (`SeparatorText`): Snap Rate, Trail Length (SliderInt), Trail Fade, Color Phase Step
3. **Geometry** (`SeparatorText`): Line Thickness, Endpoint Offset
4. **Lissajous** (`SeparatorText`): `DrawLissajousControls(&cfg->lissajous, "dancinglines", "dancingLines.lissajous", modSources, 10.0f)`
5. **Glow** (`SeparatorText`): Glow Intensity

Output section auto-generated by `STANDARD_GENERATOR_OUTPUT(dancingLines)`.

### Constants

- Enum name: `TRANSFORM_DANCING_LINES_BLEND`
- Display name: `"Dancing Lines"`
- Category badge: `"GEN"` (auto-baked by REGISTER_GENERATOR)
- Category section index: `11` (Filament)
- Field name: `dancingLines`
- Param prefix: `"dancingLines."`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create header

**Files**: `src/effects/dancing_lines.h`
**Creates**: `DancingLinesConfig` struct, `DancingLinesEffect` struct, `DANCING_LINES_CONFIG_FIELDS` macro, lifecycle function declarations

**Do**: Build the header from the Design > Types section. Include guards `DANCING_LINES_EFFECT_H`. Includes: `"config/dual_lissajous_config.h"`, `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Top-of-file one-line comment naming the effect and visual technique. Inline range comments next to each field as in `arc_strobe.h`. Forward-declare `typedef struct ColorLUT ColorLUT;` before `DancingLinesEffect`.

**Verify**: `cmake.exe --build build` compiles (header-only addition, no consumers yet).

---

### Wave 2: Implementation, Shader, and Integration

All Wave 2 tasks operate on different files and can run in parallel. Each depends on Wave 1 (the header).

#### Task 2.1: Create implementation

**Files**: `src/effects/dancing_lines.cpp`
**Depends on**: Wave 1 complete

**Do**: Implement the five lifecycle functions (`Init`, `Setup`, `Uninit`, `RegisterParams`) plus the colocated UI section, the bridge functions, the `STANDARD_GENERATOR_OUTPUT(dancingLines)` macro, and the `REGISTER_GENERATOR` macro. Mirror the structure of `arc_strobe.cpp` exactly:

- `DancingLinesEffectInit`: load `shaders/dancing_lines.fs`, cache all uniform locations from the Types section, call `ColorLUTInit(&cfg->gradient)`, on LUT failure unload shader and return false. Initialize `accumTime = 0.0f`.
- `DancingLinesEffectSetup`: per Algorithm CPU-side steps. Wrap `accumTime` above 1000.0f. Clamp `trailLength` to `>= 1` before binding as int.
- `DancingLinesEffectUninit`: unload shader, free LUT.
- `DancingLinesRegisterParams`: register every param from the Parameters table marked Modulatable. The four `lissajous.*` params use bounds: `amplitude` 0.05-2.0, `motionSpeed` 0.0-5.0, `offsetX2`/`offsetY2` -ROTATION_OFFSET_MAX to ROTATION_OFFSET_MAX. `endpointOffset` uses 0.1f to PI_F. Other ranges per the Parameters table.
- `SetupDancingLines(PostEffect *pe)`: bridge calling `DancingLinesEffectSetup(&pe->dancingLines, &pe->effects.dancingLines, pe->currentDeltaTime, pe->fftTexture)`. NON-static.
- `SetupDancingLinesBlend(PostEffect *pe)`: bridge calling `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.dancingLines.blendIntensity, pe->effects.dancingLines.blendMode)`. NON-static.
- `// === UI ===` section with `static void DrawDancingLinesParams(...)` per the UI Layout section. Uses `ModulatableSlider` for all modulatable floats; `ImGui::SliderInt` for `trailLength`; `DrawLissajousControls` for the lissajous group.
- `STANDARD_GENERATOR_OUTPUT(dancingLines)` immediately before the registration macro.
- `REGISTER_GENERATOR(TRANSFORM_DANCING_LINES_BLEND, DancingLines, dancingLines, "Dancing Lines", SetupDancingLinesBlend, SetupDancingLines, 11, DrawDancingLinesParams, DrawOutput_dancingLines)` wrapped in `// clang-format off` / `// clang-format on`.

Includes (group membership; clang-format will sort): own header, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

**Verify**: Compiles. Effect appears in Filament category in UI; toggle adds it to pipeline; sliders update live.

#### Task 2.2: Create shader

**Files**: `shaders/dancing_lines.fs`
**Depends on**: Wave 1 complete (uniform names declared in header)

**Do**: Implement the shader exactly as written in the Algorithm > Shader section. Attribution comment block above `#version 330`. Do NOT add tanh, Reinhard, or any tonemap.

**Verify**: Shader loads on startup (no `LoadShader` failure). Visual: with `motionSpeed=0`, default `snapRate=15`, the segment snaps 15 times/sec leaving a 10-echo trail. Audio energy at 55Hz brightens the newest echo; high-band energy lights older echoes.

#### Task 2.3: Register enum, include, member, order

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/dancing_lines.h"` alphabetically among the existing effect includes.
- Add `TRANSFORM_DANCING_LINES_BLEND,` to the `TransformEffectType` enum, placed at the end before `TRANSFORM_ACCUM_COMPOSITE` (matching the position of `TRANSFORM_BUTTERFLIES_BLEND`, the most recent generator addition).
- Add `DancingLinesConfig dancingLines;` to `EffectConfig`, after the last existing generator field (next to `ButterfliesConfig butterflies`).
- The `TransformOrderConfig` initializer iterates `0..TRANSFORM_EFFECT_COUNT`, so no separate order array edit is needed - the new enum value is included automatically.

**Verify**: Compiles. `EffectConfig` size grows by `sizeof(DancingLinesConfig)`.

#### Task 2.4: Register PostEffect member

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/dancing_lines.h"` alphabetically among the existing effect includes.
- Add `DancingLinesEffect dancingLines;` to the `PostEffect` struct, alongside the other effect runtime members (next to `ButterfliesEffect butterflies`).

**Verify**: Compiles.

#### Task 2.5: Register JSON serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/dancing_lines.h"` alphabetically among the existing effect includes.
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DancingLinesConfig, DANCING_LINES_CONFIG_FIELDS)` alphabetically among the existing per-config macros.
- Add `X(dancingLines)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro list.

**Verify**: Compiles. Save preset with effect enabled; reload; settings preserved.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] "Dancing Lines" appears in Filament generator category with `GEN` badge
- [ ] Enabling adds it to the pipeline; disabling removes
- [ ] All sliders update visual in real-time
- [ ] At default settings: line snaps 15 times/sec with 10-echo trail
- [ ] FFT bass content brightens newest echo; treble content brightens oldest
- [ ] `motionSpeed=0` gives pure-snap behavior; `motionSpeed>0` adds continuous figure drift
- [ ] `freqX2=0, freqY2=0` (default) renders single-frequency endpoints; setting either to nonzero enables dual-harmonic with offsets visible
- [ ] Modulating any registered param via mod bus produces expected response
- [ ] Save/load preset preserves all fields
