# Protean Clouds

Volumetric raymarched cloud generator with gradient LUT coloring and FFT brightness. Fly through infinite cloud formations built from nimitz's deformed periodic grid noise, lit by dual-sample diffuse gradients. A density-vs-depth blend slider controls how the gradient LUT maps across the volume. FFT audio drives cloud emission intensity.

**Research**: `docs/research/protean_clouds.md`

## Design

### Types

#### ProteanCloudsConfig (in protean_clouds.h)

```
bool enabled = false

// Volume
float morph = 0.5f              // Cloud morphology (0.0-1.0)
float turbulence = 0.15f        // Displacement amplitude in noise loop (0.05-0.5)
float densityThreshold = 0.3f   // Minimum density for visible cloud (0.0-1.0)
int marchSteps = 80             // Raymarch iterations (40-130)

// Color
float colorBlend = 0.3f         // LUT index: 0=density, 1=depth (0.0-1.0)
float fogIntensity = 0.5f       // Atmospheric fog amount (0.0-1.0)

// Motion
float speed = 3.0f              // Flight speed (0.5-6.0)

// Audio
float baseFreq = 55.0f          // FFT low frequency Hz (27.5-440)
float maxFreq = 14000.0f        // FFT high frequency Hz (1000-16000)
float gain = 2.0f               // FFT amplitude multiplier (0.1-10)
float curve = 1.5f              // FFT response curve exponent (0.1-3.0)
float baseBright = 0.15f        // Minimum brightness when silent (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

#### ProteanCloudsEffect (in protean_clouds.h)

```
Shader shader
ColorLUT *gradientLUT
float time          // Raw animation clock accumulator (1x rate)
float flyPhase      // Flight z-position accumulator (speed * deltaTime)
int resolutionLoc
int timeLoc
int flyPhaseLoc
int fftTextureLoc
int sampleRateLoc
int gradientLUTLoc
int morphLoc
int colorBlendLoc
int fogIntensityLoc
int turbulenceLoc
int densityThresholdLoc
int marchStepsLoc
int baseFreqLoc
int maxFreqLoc
int gainLoc
int curveLoc
int baseBrightLoc
```

Two time accumulators because the original shader uses two time scales: raw `iTime` for noise animation in `map()` and camera xy-sway, and `iTime*3` for camera z-travel. `time` = raw clock (`+= deltaTime`), `flyPhase` = flight position (`+= speed * deltaTime`).

### Algorithm

<!-- Intentional deviation from research doc: research specifies a single `time` uniform with
     speed baked in. The original shader actually uses two time scales (iTime for noise, iTime*3
     for flight). This plan splits them into `time` (animation) and `flyPhase` (flight) to
     preserve that behavior. Also removed dead `lpos`/`ldst` variables from render(). -->

Complete adapted shader (apply to `shaders/protean_clouds.fs`):

```glsl
// Protean Clouds - volumetric raymarched clouds with gradient LUT coloring
// Based on "Protean clouds" by nimitz (@stormoid)
// https://www.shadertoy.com/view/3l23Rh
// License: CC BY-NC-SA 3.0 Unported
// Modified: gradient LUT coloring, FFT brightness, parametric fog, AudioJones uniforms

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform float flyPhase;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float morph;
uniform float colorBlend;
uniform float fogIntensity;
uniform float turbulence;
uniform float densityThreshold;
uniform int marchSteps;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define MAX_STEPS 130
#define MAX_DIST 50.0

mat2 rot(in float a) { float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }
const mat3 m3 = mat3(0.33338, 0.56034, -0.71817, -0.87887, 0.32651, -0.15323,
                     0.15162, 0.69596, 0.61339) * 1.93;
float mag2(vec2 p) { return dot(p, p); }
float linstep(in float mn, in float mx, in float x) {
    return clamp((x - mn) / (mx - mn), 0., 1.);
}

vec2 disp(float t) { return vec2(sin(t * 0.22), cos(t * 0.175)) * 2.; }

vec2 map(vec3 p) {
    vec3 p2 = p;
    p2.xy -= disp(p.z).xy;
    p.xy *= rot(sin(p.z + time) * (0.1 + morph * 0.05) + time * 0.09);
    float cl = mag2(p2.xy);
    float d = 0.;
    p *= .61;
    float z = 1.;
    float trk = 1.;
    float dspAmp = turbulence;
    for (int i = 0; i < 5; i++) {
        p += sin(p.zxy * 0.75 * trk + time * trk * .8) * dspAmp;
        d -= abs(dot(cos(p), sin(p.yzx)) * z);
        z *= 0.57;
        trk *= 1.4;
        p = p * m3;
    }
    d = abs(d + morph * 3.) + morph * 0.3 - 2.5;
    return vec2(d + cl * .2 + 0.25, cl);
}

