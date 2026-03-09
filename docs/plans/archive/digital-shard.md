# Digital Shard

Noise-driven angular shard generator that shatters the screen into blocky chromatic fragments. Accumulates a scalar brightness across ~100 iterations with per-iteration positional offsets, where noise-quantized rotation creates geometric cell structures. Per-cell FFT reactivity maps noise cell identity to audio frequency, and the final scalar maps through a gradient LUT for coloring.

**Research**: `docs/research/digital_shard.md`

## Design

### Types

**DigitalShardConfig** (`src/effects/digital_shard.h`):

```cpp
struct DigitalShardConfig {
  bool enabled = false;

  // Geometry
  int iterations = 100;            // Loop count (20-100)
  float zoom = 0.4f;              // Base coordinate scale (0.1-2.0)
  float aberrationSpread = 0.01f; // Per-iteration position offset (0.001-0.05)
  float noiseScale = 64.0f;      // Noise texture tiling (16.0-256.0)
  float rotationLevels = 8.0f;   // Angle quantization steps (2.0-16.0)
  float softness = 0.0f;         // Hard binary to smooth gradient (0.0-1.0)
  float speed = 1.0f;            // Animation rate (0.1-5.0)

  // Audio
  float baseFreq = 55.0f;        // Lowest FFT frequency (27.5-440.0)
  float maxFreq = 14000.0f;      // Highest FFT frequency (1000-16000)
  float gain = 2.0f;             // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;            // Contrast exponent (0.1-3.0)
  float baseBright = 0.15f;      // Baseline brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;   // Blend strength (0.0-5.0)
};
```

**DigitalShardEffect** (`src/effects/digital_shard.h`):

```cpp
typedef struct DigitalShardEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int noiseTextureLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int iterationsLoc;
  int zoomLoc;
  int aberrationSpreadLoc;
  int noiseScaleLoc;
  int rotationLevelsLoc;
  int softnessLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  float time;
  ColorLUT *gradientLUT;
} DigitalShardEffect;
```

Config fields macro: `enabled, iterations, zoom, aberrationSpread, noiseScale, rotationLevels, softness, speed, baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity`

### Algorithm

The shader is a direct transcription of XorDev's "Gltch" with these substitutions applied:

| Reference | Adapted |
|-----------|---------|
| `texture(iChannel0,(x)/64.).r` | `texture(noiseTexture, (x) / noiseScale).r` |
| `iResolution.xy` | `resolution` |
| `(I+I-r)/r.y` | `(fragTexCoord * 2.0 - 1.0) * vec2(r.x / r.y, 1.0)` <!-- Intentional: matches laser_dance.fs pattern; generators center at screen center, no center uniform needed --> |
| `.4` (zoom factor) | `zoom` |
| `i/1e2` (aberration per step) | `t * aberrationSpread` where `t` = iteration mapped to -1..1 |
| `8.` (rotation levels) | `rotationLevels` |
| `.785` (PI/4 angle step) | `6.2831853 / rotationLevels` (TWO_PI / levels) |
| `ceil(cos(...))` | `step(0.0, cos(...))` when softness < 0.001, else `smoothstep(-softness, softness, cos(...))` <!-- Intentional: smoothstep(0,0,x) is undefined in GLSL when edge0>=edge1; 0.001 = minimum softness threshold for hard step fallback --> |
| `iTime` | `time` |
| `vec4(1.+i,2.-abs(i+i),1.-i,1)/1e2` | `1.0 / float(iterations)` (scalar accumulation) |
| Post-loop color | `texture(gradientLUT, vec2(accum, 0.5)).rgb * brightness` |

Full shader GLSL:

```glsl
// Attribution header (see Task 2.1)
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D noiseTexture;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform int iterations;
uniform float zoom;
uniform float aberrationSpread;
uniform float noiseScale;
uniform float rotationLevels;
uniform float softness;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Noise macro — samples R channel of shared noise texture
#define N(x) texture(noiseTexture, (x) / noiseScale).r

void main() {
    vec2 r = resolution;

    // Stable cell coordinate for FFT mapping (at mid-aberration, t=0)
    vec2 cellCoord = (fragTexCoord * 2.0 - 1.0) * vec2(r.x / r.y, 1.0) / zoom;
    cellCoord /= (0.1 + N(cellCoord));
    float cellId = N(cellCoord);

    // Accumulate scalar brightness across iterations
    float accum = 0.0;
    vec2 c;
    float invIter = 1.0 / float(iterations);
    float angleStep = 6.2831853 / rotationLevels;

    for (int i = 0; i < iterations; i++) {
        // Map iteration index to -1..1 for symmetric aberration spread
        float t = float(i) * invIter * 2.0 - 1.0;

        // Centered, aspect-corrected coordinates with per-iteration aberration
        c = (fragTexCoord * 2.0 - 1.0) * vec2(r.x / r.y, 1.0)
            / (zoom + t * aberrationSpread);

        // Noise cell scaling (0.1 floor prevents coordinate collapse)
        c /= (0.1 + N(c));

        // Quantized rotation matrix (vec4(0,33,11,0) ≈ vec4(0, π/2, 3π/2, 0)
        // for rotation matrix structure: cos, -sin, sin, cos)
        mat2 rot = mat2(cos(
            ceil(N(c) * rotationLevels) * angleStep + vec4(0, 33, 11, 0)));

        // Rotated x divided by stripe-frequency noise
        float x = (c * rot).x / N(N(c) + ceil(c) + time);

        // Threshold: hard binary (softness=0) or smooth gradient
        float val = cos(x);
        float s = (softness < 0.001)
            ? step(0.0, val)
            : smoothstep(-softness, softness, val);

        accum += s * invIter;
    }

    // Per-cell FFT brightness — noise cell identity maps to frequency
    float freq = baseFreq * pow(maxFreq / baseFreq, cellId);
    float bin = freq / (sampleRate * 0.5);
    float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
    float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    // Map through gradient LUT and apply brightness
    vec3 color = texture(gradientLUT, vec2(clamp(accum, 0.0, 1.0), 0.5)).rgb;
    finalColor = vec4(color * brightness, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| iterations | int | 20-100 | 100 | No | `"Iterations"` |
| zoom | float | 0.1-2.0 | 0.4 | Yes | `"Zoom"` |
| aberrationSpread | float | 0.001-0.05 | 0.01 | Yes | `"Aberration"` |
| noiseScale | float | 16.0-256.0 | 64.0 | Yes | `"Noise Scale"` |
| rotationLevels | float | 2.0-16.0 | 8.0 | Yes | `"Rotation Levels"` |
| softness | float | 0.0-1.0 | 0.0 | Yes | `"Softness"` |
| speed | float | 0.1-5.0 | 1.0 | Yes | `"Speed"` |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | `"Base Freq (Hz)"` |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | `"Max Freq (Hz)"` |
| gain | float | 0.1-10.0 | 2.0 | Yes | `"Gain"` |
| curve | float | 0.1-3.0 | 1.5 | Yes | `"Contrast"` |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | `"Base Bright"` |
| gradient | ColorConfig | — | gradient mode | No | (gradient widget) |
| blendMode | EffectBlendMode | — | SCREEN | No | (blend combo) |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | `"Blend Intensity"` |

### Constants

- Enum: `TRANSFORM_DIGITAL_SHARD_BLEND` — add before `TRANSFORM_ACCUM_COMPOSITE`
- Display name: `"Digital Shard"`
- Badge: `"GEN"` (auto via `REGISTER_GENERATOR`)
- Section index: `12` (Texture)

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Config and Effect header

**Files**: `src/effects/digital_shard.h` (create)
**Creates**: `DigitalShardConfig`, `DigitalShardEffect` types, function declarations

**Do**: Create the header following `laser_dance.h` as pattern. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Top comment: `// Digital Shard generator` / `// Noise-driven angular shards with per-cell FFT reactivity`. Define `DigitalShardConfig` and `DigitalShardEffect` structs exactly as specified in the Design section. Define `DIGITAL_SHARD_CONFIG_FIELDS` macro. Forward-declare `ColorLUT`. Declare: `DigitalShardEffectInit(DigitalShardEffect*, const DigitalShardConfig*)`, `DigitalShardEffectSetup(DigitalShardEffect*, const DigitalShardConfig*, float, Texture2D)`, `DigitalShardEffectUninit(DigitalShardEffect*)`, `DigitalShardConfigDefault(void)`, `DigitalShardRegisterParams(DigitalShardConfig*)`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Fragment shader

