# Snake Skin

A Texture-category generator that tiles the screen with overlapping reptile scales. Each scale has pointed spoke ridges, tapered shape, and 3D relief shading. A horizontal sine wave undulates the grid like a slithering body, scales scroll vertically at a modulatable rate, and per-scale twinkle flashes punctuate the surface. The gradient LUT colors each scale by its random hash `n`, and the same `n` drives a per-scale FFT frequency lookup so each scale pulses with its own spectral band.

**Research**: `docs/research/snake_skin.md`

## Design

### Types

```cpp
struct SnakeSkinConfig {
  bool enabled = false;

  // Geometry
  float scaleSize = 10.0f;         // Grid density (3.0-30.0)
  float spikeAmount = 0.6f;        // Spike intensity (-1.0-2.0)
  float spikeFrequency = 1.0f;     // Spoke ridges per scale (0.0-16.0)
  float sag = -0.8f;               // Scale taper (-2.0-0.0)

  // Motion (speeds accumulate on CPU into phases)
  float scrollSpeed = 0.0f;        // Vertical scroll rate (-5.0-5.0)
  float waveSpeed = 1.0f;          // Horizontal wave rate (0.1-5.0)
  float waveAmplitude = 0.1f;      // Wave displacement (0.0-0.5)
  float twinkleSpeed = 1.0f;       // Twinkle pulse rate (0.1-5.0)
  float twinkleIntensity = 0.5f;   // Twinkle flash brightness (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SNAKE_SKIN_CONFIG_FIELDS                                               \
  enabled, scaleSize, spikeAmount, spikeFrequency, sag, scrollSpeed,           \
      waveSpeed, waveAmplitude, twinkleSpeed, twinkleIntensity, baseFreq,      \
      maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct SnakeSkinEffect {
  Shader shader;
  int resolutionLoc;
  int scaleSizeLoc;
  int spikeAmountLoc;
  int spikeFrequencyLoc;
  int sagLoc;
  int scrollPhaseLoc;
  int wavePhaseLoc;
  int waveAmplitudeLoc;
  int twinklePhaseLoc;
  int twinkleIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int fftTextureLoc;

  float scrollPhase;
  float wavePhase;
  float twinklePhase;

  ColorLUT *gradientLUT;
} SnakeSkinEffect;
```

### Algorithm

The shader tiles the screen with a staggered grid of 5 overlapping scales per cell. Each scale is a tapered ellipse with radial spoke ridges, shaded for 3D relief. Per-scale random brightness (with twinkle flashes and FFT audio reactivity) multiplies the scale color, and gradient-LUT coloring replaces the reference's hardcoded greens/yellows. A horizontal sine wave displaces uv.x for a slithering effect, and uv2.y is offset by scrollPhase to crawl the skin vertically.

#### Shader: `shaders/snake_skin.fs`

