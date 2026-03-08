# Vortex

Raymarched distorted hollow sphere producing flowing volumetric spirals — smooth, swoopy ribbons of color ranging from languid smoke curls to active whirlpool spin. Same generator pipeline as Muons: ping-pong trail buffer with decay/blur + blend compositor output. Simpler than Muons (no mode variants, no Rodrigues rotation), but introduces geometric frequency scaling in the turbulence loop (`d /= growth` instead of linear `d++`).

**Research**: `docs/research/vortex.md`

## Design

### Types

```c
// src/effects/vortex.h

struct VortexConfig {
  bool enabled = false;

  // Raymarching
  int marchSteps = 50;            // Ray budget (4-200)
  int turbulenceOctaves = 8;      // Distortion layers (2-12)
  float turbulenceGrowth = 0.8f;  // Octave frequency growth (0.5-0.95)
  float sphereRadius = 0.5f;      // Seed shape size (0.1-3.0)
  float surfaceDetail = 40.0f;    // Step precision near surface (5.0-100.0)
  float cameraDistance = 6.0f;    // Depth into volume (3.0-20.0)

  // Trail persistence
  float decayHalfLife = 2.0f;     // Trail persistence seconds (0.1-10.0)
  float trailBlur = 1.0f;         // Trail blur amount (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;        // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f;      // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;             // FFT sensitivity multiplier (0.1-10.0)
  float curve = 1.5f;            // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.15f;      // Minimum brightness floor when silent (0.0-1.0)

  // Color
  float colorSpeed = 0.5f;       // LUT scroll rate over time (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  float colorStretch = 1.0f;     // Spatial color frequency through depth (0.1-5.0)

  // Tonemap
  float brightness = 1.0f;       // Intensity multiplier before tonemap (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;   // (0.0-5.0)
};

typedef struct VortexEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float time;
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceGrowthLoc;
  int sphereRadiusLoc;
  int surfaceDetailLoc;
  int cameraDistanceLoc;
  int colorSpeedLoc;
  int colorStretchLoc;
  int brightnessLoc;
  int gradientLUTLoc;
  int previousFrameLoc;
  int decayFactorLoc;
  int trailBlurLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} VortexEffect;
```

### Algorithm

Line-by-line mechanical translation of the reference code with the substitution table from research applied. Comments reference the original line they derive from.

```glsl
// === Shader: shaders/vortex.fs ===
// Attribution block at top (before #version), per add-effect checklist.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

// Same uniform set as Muons minus mode/turbulenceMode/phase/drift/axisFeedback/colorMode/ringThickness
uniform vec2 resolution;
uniform float time;
uniform int marchSteps;           // original: 1e2
uniform int turbulenceOctaves;    // original: d<9 loop bound
uniform float turbulenceGrowth;   // original: 0.8
uniform float sphereRadius;       // original: 0.5
uniform float surfaceDetail;      // original: 4e1
uniform float cameraDistance;     // original: 6.
uniform float colorSpeed;
uniform float colorStretch;
uniform float brightness;         // folds original /7e3 divisor
uniform sampler2D gradientLUT;
uniform sampler2D previousFrame;
uniform float decayFactor;
uniform float trailBlur;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

void main() {
    // Screen coords — verbatim from reference: (I+I - res.xyy) normalized
    vec2 fragCoord = fragTexCoord * resolution;
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    // Dithered ray start — verbatim from reference for banding suppression
    // Original: z = fract(dot(I,sin(I)))
    float z = fract(dot(fragCoord, sin(fragCoord)));
    float d;
    vec3 color = vec3(0.0);
    float stepCount = float(max(marchSteps - 1, 1));

    for (int i = 0; i < marchSteps; i++) {
        // Raymarch sample position — verbatim from reference
        // Original: vec3 p = z * normalize(vec3(I+I,0) - iResolution.xyy);
        vec3 p = z * rayDir;
        // Original: p.z += 6.  ->  p.z += cameraDistance
        p.z += cameraDistance;

        // Turbulence loop — geometric frequency scaling
        // Original: for(d=1.; d<9.; d/=.8) p += cos(p.yzx*d-iTime)/d;
        // Parameterized: loop turbulenceOctaves times, d /= turbulenceGrowth
        d = 1.0;
        for (int j = 0; j < turbulenceOctaves; j++) {
            p += cos(p.yzx * d - time) / d;
            d /= turbulenceGrowth;
        }

        // Hollow sphere distance in warped space
        // Original: z += d = .002+abs(length(p)-.5)/4e1
        // Parameterized: sphereRadius replaces 0.5, surfaceDetail replaces 4e1
        d = 0.002 + abs(length(p) - sphereRadius) / surfaceDetail;
        z += d;

        // Color: gradient LUT replaces sin(z+vec4(6,2,4,0))+1.5
        float lutCoord = fract(z * colorStretch + time * colorSpeed);
        vec3 stepColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

        // Per-step FFT — map march step to frequency band in log space
        // Same pattern as Muons additive volume mode
        float t0 = float(i) / stepCount;
        float t1 = float(i + 1) / stepCount;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);
        const int BAND_SAMPLES = 4;
        float energy = 0.0;
        for (int bs = 0; bs < BAND_SAMPLES; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float audio = baseBright + energy;

        // Additive accumulation — glow inversely proportional to step distance
        // Original: O += (coloring) / d
        color += stepColor * audio / d;
    }

    // Tanh tonemapping — original: O = tanh(O/7e3), brightness folds into divisor
    color = tanh(color * brightness / 7000.0);

    // Trail buffer with controllable blur — verbatim from Muons
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec3 raw = texelFetch(previousFrame, coord, 0).rgb;
    vec3 blurred  = 0.25   * raw;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2(-1, 0), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 1, 0), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 0,-1), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 0, 1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2(-1,-1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2( 1,-1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2(-1, 1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2( 1, 1), 0).rgb;
    vec3 prev = mix(raw, blurred, trailBlur);
    if (any(isnan(prev))) prev = vec3(0.0);
    finalColor = vec4(max(color, prev * decayFactor), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| marchSteps | int | 4-200 | 50 | No | March Steps |
| turbulenceOctaves | int | 2-12 | 8 | No | Octaves |
| turbulenceGrowth | float | 0.5-0.95 | 0.8 | Yes | Growth |
| sphereRadius | float | 0.1-3.0 | 0.5 | Yes | Sphere Radius |
| surfaceDetail | float | 5.0-100.0 | 40.0 | Yes | Surface Detail |
| cameraDistance | float | 3.0-20.0 | 6.0 | Yes | Camera Distance |
| decayHalfLife | float | 0.1-10.0 | 2.0 | Yes | Decay Half-Life |
| trailBlur | float | 0.0-1.0 | 1.0 | Yes | Trail Blur |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| colorSpeed | float | 0.0-2.0 | 0.5 | Yes | Color Speed |
| colorStretch | float | 0.1-5.0 | 1.0 | Yes | Color Stretch |
| brightness | float | 0.1-5.0 | 1.0 | Yes | Brightness |
| blendMode | EffectBlendMode | enum | SCREEN | No | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_VORTEX_BLEND`
- Display name: `"Vortex"`
- Category badge: `"GEN"`
- Section index: `11` (Filament)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR_FULL`)
- Macro: `REGISTER_GENERATOR_FULL`

### UI Sections

Colocated `DrawVortexParams` with sections in this order:
1. **Raymarching**: March Steps (SliderInt), Octaves (SliderInt), Growth, Sphere Radius, Surface Detail, Camera Distance
2. **Trails**: Decay Half-Life, Trail Blur
3. **Audio**: Base Freq (Hz), Max Freq (Hz), Gain, Contrast, Base Bright
4. **Color**: Color Speed, Color Stretch
5. **Tonemap**: Brightness

Output section via `STANDARD_GENERATOR_OUTPUT(vortex)`.

---

## Tasks

### Wave 1: Header

#### Task 1.1: VortexConfig and VortexEffect types

**Files**: `src/effects/vortex.h` (create)
**Creates**: Config struct, Effect struct, function declarations — required by all Wave 2 tasks

**Do**: Create header following the Types section above exactly. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`. Define `VORTEX_CONFIG_FIELDS` macro listing all serializable fields. Declare these functions:
- `VortexEffectInit(VortexEffect*, const VortexConfig*, int width, int height)` → bool
- `VortexEffectSetup(VortexEffect*, const VortexConfig*, float deltaTime, Texture2D fftTexture)` → void
- `VortexEffectRender(VortexEffect*, const VortexConfig*, float deltaTime, int screenWidth, int screenHeight)` → void
- `VortexEffectResize(VortexEffect*, int width, int height)` → void
- `VortexEffectUninit(VortexEffect*)` → void
- `VortexConfigDefault(void)` → VortexConfig
- `VortexRegisterParams(VortexConfig*)` → void

Pattern: follow `src/effects/muons.h` structure exactly.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker deps yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect implementation and UI

**Files**: `src/effects/vortex.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement all lifecycle functions following `src/effects/muons.cpp` as the exact pattern:

1. **Includes**: Same as Muons — own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `render/render_utils.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<math.h>`, `<stddef.h>`.

2. **Static helpers**: `InitPingPong`, `UnloadPingPong` — identical to Muons.

3. **Init**: Load `"shaders/vortex.fs"`, cache all uniform locations (see Effect struct), init gradient LUT from config, init ping-pong textures, clear both, set readIdx=0, time=0. Cascade cleanup on failure (same as Muons).

4. **Setup**: Advance `time += deltaTime`. Store fftTexture. Update gradient LUT. Bind all uniforms. Compute `decayFactor = expf(-0.693147f * deltaTime / safeHalfLife)` (same as Muons). Bind `sampleRate` from `AUDIO_SAMPLE_RATE`.

5. **Render**: Same as `MuonsEffectRender` — ping-pong with previousFrame texture binding, gradient LUT texture binding, FFT texture binding, fullscreen quad draw.

6. **Resize, Uninit, ConfigDefault, RegisterParams**: Same pattern as Muons. Register all float params listed as modulatable in the Parameters table.

7. **Bridge functions** (non-static):
   - `SetupVortex(PostEffect* pe)` — delegates to `VortexEffectSetup`
   - `SetupVortexBlend(PostEffect* pe)` — delegates to `BlendCompositorApply` with vortex's read ping-pong texture
   - `RenderVortex(PostEffect* pe)` — delegates to `VortexEffectRender`

8. **UI** (`// === UI ===` section): `static void DrawVortexParams(EffectConfig*, const ModSources*, ImU32)` with sections per UI Sections above. All float sliders use `ModulatableSlider`. Int sliders use `ImGui::SliderInt`. All IDs suffixed `##vortex`.

9. **Registration**: `STANDARD_GENERATOR_OUTPUT(vortex)` then `REGISTER_GENERATOR_FULL(TRANSFORM_VORTEX_BLEND, Vortex, vortex, "Vortex", SetupVortexBlend, SetupVortex, RenderVortex, 11, DrawVortexParams, DrawOutput_vortex)` wrapped in clang-format off/on.

**Verify**: Compiles with no warnings.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/vortex.fs` (create)
**Depends on**: Wave 1 complete (no compile dependency, but logically paired)

**Do**: Implement the Algorithm section above verbatim. The shader is fully specified there — transcribe it, do not modify the formulas.

Start with the attribution block before `#version`:
```
// Based on "Vortex [265]" by Xor
// https://www.shadertoy.com/view/wctXWN
// License: CC BY-NC-SA 3.0
// Modified: ported to GLSL 330; parameterized march steps, turbulence,
// sphere radius, surface detail, camera distance; replaced cos() coloring
// with gradient LUT; added FFT audio reactivity; added trail buffer with
// decay/blur; added brightness tonemap control.
```

**Verify**: Shader file exists and has correct uniform declarations.

---

#### Task 2.3: Integration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (all modify)
**Depends on**: Wave 1 complete

**Do** (4 files, 1-2 line edits each):

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/vortex.h"` with other effect includes (near `muons.h`)
   - Add `TRANSFORM_VORTEX_BLEND,` to `TransformEffectType` enum after `TRANSFORM_MUONS_BLEND`
   - Add `VortexConfig vortex;` to `EffectConfig` struct after the `MuonsConfig muons` member, with comment `// Vortex (raymarched distorted hollow sphere spirals with blend)`

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/vortex.h"` near `muons.h`
   - Add `VortexEffect vortex;` to `PostEffect` struct near `MuonsEffect muons`

3. **`CMakeLists.txt`**:
   - Add `src/effects/vortex.cpp` to `EFFECTS_SOURCES` near `muons.cpp`

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/vortex.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VortexConfig, VORTEX_CONFIG_FIELDS)` with other JSON macros
   - Add `X(vortex) \` to `EFFECT_CONFIG_FIELDS(X)` X-macro table near `muons`

**Verify**: `cmake.exe --build build` compiles with all files integrated.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Filament section of transform pipeline
- [ ] Effect shows "GEN" badge
- [ ] Enabling Vortex renders raymarched spirals into ping-pong buffer
- [ ] Trail buffer produces smooth persistence (adjust Decay Half-Life)
- [ ] FFT reactivity modulates glow per march step
- [ ] All sliders modify the effect in real-time
- [ ] Preset save/load preserves all Vortex settings
- [ ] Modulation routes to all registered parameters
