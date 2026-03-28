# Viscera

Organic, fleshy terrain generator using iterated domain warping (warp FBM). Each noise octave feeds back into the next through sinusoidal distortion, creating folded, intestinal structures. Bump-mapped Phong lighting gives the surface 3D quality. Radial pulsation waves expand from center. Colored via gradient LUT with FFT brightness modulation.

**Research**: `docs/research/viscera.md`

## Design

### Types

**VisceraConfig** (in `src/effects/viscera.h`):

```
struct VisceraConfig {
  bool enabled = false;

  // Geometry
  float scale = 9.0f;          // Pattern zoom - lower = larger structures (1.0-20.0)
  float twistAngle = -1.28f;   // Rotation per iteration in radians (-ROTATION_OFFSET_MAX..ROTATION_OFFSET_MAX)
  int iterations = 16;         // Octave count (4-24)
  float freqGrowth = 1.2f;     // Frequency multiplier per octave (1.05-1.5)
  float warpIntensity = 1.0f;  // Domain warp feedback strength (0.0-2.0)

  // Animation
  float animSpeed = 4.0f;      // Phase accumulation rate (0.1-8.0)

  // Pulsation
  float pulseAmplitude = 0.8f;  // Radial pulsation strength (0.0-2.0)
  float pulseFreq = 4.0f;      // Pulsation time frequency (0.5-8.0)
  float pulseRadial = 6.0f;    // Radial ring density (1.0-12.0)

  // Lighting
  float bumpDepth = 0.05f;     // Normal flatness - lower = more pronounced 3D (0.01-0.5)
  float specPower = 32.0f;     // Specular shininess exponent (4.0-128.0)
  float specIntensity = 0.8f;  // Specular highlight brightness (0.0-2.0)

  // Height
  float heightScale = 1.2f;    // Height-to-gradient mapping scale (0.5-3.0)

  // Audio
  float baseFreq = 55.0f;      // FFT low frequency bound Hz (27.5-440.0)
  float maxFreq = 14000.0f;    // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;           // FFT gain (0.1-10.0)
  float curve = 1.5f;          // FFT contrast curve (0.1-3.0)
  float baseBright = 0.15f;    // Minimum brightness floor (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;  // (0.0-5.0)
};
```

**VisceraEffect** (in `src/effects/viscera.h`):

```
typedef struct VisceraEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  Texture2D currentFFTTexture;
  float time;          // CPU-accumulated animation phase
  int resolutionLoc;
  int timeLoc;
  int scaleLoc;
  int twistAngleLoc;
  int iterationsLoc;
  int freqGrowthLoc;
  int warpIntensityLoc;
  int pulseAmplitudeLoc;
  int pulseFreqLoc;
  int pulseRadialLoc;
  int bumpDepthLoc;
  int specPowerLoc;
  int specIntensityLoc;
  int heightScaleLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} VisceraEffect;
```

### Algorithm

The shader (`shaders/viscera.fs`) implements a single-pass procedural generator with three conceptual stages: height field, bump-mapped lighting, and LUT-colored output with FFT modulation.

**Uniforms:**

```glsl
uniform vec2 resolution;
uniform float time;           // CPU-accumulated phase
uniform float scale;          // Initial frequency (ref: s = 9.0)
uniform float twistAngle;     // Rotation per iteration (ref: rotate(5.0))
uniform int iterations;       // Loop count (ref: 16)
uniform float freqGrowth;     // Per-octave multiplier (ref: s *= 1.2)
uniform float warpIntensity;  // sin(q) feedback scale (ref: 1.0)
uniform float pulseAmplitude; // Radial pulse strength (ref: 0.8)
uniform float pulseFreq;      // Pulse time frequency (ref: 4.0)
uniform float pulseRadial;    // Radial ring density (ref: 6.0)
uniform float bumpDepth;      // Normal z-component (ref: 0.05)
uniform float specPower;      // Shininess exponent (ref: 32.0)
uniform float specIntensity;  // Specular brightness (ref: 0.8)
uniform float heightScale;    // Height-to-color scale (ref: 1.2)
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
```

**Stage 1 - Height field (`map` function):**

Transcribed verbatim from reference with config substitutions applied:

```glsl
mat2 rotate(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

float map(vec2 u, float t) {
    vec2 n = vec2(0.0);
    vec2 q = vec2(0.0);
    float d = dot(u, u);
    float s = scale;
    float o = 0.0;
    float j = 0.0;
    mat2 m = rotate(twistAngle);

    for (int i = 0; i < iterations; i++) {
        j += 1.0;
        u = m * u;
        n = m * n;
        q = u * s + t + sin(t * pulseFreq - d * pulseRadial) * pulseAmplitude + j + n;
        o += dot(cos(q) / s, vec2(2.0));
        n -= sin(q) * warpIntensity;
        s *= freqGrowth;
    }

    return o;
}
```

**Stage 2 - Bump-mapped Phong lighting:**

