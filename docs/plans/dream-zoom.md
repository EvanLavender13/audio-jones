# Dream Zoom

Per-pixel iterated complex-map fractal with seamless infinite zoom. Log-polar coordinate transport carries a continuous radial zoom into a continuous translation through a Jacobi cn lattice; the cn function's double periodicity makes the wrap invisible. The fractal is generated procedurally per frame after the cn map, so depth never repeats and orbit-trap detail differs at every depth ring. FFT energy modulates depth-band brightness so bass illuminates the inner core and treble the outer rings.

**Research**: `docs/research/dream_zoom.md`

**Category**: Generator (Texture, section 12). Sibling to Byzantine, Infinity Matrix, Color Stretch, Subdivide.

**Attribution**: Inspired by "Dream Zoom" by NR4 (Alexander Kraus). Reference is GPL-3.0; this is a clean reimplementation from public-domain mathematical sources (DLMF Chapters 19, 22). The shader file MUST begin with an attribution comment block.

---

## Design

### Types

`src/effects/dream_zoom.h`:

```cpp
#ifndef DREAM_ZOOM_H
#define DREAM_ZOOM_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct DreamZoomConfig {
  bool enabled = false;

  // Variant - dropdown only, not modulatable
  int variant = 0; // 0 = DREAM, 1 = JACOBI

  // Animation (CPU-accumulated phases)
  float zoomSpeed = 0.1f;           // Continuous zoom rate (-1.0 to 1.0; +zoom out, -zoom in)
  float globalRotationSpeed = 0.1f; // UV plane rotation rate, rad/s
  float rotationSpeed = 0.0f;       // Inner fractal rotation rate, rad/s

  // Tiling
  float jacobiRepeats = 1.0f;       // cn-tile cycles per zoom revolution (0.5-4.0)

  // Iteration
  float formulaMix = 0.25f;         // exp <-> squaring blend (0-1)
  int   iterations = 64;            // Per-pixel iteration cap (16-256)

  // Coloring
  float cmapScale = 85.0f;          // LUT remap amplitude on trap minimum (1-200)
  float cmapOffset = 5.4f;          // LUT remap phase shift (0-10)

  // 2D positions (DualLissajous-driven)
  DualLissajousConfig trapOffset = {.amplitude = 0.05f}; // Orbit-trap point
  DualLissajousConfig origin = {.amplitude = 0.3f};       // Pre-iteration translation
  DualLissajousConfig constantOffset = {.amplitude = 0.03f}; // Per-iteration constant

  // Audio (FFT)
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DREAM_ZOOM_CONFIG_FIELDS                                               \
  enabled, variant, zoomSpeed, globalRotationSpeed, rotationSpeed,             \
      jacobiRepeats, formulaMix, iterations, cmapScale, cmapOffset, trapOffset,\
      origin, constantOffset, baseFreq, maxFreq, gain, curve, baseBright,      \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DreamZoomEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases
  float zoomPhase;           // += zoomSpeed * dt; wrapped to [-1024, 1024]
  float globalRotationPhase; // += globalRotationSpeed * dt; wrapped to [-PI, PI]
  float rotationPhase;       // += rotationSpeed * dt; wrapped to [-PI, PI]

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;

  int variantLoc;
  int zoomPhaseLoc;
  int globalRotationPhaseLoc;
  int rotationPhaseLoc;
  int jacobiRepeatsLoc;
  int formulaMixLoc;
  int iterationsLoc;
  int cmapScaleLoc;
  int cmapOffsetLoc;
  int trapOffsetLoc;     // vec2 from DualLissajousUpdate(&cfg->trapOffset, ...)
  int originLoc;         // vec2 from DualLissajousUpdate(&cfg->origin, ...)
  int constantOffsetLoc; // vec2 from DualLissajousUpdate(&cfg->constantOffset, ...)

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DreamZoomEffect;

bool DreamZoomEffectInit(DreamZoomEffect *e, const DreamZoomConfig *cfg);
void DreamZoomEffectSetup(DreamZoomEffect *e, DreamZoomConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);
void DreamZoomEffectUninit(DreamZoomEffect *e);
void DreamZoomRegisterParams(DreamZoomConfig *cfg);

DreamZoomEffect *GetDreamZoomEffect(PostEffect *pe);

#endif // DREAM_ZOOM_H
```

The Setup signature takes a non-const `DreamZoomConfig*` because `ColorLUTUpdate` mutates the LUT cache and `DualLissajousUpdate` mutates the embedded `phase` fields.

### Algorithm