```glsl
// Based on "Rainbow Snake" by Martijn Steinrucken aka BigWings (2015)
// https://www.shadertoy.com/view/4dc3R2
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring, per-scale FFT reactivity, uniform shape
// (no spatial morph), CPU-accumulated phases, aspect ratio correction

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float scaleSize;
uniform float spikeAmount;
uniform float spikeFrequency;
uniform float sag;
uniform float scrollPhase;
uniform float wavePhase;
uniform float waveAmplitude;
uniform float twinklePhase;
uniform float twinkleIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float sampleRate;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;

#define MOD3 vec3(.1031, .11369, .13787)

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float brightness(vec2 uv, vec2 id) {
    float n = hash12(id);
    float c = mix(0.7, 1.0, n);

    float x = abs(id.x - scaleSize * 0.5);
    float stripes = sin(x * 0.65 + sin(id.y * 0.5) + 0.3) * 0.5 + 0.5;
    stripes = pow(stripes, 4.0);
    c *= 1.0 - stripes * 0.5;

    float twinkle = sin(twinklePhase + n * 6.28) * 0.5 + 0.5;
    twinkle = pow(twinkle, 40.0);
    c += twinkle * twinkleIntensity;

    float freq = baseFreq * pow(maxFreq / baseFreq, n);
    float bin = freq / (sampleRate * 0.5);
    float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float audioBright = baseBright + mag;

    return c * audioBright;
}

float spokes(vec2 uv, float spikeFreq) {
    vec2 p = vec2(0.0, 1.0);
    vec2 d = p - uv;
    float l = length(d);
    d /= l;

    vec2 up = vec2(1.0, 0.0);

    float c = dot(up, d);
    c = abs(sin(c * spikeFreq));
    c *= l;

    return c;
}

vec4 ScaleInfo(vec2 uv, vec2 p, vec3 shape) {
    float spikeAmt = shape.x;
    float spikeFreq = shape.y;
    float sagAmt = shape.z;

    uv -= p;
    uv = uv * 2.0 - 1.0;

    float d2 = spokes(uv, spikeFreq);

    uv.x *= 1.0 + uv.y * sagAmt;

    float d1 = dot(uv, uv);

    float threshold = 1.0;
    float d = d1 + d2 * spikeAmt;

    float f = 0.05;
    float c = smoothstep(threshold, threshold - f, d);

    return vec4(d1, d2, d, c);
}

vec4 ScaleCol(vec2 uv, vec2 p, vec4 i, vec2 scaleId) {
    float n = hash12(scaleId);
    vec3 lutColor = texture(gradientLUT, vec2(n, 0.5)).rgb;

    uv -= p;

    float grad = 1.0 - uv.y;
    float lum = (grad + i.x) * 0.2 + 0.5;

    vec3 spokeShade = lutColor * 0.3;
    vec3 edgeShade = lutColor * 1.5;

    vec4 c = vec4(lum * lutColor, i.a);
    c.rgb = mix(c.rgb, spokeShade, i.y * i.x * 0.5);
    c.rgb = mix(c.rgb, edgeShade, i.x);

    c = mix(vec4(0.0), c, i.a);

    float fade = 0.3;
    float shadow = smoothstep(1.0 + fade, 1.0, i.z);

    c.a = mix(shadow * 0.25, 1.0, i.a);

    return c;
}

vec4 Scale(vec2 uv, vec2 p, vec3 shape, vec2 scaleId) {
    vec4 info = ScaleInfo(uv, p, shape);
    vec4 c = ScaleCol(uv, p, info, scaleId);
    return c;
}

vec4 ScaleTex(vec2 uv, vec2 uv2, vec3 shape) {
    vec2 id = floor(uv2);
    uv2 -= id;

    vec4 rScale  = Scale(uv2, vec2( 0.5,  0.01), shape, id + vec2(1.0, 0.0));
    vec4 lScale  = Scale(uv2, vec2(-0.5,  0.01), shape, id + vec2(0.0, 0.0));
    vec4 bScale  = Scale(uv2, vec2( 0.0, -0.5 ), shape, id + vec2(0.0, 0.0));
    vec4 tScale  = Scale(uv2, vec2( 0.0,  0.5 ), shape, id + vec2(0.0, 1.0));
    vec4 t2Scale = Scale(uv2, vec2( 1.0,  0.5 ), shape, id + vec2(2.0, 1.0));

    rScale.rgb  *= brightness(uv, id + vec2(1.0, 0.0));
    lScale.rgb  *= brightness(uv, id + vec2(0.0, 0.0));
    bScale.rgb  *= brightness(uv, id + vec2(0.0, 0.0));
    tScale.rgb  *= brightness(uv, id + vec2(0.0, 1.0));
    t2Scale.rgb *= brightness(uv, id + vec2(2.0, 1.0));

    vec4 c = vec4(0.1, 0.3, 0.2, 1.0);
    c = mix(c, bScale,  bScale.a);
    c = mix(c, rScale,  rScale.a);
    c = mix(c, lScale,  lScale.a);
    c = mix(c, tScale,  tScale.a);
    c = mix(c, t2Scale, t2Scale.a);

    return c;
}

void main() {
    vec2 uv = fragTexCoord;
    uv.x *= resolution.x / resolution.y;
    uv.x += sin(wavePhase + uv.y * scaleSize) * waveAmplitude;

    uv -= 0.5;
    vec2 uv2 = uv * scaleSize;
    uv2.y -= scrollPhase;
    uv += 0.5;

    vec3 shape = vec3(spikeAmount, spikeFrequency, sag);
    vec4 c = ScaleTex(uv, uv2, shape);

    finalColor = vec4(c.rgb, 1.0);
}
```

#### CPU phase accumulation (inside `SnakeSkinEffectSetup`)

```cpp
e->scrollPhase += cfg->scrollSpeed * deltaTime;
e->wavePhase += cfg->waveSpeed * deltaTime;
e->twinklePhase += cfg->twinkleSpeed * deltaTime;
```