Three samples of `map()` for finite-difference normals, then diffuse + specular:

```glsl
vec2 uv = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0);

float h = map(uv, time);

vec2 e = vec2(2.0 / resolution.y, 0.0);
float hx = map(uv + e.xy, time);
float hy = map(uv + e.yx, time);

vec3 normal = normalize(vec3(h - hx, h - hy, bumpDepth));

vec3 lightPos = normalize(vec3(0.5, 0.5, 1.0));
vec3 viewPos = vec3(0.0, 0.0, 1.0);

float diff = max(dot(normal, lightPos), 0.0);

vec3 reflectDir = reflect(-lightPos, normal);
float spec = pow(max(dot(viewPos, reflectDir), 0.0), specPower);
```

**Stage 3 - Gradient LUT coloring with FFT brightness:**

Replace hardcoded flesh coloring with gradient LUT + FFT:

```glsl
float t = clamp(h * heightScale, 0.0, 1.0);
vec3 lutColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

// FFT brightness using same t that indexes the LUT
float freq = baseFreq * pow(maxFreq / baseFreq, t);
float bin = freq / (sampleRate * 0.5);
float energy = 0.0;
if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;

// Specular color from bright end of gradient
vec3 specColor = texture(gradientLUT, vec2(1.0, 0.5)).rgb;

vec3 col = lutColor * brightness * (0.5 + 0.5 * diff) + specColor * spec * specIntensity;

finalColor = vec4(col, 1.0);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scale | float | 1.0-20.0 | 9.0 | yes | Scale |
| twistAngle | float | -ROTATION_OFFSET_MAX..ROTATION_OFFSET_MAX | -1.28 | yes | Twist Angle |
| iterations | int | 4-24 | 16 | no | Iterations |
| freqGrowth | float | 1.05-1.5 | 1.2 | yes | Freq Growth |
| warpIntensity | float | 0.0-2.0 | 1.0 | yes | Warp Intensity |
| animSpeed | float | 0.1-8.0 | 4.0 | yes | Anim Speed |
| pulseAmplitude | float | 0.0-2.0 | 0.8 | yes | Pulse Amplitude |
| pulseFreq | float | 0.5-8.0 | 4.0 | yes | Pulse Freq |
| pulseRadial | float | 1.0-12.0 | 6.0 | yes | Pulse Radial |
| bumpDepth | float | 0.01-0.5 | 0.05 | yes | Bump Depth |
| specPower | float | 4.0-128.0 | 32.0 | yes | Spec Power |
| specIntensity | float | 0.0-2.0 | 0.8 | yes | Spec Intensity |
| heightScale | float | 0.5-3.0 | 1.2 | yes | Height Scale |
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_VISCERA_BLEND`
- Display name: `"Viscera"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `12` (Texture)
- Macro: `REGISTER_GENERATOR`
- Init signature: `(VisceraEffect*, const VisceraConfig*)`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create viscera.h

**Files**: `src/effects/viscera.h` (create)
**Creates**: `VisceraConfig`, `VisceraEffect` types, function declarations

**Do**: Create the header following shell.h as template. Define `VisceraConfig` and `VisceraEffect` structs exactly as specified in the Design section. Declare `VisceraEffectInit(VisceraEffect*, const VisceraConfig*)`, `VisceraEffectSetup(VisceraEffect*, const VisceraConfig*, float deltaTime, const Texture2D& fftTexture)`, `VisceraEffectUninit(VisceraEffect*)`, `VisceraRegisterParams(VisceraConfig*)`. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Define `VISCERA_CONFIG_FIELDS` macro listing all serializable fields. Forward-declare `ColorLUT`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create viscera.cpp

**Files**: `src/effects/viscera.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow shell.cpp as template. Implement:
- `VisceraEffectInit`: load `shaders/viscera.fs`, cache all uniform locations, init ColorLUT, set `time = 0.0f`. Return false and cleanup on failure.
- `VisceraEffectSetup`: accumulate `time += cfg->animSpeed * deltaTime`, call `ColorLUTUpdate`, bind all uniforms including FFT params (`sampleRate` from `AUDIO_SAMPLE_RATE`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`), bind `gradientLUT` and `fftTexture` textures.
- `VisceraEffectUninit`: unload shader, uninit ColorLUT.
- `VisceraRegisterParams`: register all modulatable params per the Parameters table with dot-separated IDs (`"viscera.scale"`, etc.). Use `TWO_PI_F` for `twistAngle` max. Register `blendIntensity` too.
- `SetupViscera(PostEffect*)` bridge: calls `VisceraEffectSetup` passing `pe->viscera`, `pe->effects.viscera`, `pe->currentDeltaTime`, `pe->fftTexture`.
- `SetupVisceraBlend(PostEffect*)` bridge: calls `BlendCompositorApply` with `pe->generatorScratch.texture`, blend intensity, blend mode.
- Colocated UI section (`DrawVisceraParams`): Geometry section (Scale, Twist Angle via `ModulatableSlider` with range 0.0-TWO_PI_F and `"%.2f"` format, Iterations via `ImGui::SliderInt`, Freq Growth, Warp Intensity), Animation section (Anim Speed), Pulsation section (Pulse Amplitude, Pulse Freq, Pulse Radial), Lighting section (Bump Depth via `ModulatableSliderLog`, Spec Power, Spec Intensity), Height section (Height Scale), Audio section (standard order: Base Freq, Max Freq, Gain, Contrast, Base Bright).
- `STANDARD_GENERATOR_OUTPUT(viscera)` + `REGISTER_GENERATOR(TRANSFORM_VISCERA_BLEND, Viscera, viscera, "Viscera", SetupVisceraBlend, SetupViscera, 12, DrawVisceraParams, DrawOutput_viscera)`

Includes: `"viscera.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

Note: `twistAngle` range is 0.0 to TWO_PI_F (full circle, default 5.0 radians). This exceeds the standard angle slider convention (+/- PI), so use a plain `ModulatableSlider` instead of `ModulatableSliderAngleDeg`. Register with bounds `0.0f` to `TWO_PI_F`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create viscera.fs shader

**Files**: `shaders/viscera.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from the Design. The shader has three stages:

1. Attribution header (before `#version`):
   ```
   // Based on "VisceraPulsating" by hdrp0720
   // https://www.shadertoy.com/view/tfVfRV
   // License: CC BY-NC-SA 3.0 Unported
   // Modified: replaced hardcoded coloring with gradient LUT + FFT audio reactivity, parameterized all constants
   ```

2. `#version 330`, declare `in vec2 fragTexCoord`, `out vec4 finalColor`, all uniforms from Design.

3. `rotate()` and `map()` functions - transcribe verbatim from Algorithm Stage 1. The `map` function uses `iterations` as the loop bound directly (GLSL 330 supports dynamic loop bounds).

4. `main()` function:
   - UV: `vec2 uv = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0);` (centered, aspect-correct)
   - Bump mapping: three `map()` calls with finite-difference offset `e = vec2(2.0 / resolution.y, 0.0)` - transcribe from Algorithm Stage 2
   - Phong lighting with fixed light direction - transcribe from Algorithm Stage 2
   - Gradient LUT + FFT coloring - transcribe from Algorithm Stage 3

**Verify**: Shader loads without errors at runtime.

---

#### Task 2.3: Integration - effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/viscera.h"` with the other effect includes (alphabetical).
2. Add `TRANSFORM_VISCERA_BLEND,` to the `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`.
3. Add `TRANSFORM_VISCERA_BLEND` to the `TransformOrderConfig::order` array initialization, before the closing brace.
4. Add `VisceraConfig viscera;` to the `EffectConfig` struct with a `// Viscera` comment.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Integration - post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/viscera.h"` with the other effect includes (alphabetical).
2. Add `VisceraEffect viscera;` to the `PostEffect` struct alongside other single-shader generator effects.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Integration - serialization & build

**Files**: `src/config/effect_serialization.cpp` (modify), `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. In `effect_serialization.cpp`:
   - Add `#include "effects/viscera.h"` with the other effect includes.
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VisceraConfig, VISCERA_CONFIG_FIELDS)` with the other per-config macros.
   - Add `X(viscera) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.
2. In `CMakeLists.txt`:
   - Add `src/effects/viscera.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles.

---

## Implementation Notes

Deviations from the initial plan made during implementation:

- **twistAngle**: Changed from 0-TWO_PI_F range to standard -ROTATION_OFFSET_MAX..ROTATION_OFFSET_MAX with ModulatableSliderAngleDeg. Default adjusted from 5.0 to -1.28 (equivalent angle wrapped into [-PI, PI]).
- **bumpDepth renamed to bumpStrength**: Inverted semantics so higher value = more pronounced 3D. Config stores strength (2.0-100.0, default 20.0), Setup computes `1.0 / bumpStrength` before sending to shader as `bumpDepth` uniform.
- **Height-to-LUT mapping**: Changed from `clamp(h * heightScale, 0.0, 1.0)` to `clamp(h * heightScale / 6.0 + 0.5, 0.0, 1.0)`. Centers the mapping so h=0 maps to gradient midpoint. The `/6.0` normalizes for the typical [-3, 3] peak-to-peak range of `map()`.
- **Lighting formula**: Both diffuse and specular terms are multiplied by `lit = baseBright + mag` so that baseBright=0 with no audio produces fully black output. Specular is not independent of the brightness floor.

## Final Verification

- [x] Build succeeds with no warnings
- [ ] Viscera appears in Generators > Texture section with "GEN" badge
- [ ] Enabling shows organic folded terrain with bump-mapped lighting
- [ ] FFT audio modulates brightness per-frequency across the surface
- [ ] Gradient LUT colors the height field
- [ ] Specular highlights pick up bright-end LUT color
- [ ] Radial pulsation waves expand from center
- [ ] All sliders respond in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
