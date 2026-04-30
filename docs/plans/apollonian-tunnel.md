# Apollonian Tunnel

Raymarched flythrough of an Apollonian gasket fractal carved into a sinusoidal tunnel. Camera snakes through the tube while volumetric glow accumulates along proximity to the fractal surface; FFT brightness is mapped along tunnel depth so the viewer flies through frequency space. A blend parameter morphs between carved tunnel and open infinite lattice.

**Research**: `docs/research/apollonian_tunnel.md`

## Design

### Types

`src/effects/apollonian_tunnel.h` — declares `ApollonianTunnelConfig`, `ApollonianTunnelEffect`, and lifecycle prototypes.

```cpp
struct ApollonianTunnelConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 96;        // Raymarch iterations per pixel (48-128)
  int fractalIters = 6;       // Apollonian fold-invert iterations (3-10)
  float preScale = 16.0f;     // Fractal feature size relative to world (4.0-32.0)
  float verticalOffset = -2.0f; // Vertical shift through the lattice (-4.0 to 4.0)
  float tunnelRadius = 2.0f;  // Carved tunnel cylinder radius (0.5-4.0)

  // Tunnel
  float tunnelBlend = 0.0f;   // 0 = carved tunnel, 1 = open lattice (0.0-1.0)
  float pathAmplitude = 16.0f; // Lateral weave amplitude (4.0-32.0)
  float pathFreq = 0.07f;     // Lateral weave frequency along Z (0.01-0.2)

  // Animation (CPU-accumulated phases)
  float flySpeed = 1.5f;      // Forward travel speed (0.0-5.0)
  float rollSpeed = 0.15f;    // Camera roll rate rad/s (-2.0 to 2.0)
  float rollAmount = 0.3f;    // Camera roll intensity (0.0-1.0)

  // Glow
  float glowIntensity = 7e4f; // Volumetric glow brightness, log scale (5e3-1e6)
  float fogDensity = 40.0f;   // Depth fog falloff rate (10.0-100.0)
  float depthCycle = 12.6f;   // Gradient/FFT cycle period along depth (2.0-50.0)

  // Audio (FFT)
  float baseFreq = 55.0f;     // Lowest mapped pitch (27.5-440)
  float maxFreq = 14000.0f;   // Highest mapped pitch (1000-16000)
  float gain = 2.0f;          // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;         // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f;   // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define APOLLONIAN_TUNNEL_CONFIG_FIELDS                                        \
  enabled, marchSteps, fractalIters, preScale, verticalOffset, tunnelRadius,   \
      tunnelBlend, pathAmplitude, pathFreq, flySpeed, rollSpeed, rollAmount,   \
      glowIntensity, fogDensity, depthCycle, baseFreq, maxFreq, gain, curve,   \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ApollonianTunnelEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;  // accumulates flySpeed * deltaTime
  float rollPhase; // accumulates rollSpeed * deltaTime
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int flyPhaseLoc;
  int rollPhaseLoc;
  int marchStepsLoc;
  int fractalItersLoc;
  int preScaleLoc;
  int verticalOffsetLoc;
  int tunnelRadiusLoc;
  int tunnelBlendLoc;
  int pathAmplitudeLoc;
  int pathFreqLoc;
  int rollAmountLoc;
  int glowIntensityLoc;
  int fogDensityLoc;
  int depthCycleLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ApollonianTunnelEffect;

bool ApollonianTunnelEffectInit(ApollonianTunnelEffect *e,
                                const ApollonianTunnelConfig *cfg);
void ApollonianTunnelEffectSetup(ApollonianTunnelEffect *e,
                                 ApollonianTunnelConfig *cfg, float deltaTime,
                                 const Texture2D &fftTexture);
void ApollonianTunnelEffectUninit(ApollonianTunnelEffect *e);
void ApollonianTunnelRegisterParams(ApollonianTunnelConfig *cfg);
```

### Algorithm

The shader transcribes the reference verbatim with substitutions applied to each line. The reference's macros (`T`, `P`, `R`, `N`) are inlined as functions for clarity.

`shaders/apollonian_tunnel.fs`:

