# Dream Fractal

Raymarched IFS fractal generator — infinitely recursive carved-sphere tunnels with orbiting camera, turbulence-based gradient LUT coloring, and per-pixel FFT frequency mapping. Based on "Dream Within A Dream" by OldEclipse.

**Research**: `docs/research/dream_fractal.md`

## Design

### Types

```c
// dream_fractal.h

struct DreamFractalConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;    // Lowest mapped pitch (27.5-440)
  float maxFreq = 14000.0f;  // Highest mapped pitch (1000-16000)
  float gain = 2.0f;         // FFT sensitivity (0.1-10)
  float curve = 1.5f;        // Contrast exponent (0.1-3)
  float baseBright = 0.15f;  // Floor brightness (0-1)

  // Camera
  float orbitSpeed = 0.2f;   // Camera orbit rate rad/s (-2.0-2.0)
  float driftSpeed = 0.05f;  // Forward movement speed (0.0-0.5)

  // Fractal geometry
  int fractalIters = 8;      // Detail levels (3-12)
  float sphereRadius = 0.9f; // Carved sphere size (0.3-1.5)
  float scaleFactor = 3.0f;  // Per-iteration scale ratio (2.0-5.0)
  float twist = 0.6435f;     // Inter-level rotation angle rad (-PI-PI)

  // Color
  float colorScale = 10.0f;         // Turbulence spatial frequency (1.0-30.0)
  float turbulenceIntensity = 1.2f;  // Domain warp strength (0.1-3.0)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DREAM_FRACTAL_CONFIG_FIELDS                                            \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, orbitSpeed, driftSpeed,  \
      fractalIters, sphereRadius, scaleFactor, twist, colorScale,              \
      turbulenceIntensity, gradient, blendMode, blendIntensity

typedef struct DreamFractalEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float orbitPhase;  // Accumulated orbit angle
  float driftPhase;  // Accumulated forward drift
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int orbitPhaseLoc;
  int driftPhaseLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int fractalItersLoc;
  int sphereRadiusLoc;
  int scaleFactorLoc;
  int twistLoc;
  int colorScaleLoc;
  int turbulenceIntensityLoc;
  int gradientLUTLoc;
} DreamFractalEffect;
```

### Algorithm

#### Shader (`shaders/dream_fractal.fs`)

Attribution header (before `#version`):
```glsl
// Based on "Dream Within A Dream" by OldEclipse
// https://www.shadertoy.com/view/fclGWs
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, gradient LUT coloring, FFT frequency-band brightness
```

Uniforms:
```glsl
uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float orbitPhase;
uniform float driftPhase;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int fractalIters;
uniform float sphereRadius;
uniform float scaleFactor;
uniform float twist;
uniform float colorScale;
uniform float turbulenceIntensity;
```

Constants:
```glsl
#define MARCH_STEPS 70
#define TURBULENCE_PASSES 3
const int BAND_SAMPLES = 4;
```

Main function — apply substitution table from research:

```glsl
void main() {
    vec2 I = gl_FragCoord.xy;
    // Ray setup: normalize coords relative to resolution
    vec3 r = normalize(vec3(I + I, 0.0) - vec3(resolution.xy, resolution.y));

    // Rotate ray direction using accumulated orbit phase
    float co = cos(orbitPhase);
    float so = sin(orbitPhase);
    r.xz *= mat2(co, -so, so, co);

    vec3 p, q;
    float t = 0.0, l = 0.0, d, s;

    // Raymarching loop
    for (int i = 0; i < MARCH_STEPS; i++) {
        p = t * r;
        // Forward drift
        p.z -= driftPhase;
        // Store non-distorted position for color
        q = p * colorScale;

        // IFS: repeatedly carve spheres out of space
        d = 0.0;
        s = 1.0;
        float ct = cos(twist);
        float st = sin(twist);
        for (int j = 0; j < fractalIters; j++) {
            p *= scaleFactor;
            s *= scaleFactor;
            d = max(d, (sphereRadius - length(abs(mod(p - 1.0, 2.0) - 1.0))) / s);
            p.xz *= mat2(ct, st, -st, ct);
        }

        // Store distance at ~29% of march steps for outline ratio
        if (i == int(float(MARCH_STEPS) * 0.29)) {
            l = t;
        }

        t += d;
    }

    // Turbulence domain warping — 3 passes, 9 iterations each
    // Original: for(float n=-.2; n++<8.; q+=1.2*sin(q.zxy*n)/n)
    // n takes values 0.8, 1.8, 2.8, ..., 8.8
    for (int pass = 0; pass < TURBULENCE_PASSES; pass++) {
        float n = -0.2;
        for (int k = 0; k < 9; k++) {
            n += 1.0;
            q += turbulenceIntensity * sin(q.zxy * n) / n;
        }
    }

    // Gradient LUT coloring: turbulence scalar → gradient position
    float tColor = 0.5 + 0.5 * sin(length(q));
    vec3 color = texture(gradientLUT, vec2(tColor, 0.5)).rgb;

    // FFT: same tColor determines frequency band
    float freqLo = baseFreq * pow(maxFreq / baseFreq, tColor);
    float tHi = min(tColor + 1.0 / 8.0, 1.0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, tHi);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);

    float energy = 0.0;
    for (int sb = 0; sb < BAND_SAMPLES; sb++) {
        float bin = mix(binLo, binHi, (float(sb) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    // Apply FFT brightness and outline depth ratio
    color *= brightness;
    color *= l / t / 6.0;

    finalColor = vec4(color, 1.0);
}
```

#### Phase Accumulation (CPU)

In `DreamFractalEffectSetup()`:
```c
e->orbitPhase += cfg->orbitSpeed * deltaTime;
e->driftPhase += cfg->driftSpeed * deltaTime;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5–440 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000–16000 | 14000 | yes | Max Freq (Hz) |
| gain | float | 0.1–10 | 2.0 | yes | Gain |
| curve | float | 0.1–3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0–1.0 | 0.15 | yes | Base Bright |
| orbitSpeed | float | -2.0–2.0 | 0.2 | yes | Orbit Speed |
| driftSpeed | float | 0.0–0.5 | 0.05 | yes | Drift Speed |
| fractalIters | int | 3–12 | 8 | no | Iterations |
| sphereRadius | float | 0.3–1.5 | 0.9 | yes | Sphere Radius |
| scaleFactor | float | 2.0–5.0 | 3.0 | yes | Scale Factor |
| twist | float | -PI–PI | 0.6435 | yes | Twist |
| colorScale | float | 1.0–30.0 | 10.0 | yes | Color Scale |
| turbulenceIntensity | float | 0.1–3.0 | 1.2 | yes | Turbulence |
| blendIntensity | float | 0.0–5.0 | 1.0 | yes | Blend |

### Constants

- Enum: `TRANSFORM_DREAM_FRACTAL_BLEND`
- Display name: `"Dream Fractal"`
- Category: `"GEN"`, section 13 (Field)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Macro: `REGISTER_GENERATOR`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Effect header

**Files**: `src/effects/dream_fractal.h` (create)
**Creates**: `DreamFractalConfig`, `DreamFractalEffect` types, function declarations

**Do**: Create header with config struct, effect struct, and function declarations exactly as specified in the Design > Types section. Follow `nebula.h` structure: includes `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward-declare `ColorLUT`. Declare `Init(Effect*, const Config*)`, `Setup(Effect*, const Config*, float, const Texture2D&)`, `Uninit(Effect*)`, `RegisterParams(Config*)`.