**Files**: `shaders/digital_shard.fs` (create)

**Do**: Create the shader file. Begin with attribution comment block:
```
// Based on "Gltch" by Xor (@XorDev)
// https://www.shadertoy.com/view/mdfGRs
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring, per-cell FFT reactivity, softness parameter, configurable noise/rotation
```
Then implement the Algorithm section verbatim. All uniforms, the `#define N(x)` macro, cell coordinate computation, iteration loop, FFT brightness, and gradient LUT mapping are specified there — transcribe, do not rewrite.

**Verify**: File exists with correct attribution and all uniforms present.

---

#### Task 2.2: Effect module

**Files**: `src/effects/digital_shard.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create following `laser_dance.cpp` as pattern. Includes: `"digital_shard.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/noise_texture.h"`, `"render/post_effect.h"`, `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

Implement:

1. **`DigitalShardEffectInit`**: Load shader `"shaders/digital_shard.fs"`, cache all 17 uniform locations, init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`, init `time = 0.0f`. Cleanup on failure (unload shader if LUT fails).

2. **`DigitalShardEffectSetup`**: Accumulate `time += cfg->speed * deltaTime`, wrap with `fmodf(e->time, 1000.0f)`. Call `ColorLUTUpdate`. Bind all uniforms. Bind `sampleRate` as `(float)AUDIO_SAMPLE_RATE`. Bind `noiseTexture` via `NoiseTextureGet()`. Bind `fftTexture`, `gradientLUT` texture. `iterations` uses `SHADER_UNIFORM_INT`.

3. **`DigitalShardEffectUninit`**: `UnloadShader` + `ColorLUTUninit`.

4. **`DigitalShardConfigDefault`**: `return DigitalShardConfig{};`

5. **`DigitalShardRegisterParams`**: Register all modulatable params (zoom, aberrationSpread, noiseScale, rotationLevels, softness, speed, baseFreq, maxFreq, gain, curve, baseBright, blendIntensity) with `"digitalShard."` prefix and matching ranges.

6. **UI section** (`// === UI ===`): `static void DrawDigitalShardParams(EffectConfig*, const ModSources*, ImU32)` with:
   - `ImGui::SliderInt("Iterations##digitalShard", &c->iterations, 20, 100)` (not modulatable)
   - `ModulatableSlider` for zoom, aberrationSpread (`"%.3f"`), noiseScale (`"%.1f"`), rotationLevels (`"%.1f"`), softness (`"%.2f"`), speed (`"%.2f"`)
   - `ImGui::SeparatorText("Audio")` then standard audio sliders: baseFreq (`"%.1f"`), maxFreq (`"%.0f"`), gain (`"%.1f"`), curve (`"%.2f"`), baseBright (`"%.2f"`)

7. **Bridge functions**: Non-static `SetupDigitalShard(PostEffect* pe)` calling `DigitalShardEffectSetup(...)` with `pe->currentDeltaTime` and `pe->fftTexture`. Non-static `SetupDigitalShardBlend(PostEffect* pe)` calling `BlendCompositorApply(...)` with `pe->generatorScratch.texture`, blend intensity, blend mode.

