# Spiral Nest

Procedural generator that creates nested spirals winding outward from center, with continuous zoom animation and FFT-driven radial brightness. Adapted from Frank Force's "Spiral of Spirals 2" Shadertoy shader, replacing HSV coloring with gradient LUT and adding audio reactivity.

**Research**: `docs/research/spiral_nest.md`

## Design

### Types

**SpiralNestConfig** (`src/effects/spiral_nest.h`):

```cpp
struct SpiralNestConfig {
  bool enabled = false;

  // Geometry
  float zoom = 100.0f; // Base spiral arm count (10.0-400.0)

  // Animation
  float zoomSpeed = 0.1f;  // Exponential zoom rate (-2.0-2.0)
  float animSpeed = 0.05f; // Time evolution rate (0.01-1.0)

  // Glow
  float glowIntensity = 2.0f; // Overall brightness multiplier (0.5-10.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SPIRAL_NEST_CONFIG_FIELDS                                              \
  enabled, zoom, zoomSpeed, animSpeed, glowIntensity, baseFreq, maxFreq,      \
      gain, curve, baseBright, gradient, blendMode, blendIntensity
```

**SpiralNestEffect** (`src/effects/spiral_nest.h`):

```cpp
typedef struct ColorLUT ColorLUT;

typedef struct SpiralNestEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float zoomAccum; // CPU-accumulated zoom offset
  float timeAccum; // CPU-accumulated animation time
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int zoomLoc;
  int timeAccumLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SpiralNestEffect;
```

### Algorithm

Full shader for `shaders/spiral_nest.fs`. Transcribe this verbatim - do not reinterpret or modify formulas.

```glsl
// Based on "Spiral of Spirals 2" by KilledByAPixel (Frank Force)
// https://www.shadertoy.com/view/lsdBzX
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced HSV coloring with gradient LUT, removed mouse interaction,
// added FFT audio reactivity, configurable zoom and animation speed

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float zoom;
uniform float timeAccum;
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float PI = 3.14159265359;

void main() {
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.x;
    uv *= zoom;

    float a = atan(uv.y, uv.x);
    float d = length(uv);

    // Archimedean spiral mapping
    float i = d;
    float p = a / (2.0 * PI) + 0.5;
    i -= p;
    a += 2.0 * PI * floor(i);

    float t = timeAccum;

    // Gradient LUT index (repurposed hue computation)
    float h = 0.5 * a;
    h *= t;
    h = 0.5 * (sin(h) + 1.0);
    h = pow(h, 3.0);
    h += 4.222 * t + 0.4;

    // Nested spiral structure
    a *= (floor(i) + p);

    // Brightness pattern
    float v = a;
    v *= t;
    v = sin(v);
    v = 0.5 * (v + 1.0);
    v = pow(v, 4.0);
    v *= pow(sin(fract(i) * PI), 0.4);
    v *= min(d, 1.0);

    // FFT brightness (radial frequency mapping)
    float t_fft = clamp(2.0 * d / zoom, 0.0, 1.0);
    float freq = baseFreq * pow(maxFreq / baseFreq, t_fft);
    float bin = freq / (sampleRate * 0.5);
    float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    // Final composition
    vec3 color = texture(gradientLUT, vec2(fract(h), 0.5)).rgb;
    finalColor = vec4(color * v * brightness * glowIntensity, 1.0);
}
```

**CPU-side accumulation** (in `SpiralNestEffectSetup`):

- `e->zoomAccum += cfg->zoomSpeed * deltaTime`
- `e->timeAccum += cfg->animSpeed * deltaTime`
- Shader `zoom` uniform receives `cfg->zoom * expf(e->zoomAccum)` (exponential zoom)
- Shader `timeAccum` uniform receives `e->timeAccum` directly

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| zoom | float | 10.0-400.0 | 100.0 | Yes | `"Zoom##spiralNest"` |
| zoomSpeed | float | -2.0-2.0 | 0.1 | Yes | `"Zoom Speed##spiralNest"` |
| animSpeed | float | 0.01-1.0 | 0.05 | Yes | `"Anim Speed##spiralNest"` |
| glowIntensity | float | 0.5-10.0 | 2.0 | Yes | `"Glow Intensity##spiralNest"` |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | `"Base Freq (Hz)##spiralNest"` |
| maxFreq | float | 1000.0-16000.0 | 14000.0 | Yes | `"Max Freq (Hz)##spiralNest"` |
| gain | float | 0.1-10.0 | 2.0 | Yes | `"Gain##spiralNest"` |
| curve | float | 0.1-3.0 | 1.5 | Yes | `"Contrast##spiralNest"` |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | `"Base Bright##spiralNest"` |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | (via STANDARD_GENERATOR_OUTPUT) |

### Constants

- Enum: `TRANSFORM_SPIRAL_NEST_BLEND`
- Display name: `"Spiral Nest"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `10` (Geometric)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Field name: `spiralNest`
- Param prefix: `"spiralNest."`

### UI Section Ordering

```
Audio       -- baseFreq, maxFreq, gain, curve, baseBright (standard)
Geometry    -- zoom
Animation   -- zoomSpeed, animSpeed
Glow        -- glowIntensity
```

All sliders use `ModulatableSlider`. No angular or log-scale params.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create spiral_nest.h

**Files**: `src/effects/spiral_nest.h` (create)
**Creates**: `SpiralNestConfig`, `SpiralNestEffect` types, function declarations

**Do**: Create the header file. Use the exact struct layouts from the Design section. Follow `spiral_walk.h` for structure: include guard, includes (`"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`), config struct with `SPIRAL_NEST_CONFIG_FIELDS` macro, forward-declare `ColorLUT`, effect struct, and 4 function declarations:

- `bool SpiralNestEffectInit(SpiralNestEffect *e, const SpiralNestConfig *cfg)`
- `void SpiralNestEffectSetup(SpiralNestEffect *e, const SpiralNestConfig *cfg, float deltaTime, const Texture2D &fftTexture)`
- `void SpiralNestEffectUninit(SpiralNestEffect *e)`
- `void SpiralNestRegisterParams(SpiralNestConfig *cfg)`

**Verify**: File compiles standalone (no undefined includes).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create spiral_nest.cpp

**Files**: `src/effects/spiral_nest.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation file. Follow `spiral_walk.cpp` as the structural pattern.

Includes (generator pattern):
`"spiral_nest.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stddef.h>`

**Init**: Load shader `"shaders/spiral_nest.fs"`. Cache all 12 uniform locations (resolution, fftTexture, sampleRate, zoom, timeAccum, glowIntensity, baseFreq, maxFreq, gain, curve, baseBright, gradientLUT). Init `ColorLUT` from `cfg->gradient`. If LUT fails, unload shader and return false. Zero both accumulators.

**Setup**: Accumulate `zoomAccum += cfg->zoomSpeed * deltaTime` and `timeAccum += cfg->animSpeed * deltaTime`. Compute effective zoom as `cfg->zoom * expf(e->zoomAccum)`. Update ColorLUT. Bind all uniforms. Bind fftTexture and gradientLUT textures. Use `(float)AUDIO_SAMPLE_RATE` for sampleRate.

**Uninit**: Unload shader, `ColorLUTUninit` gradientLUT.

**RegisterParams**: Register all 10 modulatable params with `"spiralNest."` prefix. Ranges from the Parameters table. `blendIntensity` range 0.0-5.0. No angular params (no `ROTATION_OFFSET_MAX` or `ROTATION_SPEED_MAX`).

**UI section** (`// === UI ===`): `static void DrawSpiralNestParams(EffectConfig *e, const ModSources *modSources, ImU32 categoryGlow)`. Follow section ordering from Design: Audio (standard 5), Geometry (zoom), Animation (zoomSpeed, animSpeed), Glow (glowIntensity). All use `ModulatableSlider`. Format strings: baseFreq `"%.1f"`, maxFreq `"%.0f"`, gain `"%.1f"`, curve `"%.2f"`, baseBright `"%.2f"`, zoom `"%.0f"`, zoomSpeed `"%.2f"`, animSpeed `"%.3f"`, glowIntensity `"%.1f"`.

**Bridge functions** (non-static):
- `SetupSpiralNest(PostEffect *pe)` - calls `SpiralNestEffectSetup` with `pe->spiralNest`, `pe->effects.spiralNest`, `pe->currentDeltaTime`, `pe->fftTexture`
- `SetupSpiralNestBlend(PostEffect *pe)` - calls `BlendCompositorApply` with `pe->blendCompositor`, `pe->generatorScratch.texture`, blend intensity and mode from `pe->effects.spiralNest`

**Registration** (bottom of file, wrapped in `// clang-format off`/`on`):
```
STANDARD_GENERATOR_OUTPUT(spiralNest)
REGISTER_GENERATOR(TRANSFORM_SPIRAL_NEST_BLEND, SpiralNest, spiralNest,
                   "Spiral Nest", SetupSpiralNestBlend, SetupSpiralNest, 10,
                   DrawSpiralNestParams, DrawOutput_spiralNest)
```

**Verify**: `cmake.exe --build build` compiles (after all Wave 2 merges).

---

#### Task 2.2: Create spiral_nest.fs shader

**Files**: `shaders/spiral_nest.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Transcribe the Algorithm section verbatim. This is a mechanical copy - do not reinterpret, simplify, or modify any formulas. The attribution header, `#version 330`, uniforms, constants, and `main()` body are all specified in the Algorithm section.

**Verify**: File exists with correct attribution header and all 12 uniforms declared.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/spiral_nest.h"` with the other effect includes
2. Add `TRANSFORM_SPIRAL_NEST_BLEND,` to the `TransformEffectType` enum, before `TRANSFORM_ACCUM_COMPOSITE`
3. Add `SpiralNestConfig spiralNest;` to the `EffectConfig` struct (after `infinityMatrix`, before the order config)

**Verify**: Header compiles with no errors.

---

#### Task 2.4: Add effect member to post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/spiral_nest.h"` with the other effect includes
2. Add `SpiralNestEffect spiralNest;` to the `PostEffect` struct (near the other generator effect members)

**Verify**: Header compiles with no errors.

---

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/spiral_nest.cpp` to the `EFFECTS_SOURCES` list.

**Verify**: CMake configure succeeds.

---

#### Task 2.6: Add serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/spiral_nest.h"` with the other effect includes
2. Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpiralNestConfig, SPIRAL_NEST_CONFIG_FIELDS)`
3. Add `X(spiralNest) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles with no errors.

---

## Final Verification

- [x] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds with no warnings
- [x] Effect appears in Effects window under Geometric section
- [x] Spiral pattern visible when enabled
- [x] Zoom and animation controls respond in real-time
- [x] Audio reactivity visible with music playing
- [x] Gradient palette changes affect spiral colors
- [ ] Preset save/load preserves settings

## Implementation Notes

- **Removed `zoomSpeed`/`zoomAccum`**: The planned exponential zoom animation (`zoom * exp(zoomAccum)`) caused the pattern to alias into nothing as the accumulator grew unbounded. The reference shader uses a static zoom (100-400 via mouse Y) with no continuous animation. Zoom is now a static modulatable parameter only.
- **`timeAccum` grows unbounded**: Same behavior as the reference (`0.05 * iTime`). At very large values the sine terms alias, but at default `animSpeed = 0.05` this takes hours to become noticeable. Acceptable for real-time use.
