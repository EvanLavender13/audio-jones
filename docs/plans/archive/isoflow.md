# Isoflow

Raymarched gyroid isosurface tunnel with turbulence warping, iso-line depth rings,
depth-mapped FFT audio reactivity, and lissajous camera drift. The viewer flies
through a domain-warped gyroid surface where distance maps to both gradient color
and FFT frequency band, producing organic flowing corridors that pulse with audio.

**Research**: `docs/research/isoflow.md`

## Design

### Types

```c
// src/effects/isoflow.h

struct IsoflowConfig {
  bool enabled = false;

  // Geometry
  float gyroidScale = 132.0f;     // Primary gyroid cell size (20.0-200.0)
  float surfaceThickness = 32.0f; // Surface wall thickness multiplier (1.0-64.0)
  int marchSteps = 96;            // Raymarch iterations (32-128)
  float turbulenceScale = 16.0f;  // Turbulence warp wavelength (4.0-64.0)
  float turbulenceAmount = 16.0f; // Turbulence warp amplitude (0.0-32.0)
  float isoFreq = 0.7f;           // Iso-line ring frequency along z (0.1-2.0)
  float isoStrength = 1.0f;       // Iso-line effect blend (0.0-1.0)
  float baseOffset = 100.0f;      // Lateral offset to avoid gyroid origin (50.0-200.0)
  float tonemapGain = 0.00033f;   // Brightness before tanh (0.0001-0.002)

  // Speed (CPU-accumulated)
  float flySpeed = 1.0f;          // Z-axis fly-through speed (0.1-5.0)

  // Camera
  float camDrift = 0.3f;          // UV-space camera drift from lissajous (0.0-1.0)
  DualLissajousConfig lissajous = {
      .amplitude = 0.8f, .motionSpeed = 1.0f, .freqX1 = 0.6f, .freqY1 = 0.3f};

  // Audio
  float baseFreq = 55.0f;         // FFT low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f;       // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;              // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;             // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f;       // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define ISOFLOW_CONFIG_FIELDS                                                  \
  enabled, gyroidScale, surfaceThickness, marchSteps, turbulenceScale,         \
      turbulenceAmount, isoFreq, isoStrength, baseOffset, tonemapGain,         \
      flySpeed, camDrift, lissajous, baseFreq, maxFreq, gain, curve,           \
      baseBright, gradient, blendMode, blendIntensity

typedef struct IsoflowEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int flyPhaseLoc;
  int marchStepsLoc;
  int gyroidScaleLoc;
  int surfaceThicknessLoc;
  int turbulenceScaleLoc;
  int turbulenceAmountLoc;
  int isoFreqLoc;
  int isoStrengthLoc;
  int baseOffsetLoc;
  int tonemapGainLoc;
  int camDriftLoc;
  int panLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} IsoflowEffect;
```

### Algorithm

The shader is a direct transcription of the reference with the substitutions
from the research doc's Keep/Replace table applied mechanically.