The shader is a single fragment pass. The full GLSL is below; agents transcribe it verbatim into `shaders/dream_zoom.fs`.

#### Attribution header (REQUIRED)

```glsl
// Inspired by "Dream Zoom" by NR4 (Alexander Kraus)
// https://www.shadertoy.com/view/NfBXDG
// NR4's reference is GPL-3.0-or-later (incompatible with this project's
// CC BY-NC-SA 3.0 license). NO source code from NR4's shader is reproduced
// here. The technique - log-polar transport, Jacobi cn lattice tiling,
// complex-map iteration with orbit-trap coloring - is reimplemented from
// public-domain mathematical sources:
//   - DLMF Chapter 22 (Jacobian elliptic functions)
//   - DLMF 22.20.1 (descending Landen / AGM iteration)
//   - DLMF 22.6.1 (addition theorem for complex argument)
//   - DLMF 19.2.8 (complete elliptic integral K)
```

#### Full shader body

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform sampler2D gradientLUT;

uniform int   variant;              // 0 = DREAM, 1 = JACOBI
uniform float zoomPhase;            // CPU-accumulated, signed
uniform float globalRotationPhase;  // CPU-accumulated, [-PI, PI]
uniform float rotationPhase;        // CPU-accumulated, [-PI, PI]
uniform float jacobiRepeats;
uniform float formulaMix;
uniform int   iterations;
uniform float cmapScale;
uniform float cmapOffset;
uniform vec2  trapOffset;
uniform vec2  origin;
uniform vec2  constantOffset;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Calibration constants - MUST stay coupled (see Notes).
//   CN_PERIOD_CALIB = 2*K(0.5)/pi, with K the complete elliptic integral
//   of the first kind. Aligns one log-radius cycle to one real-period of
//   cn(z, 0.5). Sources: DLMF 19.2.8, Wikipedia "Jacobi elliptic functions".
const float CN_PERIOD_CALIB = 1.18034;
//   3.7 = integer multiple of cn period along the real axis at m = 0.5.
const float CN_WRAP_DISTANCE = 3.7;

// Jacobi sn, cn, dn for real argument u and parameter m = k^2.
// Descending Landen / AGM iteration. 4 iterations is sufficient for shader
// precision in our parameter range. Source: DLMF 22.20.1.
void jacobi_sncndn(float u, float m, out float sn, out float cn, out float dn) {
    const int N = 4;
    float emc = 1.0 - m;
    float a = 1.0;
    float em[4];
    float en[4];
    float c = 1.0;

    for (int i = 0; i < N; i++) {
        em[i] = a;
        emc = sqrt(emc);
        en[i] = emc;
        c = 0.5 * (a + emc);
        emc = a * emc;
        a = c;
    }

    u = c * u;
    sn = sin(u);
    cn = cos(u);
    dn = 1.0;

    if (sn != 0.0) {
        float ac = cn / sn;
        c = ac * c;
        for (int i = N - 1; i >= 0; i--) {
            float b = em[i];
            ac = c * ac;
            c = dn * c;
            dn = (en[i] + ac) / (b + ac);
            ac = c / b;
        }
        float a2 = inversesqrt(c * c + 1.0);
        sn = (sn < 0.0) ? -a2 : a2;
        cn = c * sn;
    }
}

// Complex Jacobi cn via the addition theorem. Source: DLMF 22.6.1.
//   cn(x + iy | m) = ( cn(x|m)*cn(y|1-m)
//                    - i sn(x|m)*dn(x|m)*sn(y|1-m)*dn(y|1-m) )
//                    / ( 1 - dn(x|m)^2 * sn(y|1-m)^2 )
vec2 cn_complex(vec2 z, float m) {
    float snu, cnu, dnu;
    float snv, cnv, dnv;
    jacobi_sncndn(z.x, m, snu, cnu, dnu);
    jacobi_sncndn(z.y, 1.0 - m, snv, cnv, dnv);
    float invDenom = 1.0 / (1.0 - dnu * dnu * snv * snv);
    return invDenom * vec2(cnu * cnv, -snu * dnu * snv * dnv);
}