8. **Registration**: `STANDARD_GENERATOR_OUTPUT(digitalShard)` then `REGISTER_GENERATOR(TRANSFORM_DIGITAL_SHARD_BLEND, DigitalShard, digitalShard, "Digital Shard", SetupDigitalShardBlend, SetupDigitalShard, 12, DrawDigitalShardParams, DrawOutput_digitalShard)` wrapped in clang-format off/on.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/digital_shard.h"` with other effect includes
2. Add `TRANSFORM_DIGITAL_SHARD_BLEND,` before `TRANSFORM_ACCUM_COMPOSITE` in `TransformEffectType` enum
3. Add `DigitalShardConfig digitalShard;` to `EffectConfig` struct

The order array auto-initializes from the enum via the `TransformOrderConfig` constructor loop — no manual edit needed.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: PostEffect and build system

**Files**: `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. In `post_effect.h`: add `#include "effects/digital_shard.h"` with other effect includes. Add `DigitalShardEffect digitalShard;` member in the `PostEffect` struct near other generator effects (after `laserDance` / `shell`).
2. In `CMakeLists.txt`: add `src/effects/digital_shard.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/digital_shard.h"` with other effect includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DigitalShardConfig, DIGITAL_SHARD_CONFIG_FIELDS)` with other per-config macros
3. Add `X(digitalShard) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Implementation Notes

### Noise source: cell noise hash instead of texture sampling

The plan's algorithm used `texture(noiseTexture, (x) / noiseScale).r` — sampling the codebase's shared 1024x1024 trilinear-filtered noise texture. This produced completely smooth output with no hard edges. The reference Shadertoy shader requires **nearest-filtered 64x64** noise (the author notes: "It needs nearest filtering. And preferably should be 64x64"). Each noise cell must return a constant value with no interpolation so that all ~100 iterations within a cell see identical values, preserving hard block edges after accumulation.

**Fix**: Replaced texture sampling with a procedural cell noise hash:
```glsl
float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
#define N(x) hash21(floor((x) * 64.0 / noiseScale))
```
`floor()` quantizes coordinates to integer cells, giving constant values within each cell — equivalent to nearest-filtered sampling. The `noiseTexture` uniform is still declared and bound but unused by the shader.

### Coloring: per-cell gradient sampling instead of luminance mapping

The plan accumulated a scalar brightness and mapped it through the gradient LUT (`texture(gradientLUT, vec2(accum, 0.5))`). Since all "on" blocks accumulate to similar values, a rainbow gradient produced mostly gray.

**Fix**: Use `cellId` (the per-cell random noise value, 0..1) as the gradient coordinate instead. Each block samples a distinct position on the gradient. Since `cellId` also drives FFT frequency mapping (`baseFreq * pow(maxFreq / baseFreq, cellId)`), color and frequency are naturally linked — low-frequency blocks get left-gradient colors, high-frequency blocks get right-gradient colors.

### Softness: smoothstep with epsilon guard

The plan specified `step(0.0, val)` when `softness < 0.001`, else `smoothstep(-softness, softness, val)`. Implementation uses `smoothstep(-softness, softness + 0.001, cosVal)` unconditionally — the `+ 0.001` epsilon prevents undefined behavior from equal `smoothstep` edges when softness is exactly 0, while still producing a hard step at that value. No branch needed.

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Texture generator section (section 12) with "GEN" badge
- [ ] Enabling effect renders noise-driven angular shard pattern
- [ ] `iterations` slider changes visual density and performance
- [ ] `rotationLevels` at low values (2-3) produces large chunky blocks; high values (16) produce fine shards
- [ ] `softness = 0` gives hard binary snap; `softness = 1` gives smooth gradients
- [ ] Audio reactivity: different spatial regions pulse to different frequencies
- [ ] Gradient LUT colors the pattern
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters
