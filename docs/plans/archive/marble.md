# Marble

A glowing glass marble floating in dark space, containing swirling luminous fractal filaments that breathe and morph over time. Raymarched 3D inversive fractal volume inside an analytical bounding sphere, with gradient LUT coloring and FFT-reactive depth layers. Front-of-sphere samples react to bass; core samples react to treble.

**Research**: `docs/research/marble.md`

## Design

### Types

**MarbleConfig** (in `src/effects/marble.h`):

```
bool enabled = false

// FFT mapping
float baseFreq = 55.0f        // Lowest mapped pitch (27.5-440)
float maxFreq = 14000.0f      // Highest mapped pitch (1000-16000)
float gain = 2.0f             // FFT sensitivity (0.1-10)
float curve = 1.5f            // Contrast exponent (0.1-3)
float baseBright = 0.15f      // Floor brightness (0-1)

// Geometry
int fractalIters = 10         // Fractal detail iterations (4-12)
int marchSteps = 64           // Raymarch sample count (32-128)
float stepSize = 0.02f        // Base ray step size (0.005-0.1)
float foldScale = 0.7f        // Inversive fold strength (0.3-1.2)
float foldOffset = -0.7f      // Attractor shift (-1.5-0.0)
float trapSensitivity = 19.0f // Orbit trap decay rate (5.0-40.0)
float sphereRadius = 2.0f     // Bounding sphere radius (1.0-3.0)

// Animation
float orbitSpeed = 0.1f       // Camera orbit rate rad/s (-2.0-2.0)
float perturbAmp = 0.15f      // Fold animation strength (0.0-0.5)
float perturbSpeed = 0.15f    // Fold animation rate rad/s (0.01-1.0)
float zoom = 1.0f             // Camera distance multiplier (0.5-3.0)

// Output
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**MarbleEffect** (in `src/effects/marble.h`):

```
Shader shader
ColorLUT *gradientLUT
float orbitPhase         // CPU-accumulated orbit angle
float perturbPhase       // CPU-accumulated perturb phase
int resolutionLoc
int fftTextureLoc
int sampleRateLoc
int gradientLUTLoc
int orbitPhaseLoc
int perturbPhaseLoc
int fractalItersLoc
int marchStepsLoc
int stepSizeLoc
int foldScaleLoc
int foldOffsetLoc
int trapSensitivityLoc
int sphereRadiusLoc
int perturbAmpLoc
int zoomLoc
int baseFreqLoc
int maxFreqLoc
int gainLoc
int curveLoc
int baseBrightLoc
```

### Algorithm

The shader is built from Playing Marble (guil) as the base, with Nova Marble's animated fold perturbation spliced in. Apply the substitution table from the research doc mechanically -- keep verbatim lines, replace marked lines with uniform-driven versions.

```glsl
// Based on "Playing marble" by guil (S. Guillitte 2015)
// https://www.shadertoy.com/view/MtX3Ws
// License: CC BY-NC-SA 3.0 Unported
//
// Based on "Nova Marble" by rwvens (Modified from S. Guillitte 2015)
// https://www.shadertoy.com/view/MtdGD8
// License: CC BY-NC-SA 3.0 Unported
//
// Modified: parameterized uniforms, gradient LUT coloring, FFT depth-band brightness,
// animated fold perturbation from Nova Marble

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float orbitPhase;
uniform float zoom;

uniform int fractalIters;
uniform float foldScale;
uniform float foldOffset;
uniform float trapSensitivity;
uniform float perturbPhase;
uniform float perturbAmp;

uniform int marchSteps;
uniform float stepSize;
uniform float sphereRadius;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Keep verbatim from Playing Marble
vec2 csqr(vec2 a) { return vec2(a.x*a.x - a.y*a.y, 2.*a.x*a.y); }

// Keep verbatim from Playing Marble
mat2 rot(float a) {
    return mat2(cos(a), sin(a), -sin(a), cos(a));
}

// Keep verbatim from Playing Marble (ray-sphere intersection from iq)
vec2 iSphere(in vec3 ro, in vec3 rd, in vec4 sph) {
    vec3 oc = ro - sph.xyz;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - sph.w*sph.w;
    float h = b*b - c;
    if (h < 0.0) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b-h, -b+h);
}

// Nova Marble fold perturbation + Playing Marble fractal core
// Replace: iteration count -> fractalIters uniform
// Replace: fold line -> Nova's cos(perturbPhase) animated version with uniforms
// Replace: trap exponent -> trapSensitivity uniform
// Keep: csqr(p.yz), p=p.zxy axis permutation, res/2 return
float map(in vec3 p) {
    float res = 0.;
    vec3 c = p;
    for (int i = 0; i < fractalIters; ++i) {
        p = foldScale*abs(p + cos(perturbPhase + 1.6)*perturbAmp) / dot(p,p)
            + foldOffset + cos(perturbPhase)*perturbAmp;
        p.yz = csqr(p.yz);
        p = p.zxy;
        res += exp(-trapSensitivity * abs(dot(p,c)));
    }
    return res/2.;
}