void main() {
    // ----- 1. Per-pixel coordinate transport -----

    // Centered, isotropic UV in [-aspect, aspect] x [-1, 1].
    vec2 uv = (fragTexCoord - 0.5) * resolution / resolution.y;

    // Global rotation of the sampling plane (slow drift).
    float gphi = globalRotationPhase;
    mat2 grot = mat2(cos(gphi), -sin(gphi), sin(gphi), cos(gphi));
    uv = grot * uv;

    // Cartesian -> log-polar:  clog(z) = (log|z|, atan(z.y, z.x))
    vec2 z = vec2(log(length(uv) + 1e-20), atan(uv.y, uv.x));

    // Pre-scale to align one log-radius cycle with one cn period.
    z *= CN_PERIOD_CALIB * 0.5 * jacobiRepeats;

    // Continuous translation along real axis = continuous radial zoom.
    // mod() wrap is seamless because 3.7 is a cn period at m = 0.5.
    z.x -= mod(zoomPhase, 1.0) * CN_WRAP_DISTANCE;

    // 45-degree shear places the doubly-periodic cn lattice diagonally so
    // successive zoom steps don't alias into vertical bands.
    z = mat2(1.0, 1.0, -1.0, 1.0) * z;

    // Map through Jacobi cn at m = 0.5 (lemniscatic modulus).
    z = cn_complex(z, 0.5);

    // ----- 2. Variant pre-iteration affine -----

    float coordinateScale;
    vec2  preOffset;
    if (variant == 0) {        // VARIANT_DREAM (identity)
        coordinateScale = 1.0;
        preOffset = vec2(0.0);
    } else {                    // VARIANT_JACOBI
        coordinateScale = 1.063;
        preOffset = vec2(11.88, -23.33);
    }

    z -= preOffset * 0.001;
    z *= exp(mix(log(1e-4), log(1.0), coordinateScale));

    // ----- 3. Inner fractal rotation (rotationPhase) -----
    // Applied to z after cn-tiling and variant scale, before the iteration
    // loop. Symmetric with the global UV rotation in step 1.
    float rphi = rotationPhase;
    mat2 rrot = mat2(cos(rphi), -sin(rphi), sin(rphi), cos(rphi));
    z = rrot * z;

    // ----- 4. Per-pixel fractal iteration with orbit trap -----

    vec2 zf = z - 0.01 * origin;
    float tmin = 1e9;

    for (int i = 0; i < iterations; i++) {
        if (dot(zf, zf) > 1e10) { break; }

        // exp(zf) for complex zf, with x clamped to prevent overflow.
        //   exp(x + iy) = exp(x) * (cos(y), sin(y))
        vec2 expz = vec2(cos(zf.y), sin(zf.y)) * min(exp(zf.x), 2.0e4);

        // zf * zf for complex zf:  (a + bi)^2 = (a*a - b*b) + 2ab*i
        vec2 sqz = vec2(zf.x * zf.x - zf.y * zf.y, 2.0 * zf.x * zf.y);

        // f(z) = mix(exp(z), z*z, formulaMix) + 0.01*constantOffset + z
        zf = mix(expz, sqz, formulaMix) + 0.01 * constantOffset + zf;

        // Moebius-style orbit trap: |z / (z - trapOffset)|
        float t = length(zf / (zf - trapOffset));
        tmin = min(tmin, t * cmapScale * 0.01);
    }

    // ----- 5. Per-pixel depth coordinate -----
    // Equal-tmin pixels lie on the same depth ring of the fractal.
    float tLUT = fract(cmapOffset - log(mix(exp(0.001), exp(1.0), tmin)));

    // ----- 6. FFT-reactive depth modulation (BAND_SAMPLES standard) -----
    // tLUT drives both the gradient lookup AND the FFT band - depth rings
    // respond to specific frequency bands. Per memory/generator_patterns.md:
    // 4 adjacent bins around tLUT, log-spaced via baseFreq/maxFreq.
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float ts = tLUT + (float(s) + 0.5)
                          / float(BAND_SAMPLES)
                          / float(textureSize(fftTexture, 0).x);
        float freq = baseFreq * pow(maxFreq / baseFreq, ts);
        float bin = freq / (sampleRate * 0.5);
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + mag;

    vec3 col = texture(gradientLUT, vec2(tLUT, 0.5)).rgb * brightness;
    finalColor = vec4(col, 1.0);
}
```

#### CPU-side accumulation (in `DreamZoomEffectSetup`)

```cpp
e->zoomPhase += cfg->zoomSpeed * deltaTime;
// Keep float precision bounded; the shader does its own mod(zoomPhase, 1.0).
if (e->zoomPhase >  1024.0f) { e->zoomPhase -= 1024.0f; }
if (e->zoomPhase < -1024.0f) { e->zoomPhase += 1024.0f; }

e->globalRotationPhase += cfg->globalRotationSpeed * deltaTime;
e->globalRotationPhase = fmodf(e->globalRotationPhase + PI_F, 2.0f * PI_F) - PI_F;