All three phases are sent to the shader as plain floats. The `fftTexture` is passed via `SetShaderValueTexture` and `sampleRate` comes from `AUDIO_SAMPLE_RATE`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scaleSize | float | 3.0-30.0 | 10.0 | Y | Scale Size |
| spikeAmount | float | -1.0-2.0 | 0.6 | Y | Spike Amount |
| spikeFrequency | float | 0.0-16.0 | 1.0 | Y | Spike Frequency |
| sag | float | -2.0-0.0 | -0.8 | Y | Sag |
| scrollSpeed | float | -5.0-5.0 | 0.0 | Y | Scroll Speed |
| waveSpeed | float | 0.1-5.0 | 1.0 | Y | Wave Speed |
| waveAmplitude | float | 0.0-0.5 | 0.1 | Y | Wave Amplitude |
| twinkleSpeed | float | 0.1-5.0 | 1.0 | Y | Twinkle Speed |
| twinkleIntensity | float | 0.0-1.0 | 0.5 | Y | Twinkle Intensity |
| baseFreq | float | 27.5-440.0 | 55.0 | Y | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000 | Y | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Y | Gain |
| curve | float | 0.1-3.0 | 1.5 | Y | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Y | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Y | Blend Intensity (via STANDARD_GENERATOR_OUTPUT) |

### Constants

- Enum name: `TRANSFORM_SNAKE_SKIN_BLEND`
- Display name: `"Snake Skin"`
- Badge: `"GEN"` (auto-applied by REGISTER_GENERATOR)
- Section index: `12` (Texture)
- Field name: `snakeSkin`
- Effect struct: `SnakeSkinEffect`
- Config struct: `SnakeSkinConfig`

### UI Layout

Follow Signal Stack order used by `digital_shard.cpp`:

1. `SeparatorText("Audio")` - baseFreq, maxFreq, gain, curve, baseBright (conventions.md FFT Audio UI section)
2. `SeparatorText("Geometry")` - scaleSize, spikeAmount, spikeFrequency, sag
3. `SeparatorText("Motion")` - scrollSpeed, waveSpeed, waveAmplitude
4. `SeparatorText("Twinkle")` - twinkleSpeed, twinkleIntensity
5. Output section via `STANDARD_GENERATOR_OUTPUT(snakeSkin)` (gradient, blend mode, blend intensity)

All sliders use `ModulatableSlider` (no log sliders, no angle sliders — there are no angular fields).

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create `src/effects/snake_skin.h`

**Files**: `src/effects/snake_skin.h`
**Creates**: `SnakeSkinConfig`, `SnakeSkinEffect`, `SNAKE_SKIN_CONFIG_FIELDS` macro, public function declarations

**Do**:
- Follow the exact struct layouts from the Design section.
- Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`.
- Forward-declare `typedef struct ColorLUT ColorLUT;` before `SnakeSkinEffect`.
- Declare: `SnakeSkinEffectInit(SnakeSkinEffect*, const SnakeSkinConfig*)`, `SnakeSkinEffectSetup(SnakeSkinEffect*, const SnakeSkinConfig*, float deltaTime, const Texture2D &fftTexture)`, `SnakeSkinEffectUninit(SnakeSkinEffect*)`, `SnakeSkinRegisterParams(SnakeSkinConfig*)`.
- Inline range comments on each config field per the table in the Design section.
- Include-guard `SNAKE_SKIN_H`.

Pattern to follow: `src/effects/digital_shard.h`.

**Verify**: File compiles standalone after Wave 2 task 2.5 adds the include to `effect_config.h`. Wave 1 alone won't link.

---

### Wave 2: Parallel integration

All Wave 2 tasks touch distinct files. No overlap.

#### Task 2.1: Create `src/effects/snake_skin.cpp`

**Files**: `src/effects/snake_skin.cpp`
**Depends on**: Task 1.1

**Do**:
- Implement `SnakeSkinEffectInit` — load `shaders/snake_skin.fs`, cache all uniform locations listed in `SnakeSkinEffect`, init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`, zero-initialize the three phase accumulators. Cascade cleanup on failure (unload shader if LUT init fails).
- Implement `SnakeSkinEffectSetup` — accumulate `scrollPhase`, `wavePhase`, `twinklePhase` (phase += speed * deltaTime, no fmod unless needed by the pattern seen in `digital_shard.cpp`; follow `synapse_tree.cpp` which does not fmod its phases). Call `ColorLUTUpdate`. Bind `resolution`, all 14 config floats (as-is where they map directly), the three phases, `sampleRate = (float)AUDIO_SAMPLE_RATE`, `gradientLUT` via `SetShaderValueTexture`, `fftTexture` via `SetShaderValueTexture`.
- Implement `SnakeSkinEffectUninit` — `UnloadShader`, `ColorLUTUninit`.
- Implement `SnakeSkinRegisterParams` — register all 14 modulatable params plus `blendIntensity` using the exact ranges from the Parameters table.
- Define non-static `SetupSnakeSkin(PostEffect*)` bridge that calls `SnakeSkinEffectSetup` with `pe->snakeSkin`, `&pe->effects.snakeSkin`, `pe->currentDeltaTime`, `pe->fftTexture`.
- Define non-static `SetupSnakeSkinBlend(PostEffect*)` that calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.snakeSkin.blendIntensity, pe->effects.snakeSkin.blendMode)`.
- Define `static void DrawSnakeSkinParams(EffectConfig*, const ModSources*, ImU32)` implementing the UI layout from the Design section. Suppress the `categoryGlow` parameter with `(void)categoryGlow;`.
- Append `STANDARD_GENERATOR_OUTPUT(snakeSkin)` and `REGISTER_GENERATOR(TRANSFORM_SNAKE_SKIN_BLEND, SnakeSkin, snakeSkin, "Snake Skin", SetupSnakeSkinBlend, SetupSnakeSkin, 12, DrawSnakeSkinParams, DrawOutput_snakeSkin)` inside `// clang-format off ... // clang-format on`.

