# Light Medley

Fly-through raymarcher rendering a warped crystalline lattice. A rigid repeating grid is distorted by coordinate swirling and depth-dependent twisting, producing organic tunnel-like structures. Inverse-distance glow accumulation lights surfaces — each march step maps to a frequency band, so depth encodes pitch: near surfaces react to bass, deep surfaces react to treble. The visual ranges from geometric and angular at low distortion to fluid and biological at high distortion.

**Research**: `docs/research/light_medley.md`

## Design

### Types

```c
// LightMedleyConfig — user-facing parameters
struct LightMedleyConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;      // Lowest mapped pitch in Hz (27.5-440.0)
  float maxFreq = 14000.0f;    // Highest mapped pitch in Hz (1000-16000)
  float gain = 2.0f;           // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;          // Contrast exponent on FFT magnitudes (0.1-3.0)
  float baseBright = 0.15f;    // Minimum glow when no audio present (0.0-1.0)

  // Geometry
  float swirlAmount = 3.0f;    // Coordinate displacement amplitude (0.0-10.0)
  float swirlRate = 0.3f;      // Spatial frequency of swirl displacement (0.1-2.0)
  float swirlTimeRate = 0.1f;  // How fast swirl pattern evolves (0.01-0.5)
  float twistRate = 0.1f;      // XY rotation rate with Z depth (0.01-0.5)
  float flySpeed = 1.0f;       // Forward motion speed through lattice (0.1-3.0)
  float slabFreq = 1.0f;       // Frequency of cosine slab planes (0.5-4.0)
  float latticeScale = 1.0f;   // Cell density of octahedral lattice (0.5-4.0)

  // Camera
  float panAmpX = 0.8f;        // Camera drift amplitude horizontal (0.0-2.0)
  float panAmpY = 0.7f;        // Camera drift amplitude vertical (0.0-2.0)
  float panFreqX = 0.6f;       // Camera drift frequency horizontal (0.1-2.0)
  float panFreqY = 0.3f;       // Camera drift frequency vertical (0.1-2.0)

  // Output
  float glowIntensity = 1.0f;  // Brightness of inverse-distance glow (0.1-5.0)
  float exposure = 1.0f;       // Tonemap exposure control (0.1-5.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define LIGHT_MEDLEY_CONFIG_FIELDS                                        \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, swirlAmount,       \
      swirlRate, swirlTimeRate, twistRate, flySpeed, slabFreq,            \
      latticeScale, panAmpX, panAmpY, panFreqX, panFreqY,                \
      glowIntensity, exposure, gradient, blendMode, blendIntensity

// LightMedleyEffect — runtime state + shader handles
typedef struct LightMedleyEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;           // Master time accumulator for fly-through
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int swirlAmountLoc;
  int swirlRateLoc;
  int swirlTimeRateLoc;
  int twistRateLoc;
  int flySpeedLoc;
  int slabFreqLoc;
  int latticeScaleLoc;
  int panAmpXLoc;
  int panAmpYLoc;
  int panFreqXLoc;
  int panFreqYLoc;
  int glowIntensityLoc;
  int exposureLoc;
  int gradientLUTLoc;
} LightMedleyEffect;
```

### Algorithm

The shader is a fullscreen raymarcher. Each fragment casts a ray forward through a warped crystalline lattice. The march loop accumulates color per step, where each step's color comes from the gradient LUT and its brightness from the FFT band at that step's frequency.

Build the algorithm by applying these substitutions to the reference code line by line:

**Reference → AudioJones substitutions:**

| Reference | Replace with | Notes |
|-----------|-------------|-------|
| `iTime` | `time` uniform | CPU-accumulated at `1.0 * deltaTime` (unscaled) |
| `iResolution` | `resolution` uniform | Standard |
| `1/s` + `vec3(1,2,6)/length(...)` | Drop both. Replace with `gradientColor * brightness / s * glowIntensity` | See FFT block below |
| Hardcoded `3.0` swirl amplitude | `swirlAmount` uniform | |
| Hardcoded `0.1` twist rate | `twistRate` uniform | |
| Hardcoded `0.1` swirl time rate | `swirlTimeRate` uniform | |
| Hardcoded `0.3` swirl spatial rate | `swirlRate` uniform | |
| Hardcoded `64` steps | `#define MAX_STEPS 64` | Fixed constant |
| Hardcoded camera pan `cos(t*vec2(.6,.3))*vec2(.8,.7)` | `cos(time * vec2(panFreqX, panFreqY)) * vec2(panAmpX, panAmpY)` | |
| `tanh(c*c/2e6)` tonemap | `1.0 - exp(-c * exposure)` | Filmic tonemap |

**Keep verbatim (do NOT modify):**
- `mat2(cos(twistRate * p.z + vec4(0, 33, 11, 0)))` — magic numbers 33 and 11 are integral to rotation matrix (33 mod 2π ≈ π/2, 11 mod 2π ≈ 3π/2)
- `max(cos(p.x * slabFreq), dot(abs(fract(p * latticeScale) - 0.5), vec3(0.25)))` — distance function combining cosine slabs and octahedral lattice
- `p.yzx` permutation in the swirl term
- `vec3(uv * d, d + time * flySpeed)` ray construction pattern — `flySpeed` applied in shader only, not in CPU time accumulation
- Initial march distance `d = 0.4`
- The `- 3.0` scene origin offset

**Complete shader main function (transcribed from reference with substitutions applied):**

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float time;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float swirlAmount;
uniform float swirlRate;
uniform float swirlTimeRate;
uniform float twistRate;
uniform float flySpeed;
uniform float slabFreq;
uniform float latticeScale;
uniform float panAmpX;
uniform float panAmpY;
uniform float panFreqX;
uniform float panFreqY;
uniform float glowIntensity;
uniform float exposure;

#define MAX_STEPS 64
#define BAND_SAMPLES 4

void main() {
    // UV: centered, aspect-corrected, with Lissajous camera pan
    vec2 uv = (gl_FragCoord.xy * 2.0 - resolution) / resolution.y;
    uv += cos(time * vec2(panFreqX, panFreqY)) * vec2(panAmpX, panAmpY);

    float d = 0.4;      // initial march distance
    vec3 c = vec3(0.0);  // color accumulator

    for (int i = 0; i < MAX_STEPS; i++) {
        // Build 3D sample point: XY from UV * depth, Z advances with depth + scaled time
        vec3 p = vec3(uv * d, d + time * flySpeed) - 3.0;

        // Coordinate swirl: each axis displaced by cosine of another axis (yzx permutation)
        p += cos(swirlTimeRate * time + p.yzx * swirlRate) * swirlAmount;

        // Z-dependent XY rotation (33 ≈ π/2 mod 2π, 11 ≈ 3π/2 mod 2π → proper rotation matrix)
        p.xy *= mat2(cos(twistRate * p.z + vec4(0, 33, 11, 0)));

        // Distance function: max of cosine slabs and octahedral lattice
        float s = max(cos(p.x * slabFreq),
                      dot(abs(fract(p * latticeScale) - 0.5), vec3(0.25)));
        d += s;

        // Per-step FFT band sampling
        float t0 = float(i) / float(MAX_STEPS);
        float t1 = float(i + 1) / float(MAX_STEPS);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float mag = 0.0;
        for (int b = 0; b < BAND_SAMPLES; b++) {
            float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0)
                mag += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        mag = pow(clamp(mag / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // Gradient color from march depth position
        vec3 color = texture(gradientLUT, vec2(t0, 0.5)).rgb;

        // Accumulate: gradient color × FFT brightness × inverse distance × glow
        c += color * brightness / s * glowIntensity;
    }

    // Filmic tonemap
    finalColor = vec4(1.0 - exp(-c * exposure), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5–440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000–16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1–10.0 | 2.0 | yes | Gain |
| curve | float | 0.1–3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0–1.0 | 0.15 | yes | Base Bright |
| swirlAmount | float | 0.0–10.0 | 3.0 | yes | Swirl Amount |
| swirlRate | float | 0.1–2.0 | 0.3 | yes | Swirl Rate |
| swirlTimeRate | float | 0.01–0.5 | 0.1 | yes | Swirl Speed |
| twistRate | float | 0.01–0.5 | 0.1 | yes | Twist Rate |
| flySpeed | float | 0.1–3.0 | 1.0 | yes | Fly Speed |
| slabFreq | float | 0.5–4.0 | 1.0 | yes | Slab Freq |
| latticeScale | float | 0.5–4.0 | 1.0 | yes | Lattice Scale |
| panAmpX | float | 0.0–2.0 | 0.8 | yes | Pan Amp X |
| panAmpY | float | 0.0–2.0 | 0.7 | yes | Pan Amp Y |
| panFreqX | float | 0.1–2.0 | 0.6 | yes | Pan Freq X |
| panFreqY | float | 0.1–2.0 | 0.3 | yes | Pan Freq Y |
| glowIntensity | float | 0.1–5.0 | 1.0 | yes | Glow Intensity |
| exposure | float | 0.1–5.0 | 1.0 | yes | Exposure |

### Constants

- Enum name: `TRANSFORM_LIGHT_MEDLEY_BLEND`
- Display name: `"Light Medley"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Category section index: 13 (Field)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/light_medley.h` (create)
**Creates**: `LightMedleyConfig`, `LightMedleyEffect` types, `LIGHT_MEDLEY_CONFIG_FIELDS` macro, function declarations

**Do**: Create the header file following the struct layouts from the Design section exactly. Follow `nebula.h` as the pattern — includes `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward-declare `ColorLUT`. Declare `LightMedleyEffectInit(LightMedleyEffect*, const LightMedleyConfig*)`, `LightMedleyEffectSetup(LightMedleyEffect*, const LightMedleyConfig*, float, Texture2D)`, `LightMedleyEffectUninit`, `LightMedleyConfigDefault`, `LightMedleyRegisterParams`. Top-of-file comment: "Light Medley: fly-through raymarcher rendering a warped crystalline lattice with FFT-reactive glow".

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Fragment shader

**Files**: `shaders/light_medley.fs` (create)

**Do**: Implement the Algorithm section from the Design. The complete shader code is provided verbatim — transcribe it exactly. Add attribution comment block before `#version 330`:
```
// Based on "Light Medley [353]" by diatribes (golfed by @bug)
// https://www.shadertoy.com/view/3cXBWB
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT-per-step coloring via gradient LUT
```

**Verify**: File exists, has `#version 330`, all 22 uniforms declared.

---

#### Task 2.2: Effect implementation

**Files**: `src/effects/light_medley.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `nebula.cpp` as the pattern. Structure:

1. **Includes**: Own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `<stddef.h>`

2. **`LightMedleyEffectInit`**: Load `shaders/light_medley.fs`, cache all 22 uniform locations, init `ColorLUT` from `cfg->gradient`, set `time = 0.0f`. On shader failure return false; on LUT failure unload shader then return false.

3. **`LightMedleyEffectSetup`**: Accumulate `time += deltaTime` (unscaled — `flySpeed` is applied in the shader's ray Z-term). Update gradient LUT. Bind all uniforms including `flySpeed`, `fftTexture`, and `gradientLUT` texture. Use `(float)AUDIO_SAMPLE_RATE` for sampleRate.

4. **`LightMedleyEffectUninit`**: Unload shader, uninit ColorLUT.

5. **`LightMedleyConfigDefault`**: Return `LightMedleyConfig{}`.

6. **`LightMedleyRegisterParams`**: Register all 18 modulatable params from the Parameters table. Use `"lightMedley.<fieldName>"` IDs.

7. **`SetupLightMedley` (non-static bridge)**: Delegates to `LightMedleyEffectSetup(&pe->lightMedley, &pe->effects.lightMedley, pe->currentDeltaTime, pe->fftTexture)`.

8. **`SetupLightMedleyBlend` (non-static bridge)**: Calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.lightMedley.blendIntensity, pe->effects.lightMedley.blendMode)`.

9. **UI section** (`// === UI ===`): `static void DrawLightMedleyParams(EffectConfig*, const ModSources*, ImU32)`. Sections:
   - **Audio**: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order/formats per conventions)
   - **Geometry**: Swirl Amount, Swirl Rate, Swirl Speed, Twist Rate, Fly Speed, Slab Freq, Lattice Scale
   - **Camera**: Pan Amp X, Pan Amp Y, Pan Freq X, Pan Freq Y
   - **Output**: Glow Intensity, Exposure

10. **Registration**: `STANDARD_GENERATOR_OUTPUT(lightMedley)` then `REGISTER_GENERATOR(TRANSFORM_LIGHT_MEDLEY_BLEND, LightMedley, lightMedley, "Light Medley", SetupLightMedleyBlend, SetupLightMedley, 13, DrawLightMedleyParams, DrawOutput_lightMedley)` wrapped in clang-format off/on.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Config registration and build integration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (all modify)

**Do**:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/light_medley.h"` with the other effect includes
   - Add `TRANSFORM_LIGHT_MEDLEY_BLEND,` before `TRANSFORM_ACCUM_COMPOSITE` in the enum
   - Add `LightMedleyConfig lightMedley;` to `EffectConfig` struct

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/light_medley.h"` with the other effect includes
   - Add `LightMedleyEffect lightMedley;` to `PostEffect` struct (near other generator effect members)

3. **`CMakeLists.txt`**:
   - Add `src/effects/light_medley.cpp` to `EFFECTS_SOURCES`

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/light_medley.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LightMedleyConfig, LIGHT_MEDLEY_CONFIG_FIELDS)`
   - Add `X(lightMedley) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Implementation Notes

Changes made during implementation that deviate from the original plan:

- **Camera pan → DualLissajousConfig**: Replaced 4 one-off pan params (panAmpX/Y, panFreqX/Y) with embedded `DualLissajousConfig lissajous`. CPU-side `DualLissajousUpdate()` computes offset, passed as `vec2 pan` uniform. Setup takes non-const config pointer (lissajous mutates phase).
- **Tonemap kept as tanh**: Plan specified `1.0 - exp(-c * exposure)` which blew out. Kept original `tanh(c*c/2e6)` instead — self-normalizing, no extra parameters needed.
- **Removed exposure parameter**: Single `glowIntensity` scales the accumulation; tanh handles the rest.
- **CPU-accumulated swirlPhase**: `swirlTimeRate * time` in shader caused jumps on slider change. Replaced with `swirlPhase` accumulated on CPU (`swirlPhase += swirlTimeRate * deltaTime`), passed as uniform.
- **CPU-accumulated flyPhase**: Same issue — `flySpeed * time` replaced with `flyPhase` accumulated on CPU (`flyPhase += flySpeed * deltaTime`). Removed `time` and `flySpeed` uniforms from shader.
- **Twist uses degree slider**: `twistRate` is a rotation angle, uses `ModulatableSliderAngleDeg()` with `ROTATION_OFFSET_MAX` bounds.
- **Distance clamped**: `max(s, 0.01)` before inverse-distance divide to prevent infinity at lattice seams.
- **UI label cleanup**: "Swirl Rate" → "Swirl Freq" (spatial), "Twist Rate" → "Twist" (angular).

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Light Medley appears in Generators > Field section of UI
- [ ] Effect shows "GEN" category badge
- [ ] Effect can be enabled and renders the crystalline lattice
- [ ] Audio reactivity: bass frequencies light near surfaces, treble lights deep surfaces
- [ ] All 18 sliders respond in real-time
- [ ] Camera pan drifts smoothly with Lissajous pattern
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