e->rotationPhase += cfg->rotationSpeed * deltaTime;
e->rotationPhase = fmodf(e->rotationPhase + PI_F, 2.0f * PI_F) - PI_F;

float trapX, trapY, originX, originY, kX, kY;
DualLissajousUpdate(&cfg->trapOffset,      deltaTime, 0.0f, &trapX,   &trapY);
DualLissajousUpdate(&cfg->origin,          deltaTime, 0.0f, &originX, &originY);
DualLissajousUpdate(&cfg->constantOffset,  deltaTime, 0.0f, &kX,      &kY);
const float trapVec[2]   = {trapX,   trapY};
const float originVec[2] = {originX, originY};
const float kVec[2]      = {kX,      kY};
// SetShaderValue(... SHADER_UNIFORM_VEC2) for trapOffset, origin, constantOffset
```

The standard generator boilerplate (resolution, fftTexture, sampleRate, gradientLUT, ColorLUTUpdate) follows the exact pattern in `infinity_matrix.cpp::InfinityMatrixEffectSetup`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| variant | int (combo) | DREAM, JACOBI | DREAM (0) | No | "Variant" (Combo) |
| zoomSpeed | float | -1.0 .. 1.0 | 0.1 | Yes | "Zoom Speed" |
| globalRotationSpeed | float | -ROTATION_SPEED_MAX .. +ROTATION_SPEED_MAX | 0.1 | Yes | "Global Rotation" (SpeedDeg) |
| rotationSpeed | float | -ROTATION_SPEED_MAX .. +ROTATION_SPEED_MAX | 0.0 | Yes | "Inner Rotation" (SpeedDeg) |
| jacobiRepeats | float | 0.5 .. 4.0 | 1.0 | Yes | "Jacobi Repeats" |
| formulaMix | float | 0.0 .. 1.0 | 0.25 | Yes | "Formula Mix" |
| iterations | int | 16 .. 256 | 64 | No | "Iterations" (SliderInt) |
| cmapScale | float | 1.0 .. 200.0 | 85.0 | Yes | "Color Scale" |
| cmapOffset | float | 0.0 .. 10.0 | 5.4 | Yes | "Color Offset" |
| trapOffset | DualLissajousConfig | - | amp 0.05 | amp + motionSpeed | "Trap Offset" (DrawLissajousControls) |
| origin | DualLissajousConfig | - | amp 0.3 | amp + motionSpeed | "Origin" (DrawLissajousControls) |
| constantOffset | DualLissajousConfig | - | amp 0.03 | amp + motionSpeed | "Constant Offset" (DrawLissajousControls) |
| baseFreq | float | 27.5 .. 440.0 | 55.0 | Yes | "Base Freq (Hz)" |
| maxFreq | float | 1000 .. 16000 | 14000 | Yes | "Max Freq (Hz)" |
| gain | float | 0.1 .. 10.0 | 2.0 | Yes | "Gain" |
| curve | float | 0.1 .. 3.0 | 1.5 | Yes | "Contrast" |
| baseBright | float | 0.0 .. 1.0 | 0.15 | Yes | "Base Bright" |
| gradient | ColorConfig | - | gradient mode | (output section) | (ImGuiDrawColorMode) |
| blendMode | EffectBlendMode | enum | EFFECT_BLEND_SCREEN | No | (output section) |
| blendIntensity | float | 0.0 .. 5.0 | 1.0 | Yes | (output section) |

`ROTATION_SPEED_MAX = PI_F` from `src/config/constants.h`.

### Constants

- Enum value: `TRANSFORM_DREAM_ZOOM_BLEND` (added to `TransformEffectType` before `TRANSFORM_ACCUM_COMPOSITE`)
- Display name: `"Dream Zoom"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `12` (Texture)
- Field name on `EffectConfig`: `dreamZoom`
- Param prefix: `"dreamZoom."` (auto-set by `REGISTER_GENERATOR`)
- Variant identifiers (shader-side `int variant`): 0 = `VARIANT_DREAM`, 1 = `VARIANT_JACOBI` (no enum needed; combo strings declared inline as a `static const char *VARIANT_NAMES[] = {"Dream", "Jacobi"};`).

### Modulation registration (`DreamZoomRegisterParams`)