// Replace: step count -> marchSteps, base step -> stepSize
// Keep: exp(-2.*c) adaptive slowdown
// Replace: color accumulation with gradient LUT + FFT depth-band brightness
vec3 raymarch(in vec3 ro, vec3 rd, vec2 tminmax) {
    float t = tminmax.x;
    float dt = stepSize;
    vec3 col = vec3(0.);
    float c = 0.;
    for (int i = 0; i < marchSteps; i++) {
        t += dt*exp(-2.*c);
        if (t > tminmax.y) break;

        c = map(ro + t*rd);

        // Normalized depth: 0 at sphere entry, 1 at sphere exit
        float tNorm = (t - tminmax.x) / (tminmax.y - tminmax.x);

        // Gradient LUT color from depth
        vec3 lutColor = texture(gradientLUT, vec2(tNorm, 0.5)).rgb;

        // FFT frequency from depth (bass at front, treble at core)
        float freq = baseFreq * pow(maxFreq / baseFreq, tNorm);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        // 0.99 decay + 0.08 accumulation match reference rates
        col = 0.99 * col + 0.08 * c * lutColor * brightness;
    }
    return col;
}

void main() {
    vec2 q = gl_FragCoord.xy / resolution.xy;
    vec2 p = -1.0 + 2.0 * q;
    p.x *= resolution.x / resolution.y;

    // Camera: orbit only (no mouse), zoom uniform replaces hardcoded 1.0
    vec3 ro = zoom * vec3(4.);
    ro.xz *= rot(orbitPhase);
    vec3 ta = vec3(0.0, 0.0, 0.0);
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    vec3 rd = normalize(p.x*uu + p.y*vv + 4.0*ww);

    // Sphere radius uniform replaces hardcoded 2.0
    vec2 tmm = iSphere(ro, rd, vec4(0., 0., 0., sphereRadius));

    // Dark void when ray misses sphere (no cubemap)
    vec3 col = vec3(0.);
    if (tmm.x >= 0.) {
        col = raymarch(ro, rd, tmm);
    }
    // No Fresnel reflection (no cubemap)

    // Clamp only, no tonemap
    col = clamp(col, 0.0, 1.0);
    finalColor = vec4(col, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| fractalIters | int | 4-12 | 10 | No | Iterations |
| marchSteps | int | 32-128 | 64 | No | March Steps |
| stepSize | float | 0.005-0.1 | 0.02 | Yes | Step Size |
| foldScale | float | 0.3-1.2 | 0.7 | Yes | Fold Scale |
| foldOffset | float | -1.5-0.0 | -0.7 | Yes | Fold Offset |
| trapSensitivity | float | 5.0-40.0 | 19.0 | Yes | Trap Sensitivity |
| sphereRadius | float | 1.0-3.0 | 2.0 | Yes | Sphere Radius |
| orbitSpeed | float | -2.0-2.0 | 0.1 | Yes | Orbit Speed |
| perturbAmp | float | 0.0-0.5 | 0.15 | Yes | Perturb Amp |
| perturbSpeed | float | 0.01-1.0 | 0.15 | Yes | Perturb Speed |
| zoom | float | 0.5-3.0 | 1.0 | Yes | Zoom |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_MARBLE_BLEND`
- Display name: `"Marble"`
- Badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: 13 (Field)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)

### UI Sections

Following Signal Stack section ordering and Dream Fractal as template:

1. **Audio**: baseFreq, maxFreq, gain, curve, baseBright (standard 5 sliders, standard format/ranges)
2. **Geometry**: fractalIters (`SliderInt` 4-12), marchSteps (`SliderInt` 32-128), stepSize (`ModulatableSliderLog` 0.005-0.1), foldScale, foldOffset, trapSensitivity, sphereRadius (all `ModulatableSlider`)
3. **Animation**: orbitSpeed (`ModulatableSliderSpeedDeg`), perturbSpeed (`ModulatableSliderSpeedDeg`), perturbAmp, zoom (all `ModulatableSlider`)
4. **Output**: via `STANDARD_GENERATOR_OUTPUT(marble)` macro

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create marble.h

**Files**: `src/effects/marble.h` (create)
**Creates**: `MarbleConfig`, `MarbleEffect` types, `MARBLE_CONFIG_FIELDS` macro

**Do**: Create the header following `dream_fractal.h` as template. Declare `MarbleConfig` with all fields from the Design section (include `render/color_config.h` and `render/blend_mode.h` for `ColorConfig` and `EffectBlendMode`). Declare `MarbleEffect` with all loc fields from the Design section. Declare the 4 public functions: `MarbleEffectInit(MarbleEffect*, const MarbleConfig*)`, `MarbleEffectSetup(MarbleEffect*, const MarbleConfig*, float deltaTime, const Texture2D& fftTexture)`, `MarbleEffectUninit(MarbleEffect*)`, `MarbleRegisterParams(MarbleConfig*)`. Forward-declare `ColorLUT`. Define `MARBLE_CONFIG_FIELDS` macro listing all serializable fields.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker errors expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create marble.cpp

**Files**: `src/effects/marble.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement following `dream_fractal.cpp` as template:

- `MarbleEffectInit`: Load `shaders/marble.fs`, cache all uniform locations, init `ColorLUTInit(&cfg->gradient)`, zero both phase accumulators. Cascade cleanup on failure (unload shader if LUT fails).
- `MarbleEffectSetup`: Accumulate `orbitPhase += cfg->orbitSpeed * deltaTime` and `perturbPhase += cfg->perturbSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms via `SetShaderValue`/`SetShaderValueTexture`.
- `MarbleEffectUninit`: Unload shader, `ColorLUTUninit`.
- `MarbleRegisterParams`: Register all modulatable float params from the Parameters table with `"marble."` prefix. Include `blendIntensity`.
- `SetupMarble(PostEffect* pe)`: Bridge function (non-static) calling `MarbleEffectSetup` with `pe->marble`, `pe->effects.marble`, `pe->currentDeltaTime`, `pe->fftTexture`.
- `SetupMarbleBlend(PostEffect* pe)`: Bridge function (non-static) calling `BlendCompositorApply` with `pe->blendCompositor`, `pe->generatorScratch.texture`, `pe->effects.marble.blendIntensity`, `pe->effects.marble.blendMode`.
- Colocated UI `DrawMarbleParams` (static): 3 sections (Audio, Geometry, Animation) following the UI Sections in the Design. `stepSize` uses `ModulatableSliderLog`. `orbitSpeed` uses `ModulatableSliderSpeedDeg`. `perturbSpeed` uses `ModulatableSliderSpeedDeg`.
- `STANDARD_GENERATOR_OUTPUT(marble)` macro.
- `REGISTER_GENERATOR(TRANSFORM_MARBLE_BLEND, Marble, marble, "Marble", SetupMarbleBlend, SetupMarble, 13, DrawMarbleParams, DrawOutput_marble)`.

Includes: `marble.h`, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

**Verify**: Compiles.

#### Task 2.2: Create marble.fs

**Files**: `shaders/marble.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from this plan verbatim. The shader code in the Algorithm section is the complete, final shader -- transcribe it directly.

**Verify**: Compiles (shader compilation happens at runtime, but syntax should be valid GLSL 330).

#### Task 2.3: Integration

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (modify all)
**Depends on**: Wave 1 complete

**Do**:

`src/config/effect_config.h`:
1. Add `#include "effects/marble.h"` with the other effect includes
2. Add `TRANSFORM_MARBLE_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT` (place after `TRANSFORM_SPIRAL_NEST_BLEND`)
3. Add `TRANSFORM_MARBLE_BLEND` to `TransformOrderConfig::order` array (at end, before closing brace)
4. Add `MarbleConfig marble;` to `EffectConfig` struct

`src/render/post_effect.h`:
1. Add `#include "effects/marble.h"` with the other effect includes
2. Add `MarbleEffect marble;` to `PostEffect` struct

`CMakeLists.txt`:
1. Add `src/effects/marble.cpp` to `EFFECTS_SOURCES`

`src/config/effect_serialization.cpp`:
1. Add `#include "effects/marble.h"`
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MarbleConfig, MARBLE_CONFIG_FIELDS)`
3. Add `X(marble) \` to `EFFECT_CONFIG_FIELDS` X-macro table

**Verify**: Full build succeeds with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Effects window under Field category
- [ ] Enabling Marble renders a glowing fractal sphere on dark background
- [ ] Audio reactivity: filaments brighten with music (bass at surface, treble at core)
- [ ] Orbit speed rotates the camera around the marble
- [ ] Perturb amp/speed animate the internal filament morphing
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters

## Implementation Notes

- Restored `0.5 * log(1.0 + col)` from the reference. This is not Reinhard - it is a soft log compression structurally required for the 64-step volumetric accumulation to resolve filaments without blowout.
- Added Fresnel edge glow using the reference formula (`pow(0.5 + clamp(dot(nor, rd), 0.0, 1.0), 3.0) * 1.3`). Reflects the marble's own color at glancing angles for the glass-sphere surface illusion. No cubemap needed.
- Added constant glass fog (`0.015`) to the raymarch accumulation so the sphere reads as solid marble even when fractal filaments are sparse at certain perturbation phases.