```glsl
// Based on "Fractal Tunnel Flight" by diatribes
// https://www.shadertoy.com/view/WXyfWh
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized iters/scale/path/roll/glow, gradient LUT replaces
//           cosine palette, FFT brightness on shared depth-cycle t index
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float flyPhase;
uniform float rollPhase;

uniform int marchSteps;
uniform int fractalIters;
uniform float preScale;
uniform float verticalOffset;
uniform float tunnelRadius;

uniform float tunnelBlend;
uniform float pathAmplitude;
uniform float pathFreq;
uniform float rollAmount;

uniform float glowIntensity;
uniform float fogDensity;
uniform float depthCycle;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// Reference T macro: drives camera advance and speed wobble
float pathTime() {
    return flyPhase * 1.5 + 5.0 + 1.5 * sin(flyPhase * 0.5);
}

// Reference P(z) macro: tunnel centerline (sinusoidal X weave along Z)
vec3 pathPos(float z) {
    return vec3(cos(z * pathFreq) * pathAmplitude, 0.0, z);
}

// Reference R(a) macro: 2D rotation matrix
mat2 rot2D(float a) {
    return mat2(cos(a + vec4(0.0, 33.0, 11.0, 0.0)));
}

// Apollonian gasket distance estimator: box-fold + sphere-inversion (Knighty 2010)
float apollonian(vec3 p) {
    float w = 1.0;
    vec3 b = vec3(0.5, 1.0, 1.5);
    p.y += verticalOffset;
    p.yz = p.zy;
    p /= preScale;
    for (int i = 0; i < fractalIters; i++) {
        p = mod(p + b, 2.0 * b) - b;
        float s = 2.0 / dot(p, p);
        p *= s;
        w *= s;
    }
    return length(p) / w * 6.0;
}

// Tunnel SDF intersected with fractal, blended toward open lattice
float mapTunnel(vec3 p) {
    float tunnel = tunnelRadius - length((p - pathPos(p.z)).xy);
    float fractal = apollonian(p);
    return mix(max(tunnel, fractal), fractal, tunnelBlend);
}

void main() {
    // Centered coords matching reference: (u - r/2) / r.y
    vec2 u = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    float T = pathTime();
    vec3 camPos = pathPos(T * 2.0);
    vec3 camTarget = pathPos(T * 2.0 + 7.0);
    vec3 Z = normalize(camTarget - camPos);
    vec3 X = normalize(vec3(Z.z, 0.0, -Z.x));
    vec3 D = normalize(vec3(rot2D(sin(rollPhase) * rollAmount) * u, 1.0)
                       * mat3(-X, cross(X, Z), Z));

    vec3 p = camPos;
    vec3 accumulator = vec3(0.0);
    float d = 0.0;
    float s = 0.0;

    for (int iter = 0; iter < marchSteps; iter++) {
        p += D * s;
        s = mapTunnel(p) * 0.8;
        d += s;

        // Shared color/audio index along depth axis
        float t = fract((p.z - camPos.z) / depthCycle);
        vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) {
            energy = texture(fftTexture, vec2(bin, 0.5)).r;
        }
        float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
        float brightness = baseBright + mag;

        accumulator += baseColor * brightness / max(s, 0.0003);
    }

    // tanh soft-clip with depth fog brightening (per-channel separation removed:
    // the gradient handles all coloring in this port)
    vec3 toned = tanh(accumulator / d / glowIntensity * exp(d / fogDensity));
    finalColor = vec4(toned, 1.0);
}
```

Notes on the transcription:

- `iTime` is replaced everywhere by `flyPhase` (CPU-accumulated `flySpeed * deltaTime`). The reference's `T` macro keeps the `*1.5 + 5 + 1.5*sin(...*0.5)` form so the visual speed wobble is preserved.
- The reference `R(a)` macro uses a vec4 trick that the GLSL compiler folds into a 2x2; transcribed verbatim.
- The reference's `vec3(Z.z, 0, -Z)` is non-portable swizzle ambiguity; the standard interpretation `vec3(Z.z, 0.0, -Z.x)` is used (the reference's intent is the standard "right vector from forward + world up").
- `p.y -= 2.` becomes `p.y += verticalOffset`. With default `verticalOffset = -2.0`, behavior matches reference.
- `2. - length(...)` becomes `tunnelRadius - length(...)`.
- `cos(z * .07) * 16.` becomes `cos(z * pathFreq) * pathAmplitude`.
- Reference roll `R(sin(iTime*.15)*.3)` becomes `rot2D(sin(rollPhase) * rollAmount)`.
- Cosine palette `(1. + cos(.05*i + .5*p.z + vec4(6,4,2,0))) / max(s, .0003)` is replaced with `baseColor * brightness / max(s, 0.0003)` where `baseColor` comes from the gradient LUT and `brightness = baseBright + mag` from the FFT lookup, both sampled at `t = fract((p.z - camPos.z) / depthCycle)`.
- Tonemap `tanh(o/d/7e4*exp(vec4(3,2,1,0)*d/4e1))` becomes `tanh(accumulator / d / glowIntensity * exp(d / fogDensity))`. The per-channel `vec4(3,2,1,0)` separation is removed because the gradient LUT now handles all coloring.
- The 0.8 step multiplier in `mapTunnel(p) * 0.8` is preserved as a safety factor for the non-Euclidean DE produced by sphere inversion.
- The fold box `b = vec3(0.5, 1.0, 1.5)` is intentionally not exposed; it defines the Apollonian gasket character.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| marchSteps | int | 48-128 | 96 | no | March Steps |
| fractalIters | int | 3-10 | 6 | no | Iterations |
| preScale | float | 4.0-32.0 | 16.0 | yes | Pre Scale |
| verticalOffset | float | -4.0 to 4.0 | -2.0 | yes | Vertical Offset |
| tunnelRadius | float | 0.5-4.0 | 2.0 | yes | Tunnel Radius |
| tunnelBlend | float | 0.0-1.0 | 0.0 | yes | Tunnel Blend |
| pathAmplitude | float | 4.0-32.0 | 16.0 | yes | Path Amp |
| pathFreq | float | 0.01-0.2 | 0.07 | yes | Path Freq |
| flySpeed | float | 0.0-5.0 | 1.5 | yes | Fly Speed |
| rollSpeed | float | -2.0 to 2.0 | 0.15 | yes | Roll Speed |
| rollAmount | float | 0.0-1.0 | 0.3 | yes | Roll Amount |
| glowIntensity | float | 5e3-1e6 | 7e4 | yes | Glow Intensity |
| fogDensity | float | 10.0-100.0 | 40.0 | yes | Fog Density |
| depthCycle | float | 2.0-50.0 | 12.6 | yes | Depth Cycle |
| baseFreq | float | 27.5-440 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | (output section) |

UI widget mapping:
- `glowIntensity`: `ModulatableSliderLog` (wide log range)
- `marchSteps`, `fractalIters`: `ImGui::SliderInt` (true integers, no modulation)
- All other floats: `ModulatableSlider`
- Color/output section emitted by `STANDARD_GENERATOR_OUTPUT(apollonianTunnel)`

UI section ordering (per Signal Stack):
1. **Audio** — baseFreq, maxFreq, gain, curve (label "Contrast"), baseBright
2. **Geometry** — marchSteps, fractalIters, preScale, verticalOffset, tunnelRadius
3. **Tunnel** — tunnelBlend, pathAmplitude, pathFreq
4. **Animation** — flySpeed, rollSpeed, rollAmount
5. **Glow** — glowIntensity, fogDensity, depthCycle

Color/output section appended via `STANDARD_GENERATOR_OUTPUT` macro.

### Constants