```cpp
ModEngineRegisterParam("dreamZoom.zoomSpeed",            &cfg->zoomSpeed,            -1.0f, 1.0f);
ModEngineRegisterParam("dreamZoom.globalRotationSpeed",  &cfg->globalRotationSpeed,  -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
ModEngineRegisterParam("dreamZoom.rotationSpeed",        &cfg->rotationSpeed,        -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
ModEngineRegisterParam("dreamZoom.jacobiRepeats",        &cfg->jacobiRepeats,        0.5f, 4.0f);
ModEngineRegisterParam("dreamZoom.formulaMix",           &cfg->formulaMix,           0.0f, 1.0f);
ModEngineRegisterParam("dreamZoom.cmapScale",            &cfg->cmapScale,            1.0f, 200.0f);
ModEngineRegisterParam("dreamZoom.cmapOffset",           &cfg->cmapOffset,           0.0f, 10.0f);
ModEngineRegisterParam("dreamZoom.trapOffset.amplitude",      &cfg->trapOffset.amplitude,      0.0f, 0.5f);
ModEngineRegisterParam("dreamZoom.trapOffset.motionSpeed",    &cfg->trapOffset.motionSpeed,    0.0f, 10.0f);
ModEngineRegisterParam("dreamZoom.origin.amplitude",          &cfg->origin.amplitude,          0.0f, 0.5f);
ModEngineRegisterParam("dreamZoom.origin.motionSpeed",        &cfg->origin.motionSpeed,        0.0f, 10.0f);
ModEngineRegisterParam("dreamZoom.constantOffset.amplitude",  &cfg->constantOffset.amplitude,  0.0f, 0.5f);
ModEngineRegisterParam("dreamZoom.constantOffset.motionSpeed",&cfg->constantOffset.motionSpeed,0.0f, 10.0f);
ModEngineRegisterParam("dreamZoom.baseFreq",             &cfg->baseFreq,             27.5f, 440.0f);
ModEngineRegisterParam("dreamZoom.maxFreq",              &cfg->maxFreq,              1000.0f, 16000.0f);
ModEngineRegisterParam("dreamZoom.gain",                 &cfg->gain,                 0.1f, 10.0f);
ModEngineRegisterParam("dreamZoom.curve",                &cfg->curve,                0.1f, 3.0f);
ModEngineRegisterParam("dreamZoom.baseBright",           &cfg->baseBright,           0.0f, 1.0f);
ModEngineRegisterParam("dreamZoom.blendIntensity",       &cfg->blendIntensity,       0.0f, 5.0f);
```

### UI layout (`DrawDreamZoomParams`)

Sections in this order, matching the Signal Stack vocabulary used by sibling generators:

1. **Audio** - SeparatorText. Sliders: Base Freq, Max Freq, Gain, Contrast, Base Bright (use the FFT Audio UI conventions: `ModulatableSlider` for `baseFreq` not `Log`; `%.1f` for baseFreq, `%.0f` for maxFreq, `%.1f` for gain, `%.2f` for curve and baseBright).
2. **Variant** - SeparatorText. `ImGui::Combo("Variant##dreamZoom", &cfg->variant, VARIANT_NAMES, 2)`.
3. **Iteration** - SeparatorText. `ImGui::SliderInt("Iterations##dreamZoom", &cfg->iterations, 16, 256)`. `ModulatableSlider` for `formulaMix`.
4. **Tiling** - SeparatorText. `ModulatableSlider` for `jacobiRepeats`.
5. **Coloring** - SeparatorText. `ModulatableSlider` for `cmapScale`, `cmapOffset`.
6. **Animation** - SeparatorText. `ModulatableSlider` for `zoomSpeed`. `ModulatableSliderSpeedDeg` for `globalRotationSpeed`, `rotationSpeed`.
7. **Trap Offset** - SeparatorText. `DrawLissajousControls(&cfg->trapOffset, "dreamZoom_trapOffset", "dreamZoom.trapOffset", ms, 5.0f)`.
8. **Origin** - SeparatorText. `DrawLissajousControls(&cfg->origin, "dreamZoom_origin", "dreamZoom.origin", ms, 5.0f)`.
9. **Constant Offset** - SeparatorText. `DrawLissajousControls(&cfg->constantOffset, "dreamZoom_constantOffset", "dreamZoom.constantOffset", ms, 5.0f)`.

The Output section (gradient + blend) is generated by `STANDARD_GENERATOR_OUTPUT(dreamZoom)` and rendered automatically by the dispatch system.

### Registration

Bottom of `dream_zoom.cpp`:

```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(dreamZoom)
REGISTER_GENERATOR(TRANSFORM_DREAM_ZOOM_BLEND, DreamZoom, dreamZoom, "Dream Zoom",
                   SetupDreamZoomBlend, SetupDreamZoom, 12,
                   DrawDreamZoomParams, DrawOutput_dreamZoom)
// clang-format on
```

Bridge functions just above the macro:

- `void SetupDreamZoom(PostEffect *pe)` - calls `DreamZoomEffectSetup(GetDreamZoomEffect(pe), &pe->effects.dreamZoom, pe->currentDeltaTime, pe->fftTexture)`.
- `void SetupDreamZoomBlend(PostEffect *pe)` - calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.dreamZoom.blendIntensity, pe->effects.dreamZoom.blendMode)`.

Both bridge functions are non-static (matched by the macro's forward declarations). `DrawDreamZoomParams` is `static`.

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create header

**Files**: `src/effects/dream_zoom.h`
**Creates**: `DreamZoomConfig`, `DreamZoomEffect`, lifecycle declarations, `DREAM_ZOOM_CONFIG_FIELDS` macro.

**Do**: Create the header from the Types section above verbatim. Pattern reference: `src/effects/infinity_matrix.h` (sibling Texture generator with embedded `ColorConfig` and FFT params). The struct field defaults, the field-list macro, and the function signatures must match the Types section exactly.

**Verify**: `cmake.exe --build build` compiles (header is not yet referenced).

---

### Wave 2: Parallel implementation

All four tasks depend on Wave 1 only. They touch disjoint files.

#### Task 2.1: Effect implementation + colocated UI

**Files**: `src/effects/dream_zoom.cpp`
**Depends on**: Wave 1 complete

**Do**: Implement Init, Setup, Uninit, RegisterParams, the colocated `DrawDreamZoomParams`, the two bridge functions (`SetupDreamZoom`, `SetupDreamZoomBlend`), and the registration macros. Pattern reference: `src/effects/infinity_matrix.cpp` for the overall structure (FFT generator with ColorLUT + multiple uniforms) and `src/effects/escher_droste.cpp` for the embedded `DualLissajousConfig` accumulation pattern (`DualLissajousUpdate` per-frame, `.amplitude`/`.motionSpeed` registered with the mod engine, `DrawLissajousControls` in the UI).

Specifics:
- Init: `LoadShader(NULL, "shaders/dream_zoom.fs")`, cache all uniform locations listed in `DreamZoomEffect`, `ColorLUTInit(&cfg->gradient)`. Cleanup-on-failure cascade (unload shader if LUT init fails). Initialize all three phases to 0.
- Setup: Accumulate the three phases per the CPU-side accumulation block in the Algorithm section. Wrap rotation phases via `fmodf(... + PI_F, 2.0f * PI_F) - PI_F`. Use `DualLissajousUpdate` for each of `trapOffset`, `origin`, `constantOffset` and bind the resulting vec2s. `ColorLUTUpdate` then bind LUT texture. Standard resolution + fftTexture + sampleRate (`AUDIO_SAMPLE_RATE` from `audio/audio.h`) + gradientLUT bindings follow `infinity_matrix.cpp` exactly. Bind `variant` as `SHADER_UNIFORM_INT`, `iterations` as `SHADER_UNIFORM_INT`. All other scalars `SHADER_UNIFORM_FLOAT`. The three Lissajous outputs as `SHADER_UNIFORM_VEC2`.
- Uninit: `UnloadShader(e->shader); ColorLUTUninit(e->gradientLUT);`
- RegisterParams: exact list in Design > Modulation registration above.
- UI: exact section order in Design > UI layout above. Declare `static const char *VARIANT_NAMES[] = {"Dream", "Jacobi"};` at file scope.
- Includes (per conventions, generator effect with colocated UI):
  - own header
  - `audio/audio.h` (for `AUDIO_SAMPLE_RATE`)
  - `automation/mod_sources.h`, `automation/modulation_engine.h`
  - `config/constants.h`, `config/dual_lissajous_config.h`, `config/effect_config.h`, `config/effect_descriptor.h`
  - `imgui.h`
  - `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`
  - `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`
  - `<math.h>`, `<stddef.h>`

**Verify**: `cmake.exe --build build` compiles. With Task 2.3 also done the effect appears in the pipeline.

#### Task 2.2: Fragment shader

**Files**: `shaders/dream_zoom.fs`
**Depends on**: Wave 1 complete

**Do**: Create the file with the attribution header block followed by the full GLSL body from the Algorithm section. Transcribe verbatim - do NOT reinterpret or substitute. The attribution comment block must precede `#version 330`.

