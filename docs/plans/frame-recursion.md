# Frame Tunnel

A wireframe platonic-solid skeleton receding infinitely down a tunnel. Kaleidoscopic plane-fold IFS carves polyhedral edges from raymarched space; `fract(log(p.z))` along the camera axis creates seamless self-similar zoom. Per-axis tumble, gradient-LUT coloring sampled from depth, FFT brightness modulation per march step.

**Research**: `docs/research/frame_tunnel.md`

**Sibling**: `src/effects/apollonian_tunnel.{h,cpp}`, `shaders/apollonian_tunnel.fs` (raymarched generator with FFT band sampling on shared `t` index).

## Design

### Types

`src/effects/frame_tunnel.h` declares:

```cpp
struct FrameTunnelConfig {
  bool enabled = false;

  // Shape
  int shape = 2; // 0=Octahedron, 1=Dodecahedron, 2=Icosahedron

  // Rotation (per-axis CPU-accumulated speeds, rad/s)
  float rotateSpeedX = 0.0f;
  float rotateSpeedY = 0.5f;
  float rotateSpeedZ = 0.0f;

  // Zoom
  float zoomSpeed = 0.5f; // fract(log) tunnel zoom rate (signed; negative reverses)

  // Geometry
  float edgeRadius = 0.01f;
  float glow = 0.05f;
  int marchSteps = 99;

  // Audio (FFT)
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

#define FRAME_TUNNEL_CONFIG_FIELDS                                             \
  enabled, shape, rotateSpeedX, rotateSpeedY, rotateSpeedZ, zoomSpeed,         \
      edgeRadius, glow, marchSteps, baseFreq, maxFreq, gain, curve,            \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct FrameTunnelEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases (rad)
  float rotateAngleX;
  float rotateAngleY;
  float rotateAngleZ;
  float zoomPhase;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int rotateAngleLoc; // vec3
  int zoomPhaseLoc;
  int shapeLoc;
  int edgeRadiusLoc;
  int glowLoc;
  int marchStepsLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} FrameTunnelEffect;

bool FrameTunnelEffectInit(FrameTunnelEffect *e, const FrameTunnelConfig *cfg);
void FrameTunnelEffectSetup(FrameTunnelEffect *e, FrameTunnelConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);
void FrameTunnelEffectUninit(FrameTunnelEffect *e);
void FrameTunnelRegisterParams(FrameTunnelConfig *cfg);
FrameTunnelEffect *GetFrameTunnelEffect(struct PostEffect *pe);
```

One `_BLEND` enum suffix per generator convention. All four phases wrap modulo `2*PI_F` via `fmodf` after each accumulation to bound precision growth over long sessions. The shader's `fract(log(p.z) - zoomPhase)` applies its own mod-1 wrap on the consumer side, so the CPU-side `2*PI_F` wrap doesn't affect visual output.

Two intentional deviations from a strict reading of the research substitution table, preserved for correctness and convention compliance:

<!-- Intentional deviation: research writes `gradientLUT(dot(p,p) * 0.5)` literally,
     but the plan wraps the index in `fract()`. Reason: the FFT band sampler computes
     `pow(maxFreq/baseFreq, t)` which becomes nonsensical for `t > 1` (ultra-high freq,
     bin > 1, no FFT energy contribution). Fract keeps `t` in `[0, 1]` so both gradient
     and FFT see consistent indices, satisfying the project's generator_patterns rule
     that gradient and FFT share the same `t`. Reference H() palette is intrinsically
     periodic via cos(); we replicate that periodicity explicitly with fract(). -->

<!-- Intentional deviation: research mentions "depth-mapped frequency" for the FFT
     mapping. Plan uses `fract(dot(p,p)*0.5)` (folded squared distance) as the depth
     index for both gradient and FFT, mirroring the reference's coloring formula. This
     is the same value the reference uses to drive its cosine palette - the natural
     "depth" coordinate in the gaz family. Alternative `float(i)/float(marchSteps)`
     would decouple gradient from FFT; rejected. -->

### Algorithm

The shader is a mechanical transcription of the **Icosahedron Frame** reference (gaz, https://www.shadertoy.com/view/st2GRd) with the substitution table from `docs/research/frame_tunnel.md` applied.

`shaders/frame_tunnel.fs` - full source:

```glsl
// Based on "Icosahedron frame" by gaz
// https://www.shadertoy.com/view/st2GRd
// License: CC BY-NC-SA 3.0 Unported
// Modified: per-axis CPU-accumulated rotation, shape selector for 3 platonic
//           solids, gradient LUT replaces cosine palette, FFT brightness on
//           shared depth-cycle t index
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform vec3 rotateAngle;  // CPU-accumulated per-axis (rad)
uniform float zoomPhase;   // CPU-accumulated tunnel zoom phase
uniform int shape;         // 0=Octa, 1=Dodeca, 2=Icosa
uniform float edgeRadius;
uniform float glow;
uniform int marchSteps;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define R(p, a, r) (mix(a * dot(p, a), p, cos(r)) + sin(r) * cross(p, a))

float sampleFFTBand(float t0, float t1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
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
    vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;
    vec3 d = normalize(vec3(uv, 1.0));

    // Per-shape fold normal and fold count (verified gaz-family values)
    vec3 n;
    int foldCount;
    if (shape == 0) {
        n = vec3(-0.5, -0.7071068, 0.5);
        foldCount = 4;
    } else if (shape == 1) {
        n = vec3(-0.5, -0.809, 0.309);
        foldCount = 5;
    } else {
        n = vec3(-0.5, -0.809, 0.309);
        foldCount = 5;
    }

    vec3 accumulator = vec3(0.0);
    vec3 p;
    float g = 0.0;
    float e = 0.0;
    const float intensity = 0.05;  // per-iter brightness coefficient (research-sanctioned: "kept as a separate intensity constant")

    for (int i = 0; i < marchSteps; i++) {
        p = g * d;
        p.z -= 10.0;
        p = R(p, vec3(1.0, 0.0, 0.0), rotateAngle.x);
        p = R(p, vec3(0.0, 1.0, 0.0), rotateAngle.y);
        p = R(p, vec3(0.0, 0.0, 1.0), rotateAngle.z);

        for (int j = 0; j < foldCount; j++) {
            p.xy = abs(p.xy);
            p -= 2.0 * min(0.0, dot(p, n)) * n;
        }
        p.z = fract(log(p.z) - zoomPhase) - 0.5;

        if (shape == 0) {
            // Octahedron edge metric (from polyhedron Frame ref)
            e = abs(min(length(p.yz), length(p.xz)) - edgeRadius) + 0.001;
        } else if (shape == 1) {
            // Dodecahedron edge axes (icosa frame's commented variant)
            e = length(p.xz) - edgeRadius;
        } else {
            // Icosahedron primary edge metric
            e = length(p.yz) - edgeRadius;
        }
        g += e;

        float t = fract(dot(p, p) * 0.5);
        vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;
        float brightness = sampleFFTBand(t, t + 1.0 / float(marchSteps));

        float fi = float(i + 1);
        accumulator += baseColor * brightness * intensity * exp(-glow * fi * fi * e);
    }

    finalColor = vec4(accumulator, 1.0);
}
```

CPU side, `FrameTunnelEffectSetup` accumulates all four phases each frame:

```cpp
e->rotateAngleX = fmodf(e->rotateAngleX + cfg->rotateSpeedX * deltaTime, 2.0f * PI_F);
e->rotateAngleY = fmodf(e->rotateAngleY + cfg->rotateSpeedY * deltaTime, 2.0f * PI_F);
e->rotateAngleZ = fmodf(e->rotateAngleZ + cfg->rotateSpeedZ * deltaTime, 2.0f * PI_F);
e->zoomPhase    = fmodf(e->zoomPhase    + cfg->zoomSpeed     * deltaTime, 2.0f * PI_F);
```

Bind order each frame: `resolution`, `fftTexture`, `sampleRate`, `gradientLUT`, `rotateAngle` (vec3), `zoomPhase`, `shape`, `edgeRadius`, `glow`, `marchSteps`, FFT-audio uniforms (`baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`). Mirrors `apollonian_tunnel.cpp:BindUniforms` structure.

### Parameters

| Parameter        | Type            | Range                     | Default  | Modulatable | UI Label                                                |
|------------------|-----------------|---------------------------|----------|-------------|---------------------------------------------------------|
| `shape`          | int             | 0-2                       | 2        | No          | Combo: "Shape" - Octahedron / Dodecahedron / Icosahedron |
| `rotateSpeedX`   | float           | -ROTATION_SPEED_MAX..+MAX | 0.0      | Yes         | "Rotate Speed X" (deg/s, via `ModulatableSliderSpeedDeg`) |
| `rotateSpeedY`   | float           | -ROTATION_SPEED_MAX..+MAX | 0.5      | Yes         | "Rotate Speed Y" (deg/s)                                |
| `rotateSpeedZ`   | float           | -ROTATION_SPEED_MAX..+MAX | 0.0      | Yes         | "Rotate Speed Z" (deg/s)                                |
| `zoomSpeed`      | float           | -2.0..+2.0                | 0.5      | Yes         | "Zoom Speed" (`%.2f`)                                   |
| `edgeRadius`     | float           | 0.001..0.05               | 0.01     | Yes         | "Edge Radius" (`%.3f`, `ModulatableSliderLog`)           |
| `glow`           | float           | 0.01..0.2                 | 0.05     | Yes         | "Glow" (`%.3f`)                                         |
| `marchSteps`     | int             | 32..128                   | 99       | No          | "March Steps" (`ImGui::SliderInt`)                      |
| `baseFreq`       | float           | 27.5..440                 | 55.0     | Yes         | "Base Freq (Hz)" (`%.1f`)                               |
| `maxFreq`        | float           | 1000..16000               | 14000.0  | Yes         | "Max Freq (Hz)" (`%.0f`)                                |
| `gain`           | float           | 0.1..10                   | 2.0      | Yes         | "Gain" (`%.1f`)                                         |
| `curve`          | float           | 0.1..3.0                  | 1.5      | Yes         | "Contrast" (`%.2f`)                                     |
| `baseBright`     | float           | 0.0..1.0                  | 0.15     | Yes         | "Base Bright" (`%.2f`)                                  |
| `gradient`       | ColorConfig     | -                         | gradient | No          | gradient widget (via `STANDARD_GENERATOR_OUTPUT`)       |
| `blendMode`      | EffectBlendMode | -                         | SCREEN   | No          | "Blend Mode" combo (via `STANDARD_GENERATOR_OUTPUT`)    |
| `blendIntensity` | float           | 0.0..5.0                  | 1.0      | Yes         | "Blend Intensity" (via `STANDARD_GENERATOR_OUTPUT`)     |

UI section ordering (per Signal Stack):
1. **Audio** - baseFreq, maxFreq, gain, curve (label "Contrast"), baseBright
2. **Geometry** - Shape combo, edgeRadius, marchSteps
3. **Animation** - rotateSpeedX, rotateSpeedY, rotateSpeedZ, zoomSpeed
4. **Glow** - glow
5. Output section emitted by `STANDARD_GENERATOR_OUTPUT(frameTunnel)`

### Constants

- Enum: `TRANSFORM_FRAME_TUNNEL_BLEND` (placed at end of `TransformEffectType` before `TRANSFORM_ACCUM_COMPOSITE`, matching the pattern for newly-added generators)
- Display name: `"Frame Tunnel"`
- Badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `10` (Geometric)
- Field on `EffectConfig`: `frameTunnel`
- Param prefix: `"frameTunnel."`

Modulation registrations (16 entries) in `FrameTunnelRegisterParams`:
- `frameTunnel.rotateSpeedX/Y/Z` with `-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX`
- `frameTunnel.zoomSpeed` with `-2.0f, 2.0f`
- `frameTunnel.edgeRadius` with `0.001f, 0.05f`
- `frameTunnel.glow` with `0.01f, 0.2f`
- `frameTunnel.baseFreq` with `27.5f, 440.0f`
- `frameTunnel.maxFreq` with `1000.0f, 16000.0f`
- `frameTunnel.gain` with `0.1f, 10.0f`
- `frameTunnel.curve` with `0.1f, 3.0f`
- `frameTunnel.baseBright` with `0.0f, 1.0f`
- `frameTunnel.blendIntensity` with `0.0f, 5.0f`

`shape` and `marchSteps` are not registered (discrete/perf knobs, not modulatable).

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create header

**Files**: `src/effects/frame_tunnel.h`
**Creates**: `FrameTunnelConfig`, `FrameTunnelEffect`, `FRAME_TUNNEL_CONFIG_FIELDS` macro, public function declarations.

**Do**: Implement the Types section above verbatim. Follow `src/effects/apollonian_tunnel.h` for structure: include guards, raylib + blend_mode + color_config + stdbool, forward-declare `struct PostEffect`, declare config / effect / public functions. Use the field defaults and `_CONFIG_FIELDS` macro from the Design section.

**Verify**: `cmake.exe --build build` compiles (header alone won't link anything new but should not break the build when included indirectly).

---

### Wave 2: Implementation

All four tasks depend on Wave 1 (the header must exist) and have no file overlap with each other.

#### Task 2.1: Create effect module

**Files**: `src/effects/frame_tunnel.cpp`
**Depends on**: Task 1.1

**Do**: Implement Init, Setup, Uninit, RegisterParams, the `Get`/`Setup`/`SetupBlend` bridge functions, the colocated `DrawFrameTunnelParams` UI function, and `STANDARD_GENERATOR_OUTPUT(frameTunnel)` + `REGISTER_GENERATOR` macro at the bottom. Mirror `src/effects/apollonian_tunnel.cpp` line-for-line, substituting names and the parameter set described in the Design section.

Specific points:
- Init follows apollonian_tunnel exactly: cache uniform locations, call `ColorLUTInit`, on LUT failure unload shader and return false. Initialize all four phases to 0.
- Setup accumulates four phases on CPU (formula in Design > Algorithm), calls `ColorLUTUpdate`, sets `resolution`, `fftTexture`, `sampleRate`, `gradientLUT`, `rotateAngle` (vec3 from the three accumulators), `zoomPhase`, `shape`, `edgeRadius`, `glow`, `marchSteps`, FFT block (5 uniforms). Uses `SetShaderValue(... SHADER_UNIFORM_VEC3)` for `rotateAngle`, `SHADER_UNIFORM_INT` for `shape` and `marchSteps`.
- RegisterParams registers exactly the 16 IDs listed in Design > Constants. Use `ROTATION_SPEED_MAX` from `config/constants.h` for the three rotate speeds.
- UI section ordering per Design > Parameters. Use `ModulatableSliderSpeedDeg` for the three rotate speeds, `ModulatableSliderLog` for `edgeRadius`, plain `ModulatableSlider` for everything else, `ImGui::SliderInt` for `marchSteps`, `ImGui::Combo` for `shape` with the three-option string `"Octahedron\0Dodecahedron\0Icosahedron\0"`.
- Bridge functions `SetupFrameTunnel` and `SetupFrameTunnelBlend` are NON-static (referenced by the registration macro). `DrawFrameTunnelParams` and the `DrawOutput_frameTunnel` generated by `STANDARD_GENERATOR_OUTPUT` are static.
- `GetFrameTunnelEffect(pe)` returns `(FrameTunnelEffect *)pe->effectStates[TRANSFORM_FRAME_TUNNEL_BLEND]`.
- Registration macro:
  ```cpp
  // clang-format off
  STANDARD_GENERATOR_OUTPUT(frameTunnel)
  REGISTER_GENERATOR(TRANSFORM_FRAME_TUNNEL_BLEND, FrameTunnel, frameTunnel,
                     "Frame Tunnel", SetupFrameTunnelBlend, SetupFrameTunnel, 10,
                     DrawFrameTunnelParams, DrawOutput_frameTunnel)
  // clang-format on
  ```
- Includes match the generator pattern in conventions.md (apollonian_tunnel.cpp is the canonical example).

**Verify**: `cmake.exe --build build` compiles; the effect appears under GENERATORS > Geometric in the UI; enabling the checkbox loads the shader without errors.

---

#### Task 2.2: Create shader

**Files**: `shaders/frame_tunnel.fs`
**Depends on**: Task 1.1

**Do**: Write the shader source from Design > Algorithm verbatim. The attribution comment block is REQUIRED (gaz, CC BY-NC-SA 3.0 Unported, Shadertoy URL `https://www.shadertoy.com/view/st2GRd`, modification note). The shader is a mechanical transcription of the Icosahedron Frame reference with the substitution table applied: open the Algorithm section and type out exactly what's there. Do NOT rewrite or reinterpret. Do NOT add tonemap (no `tanh`, no `pow` gamma), do NOT add a debug overlay, do NOT add fallback uniforms.

**Verify**: `cmake.exe --build build` compiles; raylib loads the shader at runtime without GLSL errors when the effect is enabled (check stderr for `WARNING: SHADER:`).

---

#### Task 2.3: Wire config

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1

**Do**: Three edits, each in alphabetical-ish position to match surrounding entries:
1. Add `#include "effects/frame_tunnel.h"` in the include block (after `effects/fracture_grid.h`).
2. Add enum entry `TRANSFORM_FRAME_TUNNEL_BLEND,` to `TransformEffectType`. Insert near the other recent generator additions, just before `TRANSFORM_ACCUM_COMPOSITE`. Do NOT touch the `TransformOrderConfig::order` array constructor: it auto-fills from the enum range, so the new entry lands automatically.
3. Add config member `FrameTunnelConfig frameTunnel;` to `EffectConfig` near `apollonianTunnel` / `jellyfish`. A single one-line `// Frame Tunnel (...)` comment is fine; do not write multi-paragraph comments.

**Verify**: `cmake.exe --build build` compiles; `TRANSFORM_EFFECT_COUNT` advances by 1.

---

#### Task 2.4: Wire serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Task 1.1

**Do**: Three edits:
1. Add `#include "effects/frame_tunnel.h"` in the per-effect include block (alphabetical position, near `effects/fracture_grid.h` and `effects/galaxy.h`).
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FrameTunnelConfig, FRAME_TUNNEL_CONFIG_FIELDS)` near the other generator macros (alphabetical near `FractalTreeConfig` / `GalaxyConfig`).
3. Add `X(frameTunnel)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table, on the line that already handles other generators (any continuation line is fine; the macro doesn't care about ordering within itself).

**Verify**: `cmake.exe --build build` compiles; saving a preset includes the `frameTunnel` block; loading a preset round-trips the values.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings.
- [ ] Frame Tunnel appears under GENERATORS > Geometric in the Effects window.
- [ ] Enabling the effect shows the icosahedral wireframe tunnel with default tumble (rotateSpeedY = 0.5).
- [ ] Switching the Shape combo to Octahedron / Dodecahedron yields visibly distinct geometries.
- [ ] Setting `zoomSpeed` to a negative value reverses the tunnel direction.
- [ ] FFT brightness modulation visibly responds to audio (audible signal -> brighter edges).
- [ ] Modulating `edgeRadius` from a slow LFO produces breathing-edge behavior.
- [ ] Preset save/load round-trips the full config.
- [ ] No `log(0)` or NaN artifacts under default parameters with audio playing for 60 seconds.

---

## Implementation Notes

Deltas from the plan as written to what shipped:

- **Display name**: "Frame Tunnel" -> "Frame Recursion". The visual is more recursive than tunnel-like. All identifiers (file names, struct names, enum, field, param prefix, display string) renamed.
- **`glow` parameter removed, replaced with `glowIntensity`**: original plan exposed the inner exp falloff coefficient (`exp(-glow * i*i * e)`) as a 0.01-0.2 slider. That control was inverted (lower value = more visible glow) and produced no usable headroom. Shipped: hardcoded falloff at `0.05` matching the reference, plus a final-output multiplier `glowIntensity` (range 0.0-2.0, default 1.0) on the accumulator. Naming and direction are now intuitive (higher = brighter).
- **No tonemap**: tried `tanh(accumulator * glowIntensity)` to soft-clip; rejected because tanh desaturates colors near the ceiling. Shipped without any tonemap. `glowIntensity` capped at 2.0 to keep saturation in check.
- **`zoomPhase` wraps mod 1.0**: original plan wrapped all four phases mod `2*PI_F`. The shader applies `fract(log(p.z) - zoomPhase)` which is mod-1, so a `2*PI_F` CPU wrap created a visible jump cut every wrap cycle. Rotate phases still wrap mod `2*PI_F`; `zoomPhase` wraps mod `1.0`.
- **Color mode selector added**: not in original plan. Three modes:
  - `Banded` (default, mode 0): `t = fract(dot(p,p) * 0.5)` - barber-pole stripes along edges (the gaz-family palette behavior).
  - `Layered` (mode 1): `t = (i+1) / marchSteps` - per-iteration normalized index. Edges at different convergent `i` get different gradient bands; within a single edge, a single color. Onion-shell appearance.
  - `Depth` (mode 2): `t = fract(log(g + 1.0))` - log-spaced raymarch distance. Different shells of the recursive tunnel get different gradient samples. Period `e` units of march distance per cycle, mirroring the tunnel's own self-similar log scaling.
- **`marchSteps` UI bound**: corrected from accidental `32-160` to plan-spec `32-128`.
- **Modulation registrations**: 12 IDs (rotateSpeedX/Y/Z, zoomSpeed, edgeRadius, glowIntensity, baseFreq, maxFreq, gain, curve, baseBright, blendIntensity). `shape`, `marchSteps`, and `colorMode` are discrete combos/perf knobs and not registered.