```glsl
// shaders/isoflow.fs

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float flyPhase;       // CPU-accumulated: flyPhase += flySpeed * dt
uniform int marchSteps;       // default 96
uniform float gyroidScale;    // default 132.0 (A in SDF)
uniform float surfaceThickness; // default 32.0
uniform float turbulenceScale;  // default 16.0
uniform float turbulenceAmount; // default 16.0
uniform float isoFreq;        // default 0.7
uniform float isoStrength;    // default 1.0
uniform float baseOffset;     // default 100.0
uniform float tonemapGain;    // default 0.00033
uniform float camDrift;       // default 0.3
uniform vec2 pan;             // from DualLissajousUpdate
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;

// sampleFFTBand: same function as voxel_march.fs
float sampleFFTBand(float freqT0, float freqT1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, freqT0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, freqT1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int b = 0; b < BAND_SAMPLES; b++) {
        float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    return baseBright + mag;
}

void main() {
    // Derive gyroid sub-scales from single gyroidScale (preserves 132:94:43 ratios)
    float gA = gyroidScale;
    float gB = gyroidScale * 0.71;
    float gC = gyroidScale * 0.33;

    float s;   // dist to gyroid
    float d = 0.0;  // total dist
    vec3 color = vec3(0.0);

    // UV setup: reference uses unnormalized rd for wider FOV
    vec2 uv = (fragTexCoord * resolution - 0.5 * resolution) / resolution.y;

    // Camera drift from lissajous
    uv += pan * camDrift;

    float bw = 1.0 / float(marchSteps);

    for (int i = 0; i < marchSteps; i++) {
        vec3 p = vec3(uv * d, d + flyPhase);

        // Iso-line warp on z
        float isoWarp = cos(p.z * isoFreq) + sin(p.z * isoFreq);
        p.z += isoWarp * isoStrength;

        // Lateral offset to avoid gyroid origin; lissajous provides drift
        p.x += baseOffset + pan.x * baseOffset * 0.1;

        // Turbulence warp (two octaves with 2:1 wavelength ratio)
        p += cos(p.yzx / turbulenceScale) * turbulenceAmount
           + sin(p.yzx / (turbulenceScale * 2.0)) * (turbulenceAmount * 0.5);

        // Gyroid SDF
        s = 0.005 + 0.8 * abs(surfaceThickness
            * dot(sin(p / gA), cos(p.yzx / gB) + sin(p.yzx / gC)));

        // Depth-mapped FFT and gradient color (index by iteration)
        float t = float(i) / float(marchSteps);
        float brightness = sampleFFTBand(t, t + bw);
        vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;

        color += col / s * brightness;

        // March forward
        d += s;
    }

    // tanh tonemapping
    finalColor = vec4(tanh(color * tonemapGain), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| gyroidScale | float | 20.0-200.0 | 132.0 | yes | "Gyroid Scale" |
| surfaceThickness | float | 1.0-64.0 | 32.0 | yes | "Thickness" |
| marchSteps | int | 32-128 | 96 | no | "March Steps" |
| turbulenceScale | float | 4.0-64.0 | 16.0 | yes | "Turb Scale" |
| turbulenceAmount | float | 0.0-32.0 | 16.0 | yes | "Turb Amount" |
| isoFreq | float | 0.1-2.0 | 0.7 | yes | "Iso Freq" |
| isoStrength | float | 0.0-1.0 | 1.0 | yes | "Iso Strength" |
| baseOffset | float | 50.0-200.0 | 100.0 | yes | "Base Offset" |
| tonemapGain | float | 0.0001-0.002 | 0.00033 | yes | "Tonemap Gain" |
| flySpeed | float | 0.1-5.0 | 1.0 | yes | "Fly Speed" |
| camDrift | float | 0.0-1.0 | 0.3 | yes | "Cam Drift" |
| baseFreq | float | 27.5-440.0 | 55.0 | yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000.0 | yes | "Max Freq (Hz)" |
| gain | float | 0.1-10.0 | 2.0 | yes | "Gain" |
| curve | float | 0.1-3.0 | 1.5 | yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.15 | yes | "Base Bright" |

### Constants

- Enum name: `TRANSFORM_ISOFLOW_BLEND`
- Display name: `"Isoflow"`
- Category: GEN, section 13 (Field)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Macro: `REGISTER_GENERATOR` (Init takes `(Effect*, Config*)`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create isoflow config header

**Files**: `src/effects/isoflow.h` (create)
**Creates**: `IsoflowConfig`, `IsoflowEffect`, `ISOFLOW_CONFIG_FIELDS` macro, function declarations

**Do**: Create `src/effects/isoflow.h` following the struct layouts in the Design section above. Include `config/dual_lissajous_config.h`, `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward-declare `ColorLUT`. Declare: `IsoflowEffectInit(IsoflowEffect*, const IsoflowConfig*)`, `IsoflowEffectSetup(IsoflowEffect*, IsoflowConfig*, float, const Texture2D&)`, `IsoflowEffectUninit(IsoflowEffect*)`, `IsoflowRegisterParams(IsoflowConfig*)`. Follow `voxel_march.h` as the structural template.