Key invariants to preserve:
- `CN_PERIOD_CALIB = 1.18034` and `CN_WRAP_DISTANCE = 3.7` are coupled - do not change either independently.
- The 45-degree shear matrix `mat2(1.0, 1.0, -1.0, 1.0)` is applied AFTER the cn pre-scale and BEFORE `cn_complex`.
- The `min(exp(zf.x), 2.0e4)` clamp inside the iteration loop is required to prevent NaN cascades.
- The iteration loop uses `for (int i = 0; i < iterations; i++)` with the uniform int directly per `memory/feedback_glsl_loops.md` - no hardcoded max + break.
- `fragTexCoord` is centered via `(fragTexCoord - 0.5) * resolution / resolution.y` per coordinate centering convention - NEVER use raw `fragTexCoord` for spatial math.
- No tonemap on the final color; brightness multiplies the LUT sample directly.
- No debug overlay.
- The FFT block is the BAND_SAMPLES standard from `memory/generator_patterns.md` - 4 adjacent bins around `tLUT`, `freq = baseFreq * pow(maxFreq/baseFreq, ts)`, `bin = freq / (sampleRate * 0.5)`.

**Verify**: `cmake.exe --build build` compiles. Shader loads at runtime without GL errors (run the binary and check stdout).

#### Task 2.3: Effect config integration

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**: Three edits in this file.
1. Add `#include "effects/dream_zoom.h"` alphabetically with the other effect includes near the top (after `dream_fractal.h`, before `drekker_paint.h` if either exists; otherwise alphabetical).
2. Add `TRANSFORM_DREAM_ZOOM_BLEND,` to the `TransformEffectType` enum just before `TRANSFORM_ACCUM_COMPOSITE`.
3. Add `DreamZoomConfig dreamZoom;` as a member of `EffectConfig` near the other Texture sub-category configs (next to `InfinityMatrixConfig infinityMatrix;` around line 861).

The default `TransformOrderConfig` constructor auto-populates `order[]` by enum index, so no manual array edit is needed.

**Verify**: `cmake.exe --build build` compiles after Task 2.1 and 2.2 also complete.

#### Task 2.4: Preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**: Three edits.
1. Add `#include "effects/dream_zoom.h"` alphabetically (after `dream_fractal.h`, before `drekker_paint.h`).
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DreamZoomConfig, DREAM_ZOOM_CONFIG_FIELDS)` alphabetically in the macro block (around line 396 next to `DreamFractalConfig`).
3. Add `X(dreamZoom) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro block. Place it next to or near `X(infinityMatrix)` since both are Texture-category generators.

**Verify**: `cmake.exe --build build` compiles. Save a preset, load it back, confirm Dream Zoom settings round-trip.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] "Dream Zoom" appears in the Texture section of the effects panel with the "GEN" badge
- [ ] Enabling the effect adds it to the transform pipeline and renders
- [ ] Variant combo switches between Dream and Jacobi visuals (Jacobi shows different scale/offset)
- [ ] Zoom is visually seamless - no flash or seam at the cycle boundary
- [ ] Bass-heavy audio brightens inner depth rings; treble brightens outer rings
- [ ] All Lissajous controls (trapOffset, origin, constantOffset) modulate the visual continuously without breaking the zoom
- [ ] Modulating `cmapOffset` cycles the gradient through depth rings cleanly
- [ ] All registered modulation params route correctly (test by binding an LFO to each in turn)
- [ ] Preset save/load round-trips all fields including the three embedded `DualLissajousConfig`s
- [ ] No NaN regions appear at high `formulaMix` or `iterations` values

---

## Implementation Notes

The shipped effect deviates from this plan in several material ways. The plan was based on a research doc that was wrong about multiple points; the shipped code follows NR4's actual reference source instead.

