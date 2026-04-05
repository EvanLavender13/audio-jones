# Color Stretch

Endlessly zooming fractal texture generator that recursively subdivides space into a grid, accumulating gradient color and FFT brightness at each depth level. The viewer sees a self-similar tunnel of shifting colored tiles, each tile containing a smaller copy of the whole pattern, zooming smoothly inward. Grid size and focus offset control the subdivision geometry, curvature warps the zoom into a dome, and spin rotates the whole tunnel.

**Research**: `docs/research/color_stretch.md`

## Design

### Types

**ColorStretchConfig** (`src/effects/color_stretch.h`):

```
bool enabled = false;

float zoomSpeed = 0.5f;       // Zoom rate (-2.0 to 2.0)
float zoomScale = 0.1f;       // Overall zoom level (0.01-1.0)
int glyphSize = 3;             // Grid subdivision size (2-8)
int recursionCount = 6;        // Fractal recursion depth (2-12)
float curvature = 0.0f;        // Dome/tunnel time warp (0.0-2.0)
float spinSpeed = 0.0f;        // UV rotation rate rad/s (-ROTATION_SPEED_MAX to ROTATION_SPEED_MAX)
DualLissajousConfig lissajous = {.amplitude = 0.3f, .freqX1 = 0.07f, .freqY1 = 0.05f};

float baseFreq = 55.0f;        // FFT low frequency (27.5-440)
float maxFreq = 14000.0f;      // FFT high frequency (1000-16000)
float gain = 2.0f;             // FFT energy multiplier (0.1-10)
float curve = 1.5f;            // FFT contrast exponent (0.1-3)
float baseBright = 0.15f;      // Minimum brightness floor (0-1)

ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
float blendIntensity = 1.0f;
```

**ColorStretchEffect** (`src/effects/color_stretch.h`):

```
Shader shader;
ColorLUT *gradientLUT;
float zoomPhase;    // Accumulated zoom: zoomPhase += zoomSpeed * dt
float spinPhase;    // Accumulated spin: spinPhase += spinSpeed * dt
int resolutionLoc;
int fftTextureLoc;
int sampleRateLoc;
int zoomPhaseLoc;
int zoomSpeedLoc;   // Needed in shader for curvature scaling
int zoomScaleLoc;
int glyphSizeLoc;
int recursionCountLoc;
int curvatureLoc;
int spinPhaseLoc;
int focusOffsetLoc; // vec2
int baseFreqLoc;
int maxFreqLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
int gradientLUTLoc;
```

### Algorithm

Attribution block at top of shader file (before `#version`):

```glsl
// Based on "Color Stretch" by KilledByAPixel (Frank Force)
// https://www.shadertoy.com/view/4lXcD7
// License: CC BY-NC-SA 3.0 Unported
// Modified: uniforms for grid/recursion/focus, gradient LUT replaces HSV, FFT audio, spin
```

Full shader body:

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform float zoomPhase;
uniform float zoomSpeed;
uniform float zoomScale;
uniform int glyphSize;
uniform int recursionCount;
uniform float curvature;
uniform float spinPhase;
uniform vec2 focusOffset;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float E = 2.718281828459;