vec4 render(in vec3 ro, in vec3 rd, float brightness) {
    vec4 rez = vec4(0);
    float t = 1.5;
    float fogT = 0.;
    for (int i = 0; i < MAX_STEPS; i++) {
        if (i >= marchSteps) break;
        if (rez.a > 0.99) break;

        vec3 pos = ro + t * rd;
        vec2 mpv = map(pos);
        float den = clamp(mpv.x - densityThreshold, 0., 1.) * 1.12;
        float dn = clamp((mpv.x + 2.), 0., 3.);

        vec4 col = vec4(0);
        if (mpv.x > densityThreshold + 0.3) {
            float lutIndex = mix(den, t / MAX_DIST, colorBlend);
            col = vec4(texture(gradientLUT, vec2(lutIndex, 0.5)).rgb, 0.08);
            col *= den * den * den;
            col.rgb *= linstep(4., -2.5, mpv.x) * 2.3;
            float dif = clamp((den - map(pos + .8).x) / 9., 0.001, 1.);
            dif += clamp((den - map(pos + .35).x) / 2.5, 0.001, 1.);
            col.rgb *= den * brightness * (0.3 + 1.5 * dif);
        }

        float fogC = exp(t * 0.2 - 2.2);
        col.rgba += vec4(texture(gradientLUT, vec2(0.0, 0.5)).rgb * 0.3, 0.1)
                    * fogIntensity * clamp(fogC - fogT, 0., 1.);
        fogT = fogC;
        rez = rez + col * (1. - rez.a);
        t += clamp(0.5 - dn * dn * .05, 0.09, 0.3);
    }
    return clamp(rez, 0.0, 1.0);
}

void main() {
    vec2 p = (fragTexCoord * resolution - 0.5 * resolution) / resolution.y;

    vec3 ro = vec3(0, 0, flyPhase);
    ro += vec3(sin(time) * 0.5, 0., 0.);

    float dspAmp = .85;
    ro.xy += disp(ro.z) * dspAmp;
    float tgtDst = 3.5;

    vec3 target = normalize(ro - vec3(disp(flyPhase + tgtDst) * dspAmp, flyPhase + tgtDst));
    vec3 rightdir = normalize(cross(target, vec3(0, 1, 0)));
    vec3 updir = normalize(cross(rightdir, target));
    rightdir = normalize(cross(updir, target));
    vec3 rd = normalize((p.x * rightdir + p.y * updir) - target);
    rd.xy *= rot(-disp(flyPhase + 3.5).x * 0.2);

    // FFT brightness
    const int FFT_SAMPLES = 8;
    float energy = 0.0;
    float binLo = baseFreq / (sampleRate * 0.5);
    float binHi = maxFreq / (sampleRate * 0.5);
    for (int s = 0; s < FFT_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(FFT_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float fftBright = pow(clamp(energy / float(FFT_SAMPLES) * gain, 0.0, 1.0), curve);
    float brightness = baseBright + fftBright;

    vec4 scn = render(ro, rd, brightness);
    finalColor = vec4(scn.rgb, scn.a);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| speed | float | 0.5-6.0 | 3.0 | yes | Speed |
| morph | float | 0.0-1.0 | 0.5 | yes | Morph |
| turbulence | float | 0.05-0.5 | 0.15 | yes | Turbulence |
| densityThreshold | float | 0.0-1.0 | 0.3 | yes | Density |
| marchSteps | int | 40-130 | 80 | no | March Steps |
| colorBlend | float | 0.0-1.0 | 0.3 | yes | Color Blend |
| fogIntensity | float | 0.0-1.0 | 0.5 | yes | Fog |
| baseFreq | float | 27.5-440 | 55 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000 | yes | Max Freq (Hz) |
| gain | float | 0.1-10 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |

### Constants

- Enum: `TRANSFORM_PROTEAN_CLOUDS_BLEND`
- Display name: `"Protean Clouds"`
- Category section: 13 (GEN badge auto-set by `REGISTER_GENERATOR`)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Registration macro: `REGISTER_GENERATOR`
- Field name: `proteanClouds`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create protean_clouds.h

**Files**: `src/effects/protean_clouds.h` (create)
**Creates**: `ProteanCloudsConfig`, `ProteanCloudsEffect` types, lifecycle declarations

**Do**: Follow `src/effects/voxel_march.h` as pattern. Define `ProteanCloudsConfig` struct and `ProteanCloudsEffect` struct per the Design section above. Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`. Define `PROTEAN_CLOUDS_CONFIG_FIELDS` macro listing all serializable fields. Declare:
- `bool ProteanCloudsEffectInit(ProteanCloudsEffect *e, const ProteanCloudsConfig *cfg);`
- `void ProteanCloudsEffectSetup(ProteanCloudsEffect *e, const ProteanCloudsConfig *cfg, float deltaTime, const Texture2D &fftTexture);`
- `void ProteanCloudsEffectUninit(ProteanCloudsEffect *e);`
- `void ProteanCloudsRegisterParams(ProteanCloudsConfig *cfg);`

Setup takes `const` config (no lissajous state mutation needed unlike voxel_march).

**Verify**: `cmake.exe --build build` compiles (header-only, tested via later includes).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create protean_clouds.cpp

**Files**: `src/effects/protean_clouds.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `src/effects/voxel_march.cpp` as pattern.

**Init**: Load shader `"shaders/protean_clouds.fs"`, cache all 17 uniform locations (`resolution`, `time`, `flyPhase`, `fftTexture`, `sampleRate`, `gradientLUT`, `morph`, `colorBlend`, `fogIntensity`, `turbulence`, `densityThreshold`, `marchSteps`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`). Init `ColorLUT` from `cfg->gradient`. Set `time = 0`, `flyPhase = 0`. On shader fail return false; on LUT fail unload shader then return false.

**Setup**: Accumulate `e->time += deltaTime` and `e->flyPhase += cfg->speed * deltaTime`. Call `ColorLUTUpdate`. Bind resolution (GetScreenWidth/Height), fftTexture, sampleRate (AUDIO_SAMPLE_RATE), time, flyPhase, gradientLUT, and all config params. Extract `BindUniforms` static helper for config params (same pattern as voxel_march).

**Uninit**: Unload shader, `ColorLUTUninit`.

**RegisterParams**: Register all modulatable float params per the Parameters table. Use `"proteanClouds."` prefix. Do NOT register `marchSteps` (int, not modulatable).

**Bridge functions**:
- `void SetupProteanClouds(PostEffect *pe)` - calls `ProteanCloudsEffectSetup` (non-static)
- `void SetupProteanCloudsBlend(PostEffect *pe)` - calls `BlendCompositorApply` (non-static)

**UI section** (`// === UI ===`):
- `static void DrawProteanCloudsParams(EffectConfig*, const ModSources*, ImU32)`:
  - Audio: Base Freq, Max Freq, Gain, Contrast (curve), Base Bright - standard pattern
  - Volume: March Steps (`SliderInt`), Morph, Turbulence, Density (densityThreshold)
  - Color: Color Blend, Fog (fogIntensity)
  - Motion: Speed

**Registration**:
```
STANDARD_GENERATOR_OUTPUT(proteanClouds)
REGISTER_GENERATOR(TRANSFORM_PROTEAN_CLOUDS_BLEND, ProteanClouds, proteanClouds,
                   "Protean Clouds", SetupProteanCloudsBlend, SetupProteanClouds,
                   13, DrawProteanCloudsParams, DrawOutput_proteanClouds)
```

**Includes**: `protean_clouds.h`, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create protean_clouds.fs shader

**Files**: `shaders/protean_clouds.fs` (create)
**Depends on**: Wave 1 (for uniform name alignment)

**Do**: Implement the Algorithm section verbatim. The complete shader code is provided in the Algorithm section above - transcribe it exactly. Attribution header is included in the Algorithm code.

**Verify**: `cmake.exe --build build` succeeds (shader parse errors surface at runtime, but no compile errors).

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/protean_clouds.h"` in the alphabetical include list (after `plasma.h`, before `pointillism.h` or similar 'p' entry)
2. Add `TRANSFORM_PROTEAN_CLOUDS_BLEND,` in the `TransformEffectType` enum, before `TRANSFORM_ACCUM_COMPOSITE` (after `TRANSFORM_SUBDIVIDE_BLEND`)
3. Add `ProteanCloudsConfig proteanClouds;` member in `EffectConfig` struct (near the end, after `VoxelMarchConfig voxelMarch;`)

The order array auto-initializes from enum indices - no manual list to edit.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/protean_clouds.h"` in the include list
2. Add `ProteanCloudsEffect proteanClouds;` member in the `PostEffect` struct (near `VoxelMarchEffect voxelMarch;`)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)

**Do**: Add `src/effects/protean_clouds.cpp` to the `EFFECTS_SOURCES` list (alphabetical placement near other 'p' entries).

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release` configures successfully.

---

#### Task 2.6: Register serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/protean_clouds.h"` with other effect includes
2. Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProteanCloudsConfig, PROTEAN_CLOUDS_CONFIG_FIELDS)`
3. Add `X(proteanClouds) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (after `X(voxelMarch)`)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] `TRANSFORM_PROTEAN_CLOUDS_BLEND` enum exists before `TRANSFORM_ACCUM_COMPOSITE`
- [ ] Attribution comment present in `shaders/protean_clouds.fs` first lines
- [ ] Two time accumulators in Setup: `time += deltaTime`, `flyPhase += speed * deltaTime`
- [ ] Shader uses `time` in `map()` for noise animation, `flyPhase` in `main()` for camera z-travel
- [ ] `marchSteps` is `int` in config and `SliderInt` in UI (not modulatable)
- [ ] `REGISTER_GENERATOR` macro at bottom of .cpp with `STANDARD_GENERATOR_OUTPUT`
- [ ] All 17 uniform locations cached in Init, bound in Setup