**Variant dropped.** Dream-only. The plan's `VARIANT_DREAM` / `VARIANT_JACOBI` toggle was based on a research claim that the only difference between NR4's two source shaders was `iCoordinateScale` and `iOffset`. The actual references differ in 10+ params (cmapScale, cmapOffset, trapOffset, origin, constantMapOffset, formulaMix, jacobiRepeats, globalRotationSpeed, spiral wrap divisor, iter cap, plus a `z += 0.3*(cos(.4t),sin(.5t))` translation in Jacobi's spiralize). A working two-variant effect would either have to snap N other params on a dropdown change (rejected on UX grounds) or be split into two separate effect entries (rejected on scope). Single Dream effect ships with the reference's actual Dream constants.

**Lissajous embeddings dropped.** The plan used `DualLissajousConfig` for `trapOffset`, `origin`, `constantOffset` per `feedback_lissajous_for_2d_positions.md`. That convention is wrong for orbit-trap fractals: the trap point determines the entire fractal structure, so animating it dissolves the coherent depth bands; and with `amplitude=0` the Lissajous parks at exactly `(0,0)`, which is the singularity in `length(z / (z - trapOffset))`. Replaced with plain `float trapOffsetX/Y, originX/Y, constantOffsetX/Y` defaulting to NR4's static reference values.

**`formula()` body fixed.** The research's per-iteration map used `vec2(cos(zf.y), sin(zf.y)) * min(exp(zf.x), 2e4)` for the exp branch. The reference actually uses `vec2(cos(mod(abs(z.y), 2*pi)), sign(z.y) * sin(mod(abs(z.y), 2*pi))) * min(exp(z.x), 2e4)` — the `mod(abs(z.y))` / `sign(z.y)` folding of the imaginary axis is load-bearing. Without it the fractal looks structurally different.

**`iOffset * 0.001` UV pre-shift added.** The research omitted the per-pixel UV shift `uv -= iOffset * 0.001` that runs inside the reference's `pixel()` function. Without it the fractal renders centered at the wrong viewpoint (the reference's `iOffset = (-1252.38, -1818.59)` puts the framing where it does in the reference image).

**Per-pixel rotation moved.** The plan applied `rotationPhase` as a 2x2 rotation on `z` after `cn_complex` and before the iteration loop. The reference applies it to `uv` inside `pixelOf()` after the iOffset shift and before the coordinate-scale exp. Moved to match.

**`jacobiRepeats` is `int`.** The plan had it as a modulatable float. Fractional values break the cn-tile periodicity (`mod(zoomPhase / spiralWrap, 1.0) * 3.7` only lands on a phase-equivalent lattice point when `jacobiRepeats` is an integer). Now `int`, no modulation, range 1-8, default 1, cast to `float` at the `SetShaderValue` call site.

**Polish features added.** The plan deferred Vogel-disk DOF, hash grain, and temporal AA to "use AudioJones Bokeh and Film Grain transforms downstream." That doesn't reproduce what those features do baked into the per-sample loop. Added all three in-shader:
- **Vogel DOF**: Multi-sample per-pixel via golden-angle disk offsets, `sampleCount` slider (default 2 = reference value).
- **Hash grain**: `+= grainAmount * (2 * hash12(1e4 * uvn) - 1)`, default 0.025 = reference value.
- **Temporal AA**: Owns its own HDR ping-pong texture (`prevFrame`). Uses `REGISTER_GENERATOR_FULL` with a custom `RenderFn` and `Resize` hook. Two render passes per frame:
  1. Shader writes to `pe->generatorScratch` with `prevFrame.texture` as the fullscreen-quad source.
  2. Default raylib shader copies `pe->generatorScratch` back into `prevFrame` for next frame.

**TAA: previous-frame texture is bound via `texture0`, not a named sampler.** First TAA attempt used a separate `uniform sampler2D prevFrame` bound via `SetShaderValueTexture`. Black-screened. Root cause: passing `prevFrame.texture` as the fullscreen-quad source AND binding it to a second named sampler created two simultaneous bindings of the same texture to different slots, and raylib's slot allocator did the wrong thing. Fix follows the `byzantine_display.fs` convention: declare `uniform sampler2D texture0` in the shader, sample THAT for the previous frame, and let the fullscreen-quad source argument bind it implicitly.

**Texture samplers must be bound INSIDE `BeginShaderMode` for `REGISTER_GENERATOR_FULL`.** Second black-screen during TAA wiring. The dispatcher's `REGISTER_GENERATOR_FULL` path calls `scratchSetup(pe)` BEFORE the custom `RenderFn`, with no shader active. Scalar uniforms set via `SetShaderValue` are cached in raylib's shader struct and reapplied at `glUseProgram`, but `SetShaderValueTexture` bindings need the shader active to land in the right texture unit. `byzantine.cpp` does this correctly: `gradientLUT` is bound inside the render's `BeginShaderMode` block. Followed the same pattern: `fftTexture` and `gradientLUT` are now bound inside `RenderDreamZoom`'s `BeginShaderMode`, not in `DreamZoomEffectSetup`.

**FFT uses BAND_SAMPLES standard.** Per the original plan and `memory/generator_patterns.md`. The research's single-bin lookup was wrong. Field naming is `gain` / `curve` (not `contrast`) per AudioJones convention.

**`spiralWrap` exposed as a slider.** The reference hardcoded `Speed = 4` for Dream and `Speed = 2` for Jacobi inside `spiralize()`. Surfaced as a user param so any cycle period is reachable.

**Default `baseBright = 0.15`, `gain = 2.0`.** Convention defaults. Do not change.