Includes to add (clang-format will sort):
- `"snake_skin.h"`
- `"audio/audio.h"` (for `AUDIO_SAMPLE_RATE`)
- `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`
- `"config/effect_config.h"`, `"config/effect_descriptor.h"`
- `"imgui.h"`
- `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`
- `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
- `<stddef.h>`

Pattern to follow: `src/effects/digital_shard.cpp` (same category, same FFT + LUT pattern) and `src/effects/synapse_tree.cpp` (multi-phase accumulation).

**Verify**: `cmake.exe --build build` compiles after all other Wave 2 tasks complete.

---

#### Task 2.2: Create `shaders/snake_skin.fs`

**Files**: `shaders/snake_skin.fs`
**Depends on**: None (independent)

**Do**:
- Copy the shader code verbatim from the Algorithm section of this plan. The header attribution comment is part of the deliverable.
- Do NOT add tonemap, Reinhard, gamma, or any post-adjustment. Output is straight `finalColor = vec4(c.rgb, 1.0)`.
- Do NOT add debug overlays.

**Verify**: Effect renders a tiled scale grid when enabled. Gradient LUT drives per-scale color. FFT pulses modulate scale brightness when audio plays. Scroll stays still at `scrollSpeed = 0`.

---

#### Task 2.3: Register in `src/config/effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1

**Do**:
- Add `#include "effects/snake_skin.h"` in the alphabetical include block.
- Add `TRANSFORM_SNAKE_SKIN_BLEND,` to `TransformEffectType` enum. Place before `TRANSFORM_ACCUM_COMPOSITE` (the terminal enum slot). Suggested position: after `TRANSFORM_MARBLE_BLEND` to keep recently-added generators grouped, or alphabetically — either is fine; append near the generator block.
- Add `SnakeSkinConfig snakeSkin;` member to `EffectConfig`. Place near other Texture generators.

No action needed in `TransformOrderConfig` — the default constructor initializes the order array by enum value, so new enums are picked up automatically.

**Verify**: Header compiles.

---

#### Task 2.4: Register in `src/render/post_effect.h`

**Files**: `src/render/post_effect.h`
**Depends on**: Task 1.1

**Do**:
- Add `#include "effects/snake_skin.h"` in the alphabetical include block.
- Add `SnakeSkinEffect snakeSkin;` member to `PostEffect` struct, grouped with other generator effect members (e.g., near `DigitalShardEffect digitalShard;`).

**Verify**: Header compiles.

---

#### Task 2.5: Register in `src/config/effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Task 1.1

**Do**:
- Add `#include "effects/snake_skin.h"` alphabetically.
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SnakeSkinConfig, SNAKE_SKIN_CONFIG_FIELDS)` near the other per-config macros.
- Add `X(snakeSkin) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Build succeeds; preset save/load round-trips all Snake Skin fields.

---

#### Task 2.6: Add to `CMakeLists.txt`

**Files**: `CMakeLists.txt`
**Depends on**: Task 1.1

**Do**:
- Add `src/effects/snake_skin.cpp` to the `EFFECTS_SOURCES` list (alphabetical within that list).

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release` regenerates cleanly; subsequent build links the new TU.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] "Snake Skin" appears in the Texture section (section index 12) with the `GEN` badge
- [ ] Enabling the effect renders a tiled scale grid with gradient-LUT colors
- [ ] Per-scale twinkle flashes fire at irregular intervals when `twinkleIntensity > 0`
- [ ] At `scrollSpeed = 0` the skin is still; increasing the value scrolls vertically; negative values scroll the opposite direction
- [ ] Horizontal wave undulation responds to `waveAmplitude` and `waveSpeed`
- [ ] Scales pulse per-band with audio — low-frequency content brightens some scales, high-frequency content brightens others
- [ ] Modulation routes to all 14 listed params plus `blendIntensity`
- [ ] Preset save/load preserves all fields
- [ ] Effect can be reordered via drag-drop in the transform pipeline UI