**Verify**: `cmake.exe --build build` compiles (header-only, no link errors expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect implementation

**Files**: `src/effects/dream_fractal.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `nebula.cpp` as pattern. Implement:
- `DreamFractalEffectInit`: load `shaders/dream_fractal.fs`, cache all uniform locations, init `ColorLUT` from `cfg->gradient`. On failure, unload shader and return false.
- `DreamFractalEffectSetup`: accumulate `orbitPhase += orbitSpeed * dt` and `driftPhase += driftSpeed * dt`. Update `ColorLUT`. Bind all uniforms including `fftTexture` and `gradientLUT` texture.
- `DreamFractalEffectUninit`: unload shader, uninit ColorLUT.
- `DreamFractalRegisterParams`: register all modulatable params from the Parameters table.
- `SetupDreamFractal(PostEffect*)`: bridge function (non-static) calling `DreamFractalEffectSetup`.
- `SetupDreamFractalBlend(PostEffect*)`: bridge function (non-static) calling `BlendCompositorApply`.
- Colocated UI (`DrawDreamFractalParams`): Audio section (Base Freq, Max Freq, Gain, Contrast, Base Bright), Geometry section (Iterations as `SliderInt`, Sphere Radius, Scale Factor), Animation section (Orbit Speed with `ModulatableSliderSpeedDeg`, Drift Speed), Color section (Color Scale, Turbulence, Twist with `ModulatableSliderAngleDeg`).
- `STANDARD_GENERATOR_OUTPUT(dreamFractal)` macro.
- `REGISTER_GENERATOR(TRANSFORM_DREAM_FRACTAL_BLEND, DreamFractal, dreamFractal, "Dream Fractal", SetupDreamFractalBlend, SetupDreamFractal, 13, DrawDreamFractalParams, DrawOutput_dreamFractal)`.

Includes: `dream_fractal.h`, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

**Verify**: Compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/dream_fractal.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the shader exactly as specified in the Design > Algorithm > Shader section. Include the attribution header before `#version 330`. Declare all uniforms, constants, and the `main()` function verbatim from the plan's Algorithm section. Do NOT consult the research doc — the plan's Algorithm section is the single source of truth.

**Verify**: Shader file exists and has correct `#version 330` header.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (all modify)
**Depends on**: Wave 1 complete

**Do**: Four mechanical edits following the Nebula pattern:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/dream_fractal.h"` with other effect includes
   - Add `TRANSFORM_DREAM_FRACTAL_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
   - Add `TRANSFORM_DREAM_FRACTAL_BLEND` to `TransformOrderConfig::order` array (at end, before closing brace)
   - Add `DreamFractalConfig dreamFractal;` to `EffectConfig` struct

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/dream_fractal.h"` with other effect includes
   - Add `DreamFractalEffect dreamFractal;` to `PostEffect` struct

3. **`CMakeLists.txt`**:
   - Add `src/effects/dream_fractal.cpp` to `EFFECTS_SOURCES`

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/dream_fractal.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DreamFractalConfig, DREAM_FRACTAL_CONFIG_FIELDS)` with other config macros
   - Add `X(dreamFractal)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Implementation Notes

Deviations from the original plan during implementation:

- **`twist` removed**: Hardcoded to original value (cos=0.8, sin=0.6) in shader. Changing it at runtime was jarring and not meaningful.
- **`driftSpeed` bidirectional**: Range changed from 0.0-0.5 to -0.5-0.5.
- **`marchSteps` added**: Configurable uniform (30-120, default 70) instead of a compile-time constant. The `l` capture point scales proportionally at `marchSteps * 2 / 7`.
- **Per-axis gradient sampling**: Each turbulence pass samples the gradient from a different component of `q` (q.x, q.y, q.z) instead of `length(q)`. This produces directional color variation closer to the reference's per-channel `sin(q.xyzz)`.
- **3-pass gradient blending**: The gradient is sampled once per turbulence pass and summed, matching the reference's 3-pass color accumulation structure. Divisor is 3.0 (reference uses 6.0 because its color range is [0,6] vs our [0,3]).

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in Field generator section of UI
- [x] Effect shows "GEN" badge
- [x] Effect can be enabled and shows raymarched fractal
- [x] Camera orbits and drifts forward
- [x] Gradient palette changes affect coloring
- [x] FFT reactivity varies spatially (different colors respond to different frequencies)
- [x] All sliders modify effect in real-time
- [ ] Preset save/load preserves settings