**Verify**: `cmake.exe --build build` compiles (header-only, no link yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create isoflow shader

**Files**: `shaders/isoflow.fs` (create)
**Depends on**: Wave 1 complete (for uniform name alignment)

**Do**: Create `shaders/isoflow.fs`. Begin with attribution block (Based on "Kuantum Dalgalanmalari" by msttezcan, https://www.shadertoy.com/view/7fj3DK, CC BY-NC-SA 3.0). Implement the Algorithm section from the Design above. Use `#version 330`. Declare all uniforms matching the Design's uniform list. Include the `sampleFFTBand` function. The `main()` function is specified in full in the Algorithm section - transcribe it.

**Verify**: File exists and has correct `#version 330` header.

---

#### Task 2.2: Create isoflow implementation

**Files**: `src/effects/isoflow.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create `src/effects/isoflow.cpp` following `voxel_march.cpp` as the structural template. Includes: `isoflow.h`, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

Implement:
- `IsoflowEffectInit`: Load `shaders/isoflow.fs`, cache all uniform locations matching the Design's `IsoflowEffect` loc fields, init `ColorLUT`, set `flyPhase = 0.0f`. Return false on failure with cleanup cascade.
- `IsoflowEffectSetup`: Accumulate `flyPhase += cfg->flySpeed * deltaTime`. Call `ColorLUTUpdate`. Bind resolution, fftTexture, sampleRate, flyPhase, all config uniforms, pan from `DualLissajousUpdate`, gradientLUT texture. Follow `voxel_march.cpp` BindUniforms pattern.
- `IsoflowEffectUninit`: Unload shader, `ColorLUTUninit`.
- `IsoflowRegisterParams`: Register all modulatable params from the Parameters table. Use `"isoflow."` prefix. Include `lissajous.amplitude`, `lissajous.motionSpeed`, `blendIntensity`.

UI section (`// === UI ===`):
- `static void DrawIsoflowParams(EffectConfig*, const ModSources*, ImU32)`:
  - Audio section: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order)
  - Geometry section: March Steps (`SliderInt`), Gyroid Scale, Thickness, Turb Scale, Turb Amount, Iso Freq, Iso Strength, Base Offset, Tonemap Gain (`ModulatableSliderLog`)
  - Motion section: Fly Speed, Cam Drift
  - Camera section: `DrawLissajousControls` with tag `"iso_cam"`, prefix `"isoflow.lissajous"`, maxAmplitude `2.0f`

Bridge functions:
- `SetupIsoflow(PostEffect*)`: delegates to `IsoflowEffectSetup`
- `SetupIsoflowBlend(PostEffect*)`: calls `BlendCompositorApply`

Registration:
```
STANDARD_GENERATOR_OUTPUT(isoflow)
REGISTER_GENERATOR(TRANSFORM_ISOFLOW_BLEND, Isoflow, isoflow, "Isoflow",
                   SetupIsoflowBlend, SetupIsoflow, 13, DrawIsoflowParams, DrawOutput_isoflow)
```

**Verify**: `cmake.exe --build build` compiles (after Wave 3 integrations).

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/isoflow.h"` in the include block (alphabetical)
2. Add `TRANSFORM_ISOFLOW_BLEND,` before `TRANSFORM_ACCUM_COMPOSITE` in the enum
3. Add `IsoflowConfig isoflow;` member in `EffectConfig` struct (after `voxelMarch` or nearby generators, with comment `// Isoflow (turbulence-warped gyroid tunnel generator)`)

**Verify**: Header parses without errors.

---

#### Task 2.4: PostEffect struct integration

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/isoflow.h"` in the include block
2. Add `IsoflowEffect isoflow;` member in the `PostEffect` struct (near other generator effects)

**Verify**: Header parses without errors.

---

#### Task 2.5: Build system and serialization

**Files**: `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. `CMakeLists.txt`: Add `src/effects/isoflow.cpp` to `EFFECTS_SOURCES` (alphabetical)
2. `src/config/effect_serialization.cpp`:
   - Add `#include "effects/isoflow.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IsoflowConfig, ISOFLOW_CONFIG_FIELDS)`
   - Add `X(isoflow) \` to `EFFECT_CONFIG_FIELDS` X-macro table

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Field (13) generator section
- [ ] Gyroid tunnel renders with depth-mapped FFT audio reactivity
- [ ] Gradient LUT colors the tunnel by depth
- [ ] Lissajous camera drift produces smooth organic motion
- [ ] Turbulence controls warp the gyroid from rigid to flowing
- [ ] Iso-line rings visible and respond to isoFreq/isoStrength
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes work for registered params