void main() {
    // Center UV, normalize by height (from reference mainImage)
    vec2 uv = fragTexCoord * resolution - resolution * 0.5;
    uv /= resolution.y;

    // Spin: 2x2 rotation by CPU-accumulated spinPhase (replaces no-op InitUV)
    float c = cos(spinPhase);
    float s = sin(spinPhase);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

    // Per-pixel time with curvature warp (reference: time = iTime + curvature*pow(length(uv), 0.2))
    // zoomPhase accumulated on CPU; curvature offset scaled by zoomSpeed for rate consistency
    float timePercent = zoomPhase + curvature * pow(length(uv), 0.2) * zoomSpeed;
    int iterations = int(floor(timePercent));
    timePercent -= float(iterations);

    // Zoom math (kept verbatim: pow(e, -glyphSizeLog * timePercent) * zoomScale)
    float glyphSizeF = float(glyphSize);
    float glyphSizeLog = log(glyphSizeF);
    float zoom = pow(E, -glyphSizeLog * timePercent) * zoomScale;

    // Focus position: center cell + clamped offset (research: clamp to [0, glyphSize-1])
    ivec2 focus = clamp(ivec2(glyphSize / 2) + ivec2(round(focusOffset)),
                        ivec2(0), ivec2(glyphSize - 1));
    vec2 focusF = vec2(focus);

    // Offset convergence loop (kept verbatim, 13 iterations sufficient for glyphSize up to 8)
    vec2 offset = vec2(0.0);
    float gsfi = 1.0 / glyphSizeF;
    for (int i = 0; i < 13; ++i) {
        offset += focusF * gsfi * pow(gsfi, float(i));
    }

    // Apply zoom & offset (kept verbatim)
    vec2 pos = uv * zoom + offset;

    // Fractal recursion loop (core loop structure from GetPixelFractal)
    vec3 color = vec3(0.0);

    for (int r = 0; r <= recursionCount + 1; ++r) {
        // Shared index t for gradient LUT and FFT (research: t = r / recursionCount)
        float t = float(r) / float(recursionCount);

        // FFT frequency lookup (standard generator pattern)
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Gradient LUT color (replaces HSV accumulation)
        vec3 layerColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // GetRecursionFade logic (kept verbatim)
        float fade;
        if (r > recursionCount) {
            fade = timePercent;
        } else {
            float rt = max(float(r) - timePercent, 0.0);
            fade = rt / float(recursionCount);
        }

        color += layerColor * brightness * fade;

        if (r > recursionCount) { break; }

        // Subdivide space (core loop: multiply pos by glyphSize, subtract floor)
        pos *= glyphSizeF;
        pos -= floor(pos);
    }

    finalColor = vec4(color, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| zoomSpeed | float | -2.0 - 2.0 | 0.5 | Yes | `"Zoom Speed"` |
| zoomScale | float | 0.01 - 1.0 | 0.1 | Yes | `"Zoom Scale"` |
| glyphSize | int | 2 - 8 | 3 | No | `"Grid Size"` |
| recursionCount | int | 2 - 12 | 6 | No | `"Recursion"` |
| curvature | float | 0.0 - 2.0 | 0.0 | Yes | `"Curvature"` |
| spinSpeed | float | -ROTATION_SPEED_MAX - ROTATION_SPEED_MAX | 0.0 | Yes | `"Spin Speed"` (ModulatableSliderSpeedDeg) |
| lissajous.amplitude | float | 0.0 - 0.5 | 0.3 | Yes | `"Amplitude"` (via DrawLissajousControls) |
| lissajous.motionSpeed | float | 0.0 - 5.0 | 1.0 | Yes | `"Speed"` (via DrawLissajousControls) |
| baseFreq | float | 27.5 - 440.0 | 55.0 | Yes | `"Base Freq (Hz)"` |
| maxFreq | float | 1000.0 - 16000.0 | 14000.0 | Yes | `"Max Freq (Hz)"` |
| gain | float | 0.1 - 10.0 | 2.0 | Yes | `"Gain"` |
| curve | float | 0.1 - 3.0 | 1.5 | Yes | `"Contrast"` |
| baseBright | float | 0.0 - 1.0 | 0.15 | Yes | `"Base Bright"` |
| blendIntensity | float | 0.0 - 5.0 | 1.0 | Yes | (in STANDARD_GENERATOR_OUTPUT) |

### Constants

- Enum name: `TRANSFORM_COLOR_STRETCH_BLEND`
- Display name: `"Color Stretch"`
- Badge: `"GEN"` (auto-set by REGISTER_GENERATOR)
- Section index: `12` (Texture)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by REGISTER_GENERATOR)
- Field name: `colorStretch`
- Param prefix: `"colorStretch."`

### UI Layout

```
Audio:
  Base Freq (Hz)    ModulatableSlider  "%.1f"
  Max Freq (Hz)     ModulatableSlider  "%.0f"
  Gain              ModulatableSlider  "%.1f"
  Contrast          ModulatableSlider  "%.2f"
  Base Bright       ModulatableSlider  "%.2f"

Geometry:
  Grid Size         SliderInt          2-8
  Recursion         SliderInt          2-12
  Zoom Scale        ModulatableSlider  "%.3f"

Focus:
  DrawLissajousControls(&cfg->lissajous, "colorStretch", "colorStretch.lissajous", ms)

Animation:
  Zoom Speed        ModulatableSlider  "%.2f"
  Curvature         ModulatableSlider  "%.2f"
  Spin Speed        ModulatableSliderSpeedDeg
```

Output section via `STANDARD_GENERATOR_OUTPUT(colorStretch)`.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create color_stretch.h

**Files**: `src/effects/color_stretch.h` (create)
**Creates**: `ColorStretchConfig`, `ColorStretchEffect`, function declarations

**Do**: Create the header following `dream_fractal.h` as pattern. Define `ColorStretchConfig` with all fields from the Design section (include `DualLissajousConfig lissajous` with overridden defaults). Define `COLOR_STRETCH_CONFIG_FIELDS` macro -- use `DUAL_LISSAJOUS_CONFIG_FIELDS` prefixed through the lissajous member (list as `lissajous.amplitude, lissajous.motionSpeed, lissajous.freqX1, lissajous.freqY1, lissajous.freqX2, lissajous.freqY2, lissajous.offsetX2, lissajous.offsetY2`). Define `ColorStretchEffect` typedef struct with all `*Loc` fields from Design. Declare `ColorStretchEffectInit(ColorStretchEffect*, const ColorStretchConfig*)`, `ColorStretchEffectSetup(ColorStretchEffect*, const ColorStretchConfig*, float deltaTime, const Texture2D& fftTexture)`, `ColorStretchEffectUninit(ColorStretchEffect*)`, `ColorStretchRegisterParams(ColorStretchConfig*)`. Include `"raylib.h"`, `"config/dual_lissajous_config.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker errors expected yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create color_stretch.cpp

**Files**: `src/effects/color_stretch.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation following `dream_fractal.cpp` as pattern.

Includes: `"color_stretch.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

`ColorStretchEffectInit`: Load `"shaders/color_stretch.fs"`, cache all uniform locations (names match Design section uniform names), init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`, set `zoomPhase = 0.0f`, `spinPhase = 0.0f`. On shader fail return false. On LUT fail, unload shader then return false.

`ColorStretchEffectSetup`: Accumulate `zoomPhase += cfg->zoomSpeed * deltaTime`, accumulate `spinPhase += cfg->spinSpeed * deltaTime`. Call `DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &focusX, &focusY)` to get focus offset, pack as `float[2]` and bind to `focusOffset` uniform as `SHADER_UNIFORM_VEC2`. Call `ColorLUTUpdate`. Set resolution, fftTexture, sampleRate (`(float)AUDIO_SAMPLE_RATE`). Bind all other config uniforms. Bind gradientLUT texture.

`ColorStretchEffectUninit`: Unload shader, `ColorLUTUninit`.

`ColorStretchRegisterParams`: Register all modulatable params from the Parameters table. Use `"colorStretch."` prefix. `spinSpeed` bounds: `-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX`. Register lissajous params: `"colorStretch.lissajous.amplitude"` (0.0-0.5) and `"colorStretch.lissajous.motionSpeed"` (0.0-5.0), following isoflow.cpp pattern.

UI section (`// === UI ===`): `static void DrawColorStretchParams(EffectConfig*, const ModSources*, ImU32)`. Follow the UI Layout from Design exactly. Use `ModulatableSliderSpeedDeg` for spinSpeed. For Focus section use `DrawLissajousControls(&cfg->lissajous, "colorStretch", "colorStretch.lissajous", modSources)` from `ui/ui_units.h`.

Bridge functions: `SetupColorStretch(PostEffect*)` delegates to `ColorStretchEffectSetup`. `SetupColorStretchBlend(PostEffect*)` calls `BlendCompositorApply`.

Registration: `STANDARD_GENERATOR_OUTPUT(colorStretch)` then `REGISTER_GENERATOR(TRANSFORM_COLOR_STRETCH_BLEND, ColorStretch, colorStretch, "Color Stretch", SetupColorStretchBlend, SetupColorStretch, 12, DrawColorStretchParams, DrawOutput_colorStretch)`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create color_stretch.fs

**Files**: `shaders/color_stretch.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from Design verbatim. The shader code is fully specified -- transcribe it as-is. Include the attribution block before `#version 330`.

**Verify**: File exists at `shaders/color_stretch.fs`.

---

#### Task 2.3: Integration registration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (modify all)
**Depends on**: Wave 1 complete

**Do**:

`src/config/effect_config.h`:
- Add `#include "effects/color_stretch.h"` with other effect includes
- Add `TRANSFORM_COLOR_STRETCH_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
- Add `ColorStretchConfig colorStretch;` to `EffectConfig` struct

`src/render/post_effect.h`:
- Add `#include "effects/color_stretch.h"` with other effect includes
- Add `ColorStretchEffect colorStretch;` to `PostEffect` struct

`CMakeLists.txt`:
- Add `src/effects/color_stretch.cpp` to `EFFECTS_SOURCES`

`src/config/effect_serialization.cpp`:
- Add `#include "effects/color_stretch.h"`
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorStretchConfig, COLOR_STRETCH_CONFIG_FIELDS)`
- Add `X(colorStretch) \` to `EFFECT_CONFIG_FIELDS(X)` macro

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears under Texture category (section 12)
- [ ] Enabling shows fractal zoom tunnel
- [ ] glyphSize slider changes subdivision geometry
- [ ] Curvature creates dome/tunnel warp
- [ ] Spin rotates the tunnel
- [ ] Focus offset shifts zoom trajectory
- [ ] FFT audio drives per-depth brightness
- [ ] Gradient LUT colors the recursion depths
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to all registered params