- Enum name: `TRANSFORM_APOLLONIAN_TUNNEL_BLEND`
- Display name: `"Apollonian Tunnel"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: `13` (Atmosphere — same as Voxel March, Spiral March, Dream Fractal)
- Registration macro: `REGISTER_GENERATOR` (Init signature is `Init(Effect*, Config*)`; no resize, no custom render)
- Field name: `apollonianTunnel`
- Param prefix: `"apollonianTunnel."`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create header

**Files**: `src/effects/apollonian_tunnel.h`
**Creates**: `ApollonianTunnelConfig`, `ApollonianTunnelEffect` types and lifecycle prototypes — these are needed by every Wave 2 task.

**Do**: Create the header exactly as specified in the Design > Types section. Follow the structure of `src/effects/spiral_march.h` (same generator pattern: ColorConfig embed, EffectBlendMode + blendIntensity, ColorLUT pointer, two CPU-accumulated phases, all uniform locations as int fields). Header guard `APOLLONIAN_TUNNEL_H`. Top-of-file comment naming the effect and its visual technique in one line. Top-level comment includes attribution to "Fractal Tunnel Flight" by diatribes.

Includes:
- `"raylib.h"`
- `"render/blend_mode.h"`
- `"render/color_config.h"`
- `<stdbool.h>`

**Verify**: `cmake.exe --build build` compiles. (No callers yet, but the header should be self-contained.)

---

### Wave 2: Parallel Implementation

All Wave 2 tasks depend on Wave 1 complete. They touch disjoint files and run in parallel.

#### Task 2.1: Create effect implementation with colocated UI

**Files**: `src/effects/apollonian_tunnel.cpp`
**Depends on**: Wave 1 complete

**Do**: Implement the lifecycle functions and colocated UI. Follow the structure of `src/effects/spiral_march.cpp` (most recent generator-with-FFT sibling):

- `ApollonianTunnelEffectInit`: `LoadShader(NULL, "shaders/apollonian_tunnel.fs")` → cache all uniform locations → `ColorLUTInit(&cfg->gradient)` (unload shader on LUT failure) → zero-init `flyPhase` and `rollPhase`.
- `ApollonianTunnelEffectSetup`: accumulate `flyPhase += cfg->flySpeed * deltaTime` and `rollPhase += cfg->rollSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind `resolution`, `fftTexture`, `sampleRate` (from `AUDIO_SAMPLE_RATE`), the two phases, then bind all config uniforms via a `static BindUniforms` helper. Bind `gradientLUT` last via `SetShaderValueTexture`.
- `ApollonianTunnelEffectUninit`: `UnloadShader` + `ColorLUTUninit`.
- `ApollonianTunnelRegisterParams`: register every modulatable param from the parameter table with the exact bounds shown. Register `apollonianTunnel.blendIntensity` last (range 0.0-5.0). Do NOT register `marchSteps` or `fractalIters` (true integers). Audio params use 27.5-440 / 1000-16000 / 0.1-10 / 0.1-3 / 0-1 ranges.
- `SetupApollonianTunnel(PostEffect *pe)` (non-static bridge): calls `ApollonianTunnelEffectSetup(&pe->apollonianTunnel, &pe->effects.apollonianTunnel, pe->currentDeltaTime, pe->fftTexture)`.
- `SetupApollonianTunnelBlend(PostEffect *pe)` (non-static bridge): calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.apollonianTunnel.blendIntensity, pe->effects.apollonianTunnel.blendMode)`.
- `// === UI ===` section: `static void DrawApollonianTunnelParams(EffectConfig*, const ModSources*, ImU32)`. Sections in order: Audio → Geometry → Tunnel → Animation → Glow. Use `ImGui::SliderInt` for `marchSteps` and `fractalIters`. Use `ModulatableSliderLog` for `glowIntensity`. All other floats use `ModulatableSlider` with the format strings shown below. Audio section uses the standard 5-slider layout from `docs/conventions.md` and `voxel_march.cpp`/`spiral_march.cpp`.
- Bottom of file (inside `// clang-format off` / `// clang-format on`): `STANDARD_GENERATOR_OUTPUT(apollonianTunnel)` then `REGISTER_GENERATOR(TRANSFORM_APOLLONIAN_TUNNEL_BLEND, ApollonianTunnel, apollonianTunnel, "Apollonian Tunnel", SetupApollonianTunnelBlend, SetupApollonianTunnel, 13, DrawApollonianTunnelParams, DrawOutput_apollonianTunnel)`.

Format strings: `"%.1f"` for baseFreq, gain; `"%.0f"` for maxFreq; `"%.2f"` for curve, baseBright, preScale, verticalOffset, tunnelRadius, tunnelBlend, pathAmplitude, flySpeed, rollSpeed, rollAmount, fogDensity, depthCycle; `"%.3f"` for pathFreq; `"%.0f"` for glowIntensity (log slider).

Includes (alphabetical within groups, per conventions):
- Own header: `"apollonian_tunnel.h"`
- Project: `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`
- ImGui/UI: `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
- System: `<stddef.h>`

**Verify**: `cmake.exe --build build` compiles. Effect appears in the Atmosphere generator section with badge "GEN".

#### Task 2.2: Create fragment shader

**Files**: `shaders/apollonian_tunnel.fs`
**Depends on**: Wave 1 complete (independent of Task 2.1; runs in parallel)

**Do**: Implement the shader exactly as written in the Design > Algorithm section. Attribution comment block above `#version 330`. GLSL 330. Use `for (int i = 0; i < fractalIters; i++)` and `for (int iter = 0; iter < marchSteps; iter++)` — GLSL 330 supports dynamic loop bounds against uniforms; do NOT hardcode max + early break.

Do not add tonemap beyond the `tanh` already shown. Do not add gamma. Do not change the 0.8 step multiplier. Do not expose the fold-box `b` constant.

**Verify**: Shader compiles at runtime when the effect is enabled (raylib `LoadShader` returns non-zero `id`). Visually: enabling the effect produces a fractal tunnel flythrough.

#### Task 2.3: Wire into effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**: Four edits, none overlap with other Wave 2 tasks:

1. Add `#include "effects/apollonian_tunnel.h"` in the alphabetical effect-include block (between `anamorphic_streak.h` and `arc_strobe.h`).
2. Add `TRANSFORM_APOLLONIAN_TUNNEL_BLEND` enum entry. Place it immediately before `TRANSFORM_ACCUM_COMPOSITE`, after `TRANSFORM_SPIRAL_MARCH_BLEND` (matches the chronological tail-append pattern used for recent additions: `TRANSFORM_RANDOM_VOLUMETRIC`, `TRANSFORM_LICHEN`, `TRANSFORM_SPIRAL_MARCH_BLEND`).
3. The `TransformOrderConfig::order[]` array is initialized programmatically by the `TransformOrderConfig::TransformOrderConfig()` body — the loop fills it from the enum, so no manual order entry is needed. (The "order array" reminder in the add-effect skill applies when there is an explicit initializer; this codebase uses a default-fill loop. Verify by reading the existing constructor and confirm.)
4. Add `ApollonianTunnelConfig apollonianTunnel;` field on `EffectConfig`. Place it at the end of the struct, after `LichenConfig lichen;` and before `TransformOrderConfig transformOrder;`. One-line comment above describing the effect.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Wire into post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Do**: Two edits:

1. Add `#include "effects/apollonian_tunnel.h"` alphabetically with the other effect header includes.
2. Add `ApollonianTunnelEffect apollonianTunnel;` member at the end of the per-effect runtime state block (place it after `SpiralMarchEffect spiralMarch;` near line 350, or wherever recent additions land — match the same group as `voxelMarch` and `spiralMarch`).

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Wire into effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**: Three edits:

1. Add `#include "effects/apollonian_tunnel.h"` alphabetically.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ApollonianTunnelConfig, APOLLONIAN_TUNNEL_CONFIG_FIELDS)` in the alphabetically-ordered macro block.
3. Add `X(apollonianTunnel)` to the `EFFECT_CONFIG_FIELDS(X)` macro table. Append at the end alongside the most recent additions (`X(spiralMarch)` is currently last).

**Verify**: `cmake.exe --build build` compiles. Save a preset with the effect enabled and reload it — settings round-trip.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears under the Atmosphere generator section with badge "GEN" and display name "Apollonian Tunnel"
- [ ] Enabling the effect produces a recognizable Apollonian-fractal tunnel flythrough
- [ ] All modulatable params register with the modulation engine and accept routes
- [ ] FFT brightness modulates along depth (audio reactivity visible at non-zero `gain`)
- [ ] Camera roll responds to `rollSpeed` and `rollAmount`
- [ ] Preset save/load round-trips the config
- [ ] Static analysis passes (`/lint` if requested)

---

## Implementation Notes

Recording actual shipped behavior, which diverged from the original spec.

### Removed parameters

- `tunnelBlend` removed. SDF is unconditional `max(tunnel, fractal)`. The mix-toward-bare-fractal mode looked bad and felt binary.
- `fogDensity` removed. The depth-fog `exp(d/fogDensity)` term in the original tonemap caused background pixels (where `d` grows large) to blow into pure red and made the slider counter-intuitive. Without per-channel separation it had no salvageable purpose.

### Tonemap

Final form: `tanh(accumulator * glowIntensity * 0.0001 / max(d, 1.0))`.

- `glowIntensity` is a multiplicative gain. Range `0.0` to `5.0`, default `1.0`. Higher = brighter. Linear slider.
- The internal `0.0001` constant is a fixed pre-attenuation that lets the multiplier land in a usable user range; `max(d, 1.0)` prevents stuck-ray division blowout.
- The per-channel `vec3(3,2,1) * exp(d/fog)` boost is gone. The gradient LUT carries all coloring.

### Per-step color/audio index

Each march step samples both the gradient LUT and the FFT at the same `t`, computed as `fract(0.05 * float(iter) / 2π + p.z / depthCycle)`. Iteration count and world-space depth both contribute to the gradient cycle. `depthCycle` controls the spatial period (default 12.6, matching the cosine-palette period).

FFT lookup uses 4-sample band averaging via `sampleFFTBand(t, t + 1/marchSteps)`. Helper ported from `shaders/voxel_march.fs`. Single-point sampling aliased on narrow tones. Same helper added to `shaders/spiral_march.fs` for consistency.

### Camera animation simplified

- Speed wobble dropped. `T = flyPhase + 5`. No more sinusoidal speed surge on top of the linear fly motion.
- Roll is continuous, not oscillating. `rot2D(rollPhase * rollAmount)`. The phase accumulates monotonically.

### Known UX limitation: path knobs move the camera

`pathAmplitude` and `pathFreq` define the tunnel centerline AND the camera position (`camPos = pathPos(T*2)`). Adjusting either knob teleports the camera laterally to a different slice of the lattice — visually it reads as "the fractal jumped" even though `apollonian(p)` is unchanged. Decoupling was considered and rejected (camera would exit the tube). Lissajous-driven lateral motion in other effects doesn't have this issue because those effects don't carve a path-following tube around the camera.

### Tuning history (visual iteration)

Multiple tonemap formulations were tried before settling on the final form:
- `tanh(accumulator / d / glowIntensity * exp(d / fogDensity))` — inverted slider semantics, blown-out red background.
- `tanh(accumulator * glowIntensity / d * exp(-d * fogDensity))` — fixed slider direction but exp brightening still produced uneven saturation.
- `tanh(accumulator / d / glowIntensity)` — dim, no exp boost.
- `tanh(accumulator / d / glowIntensity * exp(vec3(3,2,1) * d / fogDensity))` — kept channel separation but produced strong red bias at depth.
- `tanh(accumulator * glowIntensity / d)` — too bright at any value of glow.
- `min(accumulator * glowIntensity * scale / max(d, 1.0), vec3(1.0))` — hard per-channel clamp; produced primary-saturation magenta/cyan/white at default settings because all three channels exceeded 1.0 simultaneously in dense regions.
- Channel-max normalize (`raw / max(maxC, 1.0)`) — every pixel ended up peak-normalized, image went posterized.
- Density-into-gradient-t (single gradient sample per pixel via `1 - exp(-density * scale)`) — collapsed to flat single-color regions because most pixels saturated the exp curve.

Final form (`tanh(accumulator * glowIntensity * 0.0001 / max(d, 1.0))`) accepts that some saturation may occur in extreme regions; FFT brightness pumping the accumulator is bounded by `tanh` per channel, and the gradient's own colors define what saturated pixels look like.
