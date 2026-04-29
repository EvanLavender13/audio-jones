# Lichen

Reaction-diffusion generator with three competing species locked in a rock-paper-scissors cycle. Each species runs the Gray-Scott two-chemical reaction independently, but their kill rates are coupled to their cyclic predator and prey, so no color can dominate. An 8-octave sinusoidal coordinate warp stirs the territory boundaries; a per-species log-spaced FFT band modulates the brightness of each color slice so the surface pulses with the music.

**Research**: `docs/research/lichen.md`

**Sibling pattern**: `src/effects/curl_advection.cpp` (`REGISTER_GENERATOR_FULL` with state shader + color shader, ping-pong state textures, custom render callback)

## Design

### Texture Packing

Three species are packed into two RGBA32F state textures, each ping-ponged:

- `statePingPong0[2]` — species 0 in `.rg` (u0, v0), species 1 in `.ba` (u1, v1)
- `statePingPong1[2]` — species 2 in `.rg` (u2, v2); `.ba` unused
- `colorRT` — single RGBA32F render texture for the color output pass

`u` is the activator/substrate (default 1, depleted inside blobs); `v` is the inhibitor/visible chemical (default 0, raised inside blobs).

### Render Flow Per Frame

Inside the custom `RenderLichen(PostEffect *pe)` callback:

1. Compute `writeIdx0 = 1 - readIdx0`, `writeIdx1 = 1 - readIdx1`.
2. **Pass 0** — output species 0+1 to `statePingPong0[writeIdx0]`:
   - `BeginTextureMode(statePingPong0[writeIdx0])`, `rlDisableColorBlend()`, `BeginShaderMode(stateShader)`
   - Bind `stateTex1Loc` to `statePingPong1[readIdx1].texture`
   - Bind `passIndex = 0` (int uniform)
   - `RenderUtilsDrawFullscreenQuad(statePingPong0[readIdx0].texture, w, h)` — texture0 auto-bound to species 0+1 read buffer
   - `EndShaderMode()`, `rlEnableColorBlend()`, `EndTextureMode()`
3. **Pass 1** — output species 2 to `statePingPong1[writeIdx1]`:
   - `BeginTextureMode(statePingPong1[writeIdx1])`, `rlDisableColorBlend()`, `BeginShaderMode(stateShader)`
   - Bind `stateTex1Loc` to `statePingPong1[readIdx1].texture`
   - Bind `passIndex = 1`
   - `RenderUtilsDrawFullscreenQuad(statePingPong0[readIdx0].texture, w, h)` — same auto-bound source as pass 0; both passes read the SAME read buffers so coupling is consistent within a frame
   - `EndShaderMode()`, `rlEnableColorBlend()`, `EndTextureMode()`
4. **Color pass** — output to `colorRT`:
   - `BeginTextureMode(colorRT)`, `BeginShaderMode(shader)`
   - Bind `stateTex0Loc = statePingPong0[writeIdx0].texture` (just written)
   - Bind `stateTex1Loc = statePingPong1[writeIdx1].texture`
   - Bind `gradientLUTLoc`, `fftTextureLoc`, plus all scalar audio/brightness uniforms
   - `RenderUtilsDrawFullscreenQuad(statePingPong0[writeIdx0].texture, w, h)` (any RT — texture0 unused)
   - `EndShaderMode()`, `EndTextureMode()`
5. Flip indices: `readIdx0 = writeIdx0; readIdx1 = writeIdx1`.

The pipeline then runs the `EFFECT_FLAG_BLEND` composite pass via `SetupLichenBlend` which calls `BlendCompositorApply(pe->blendCompositor, pe->lichen.colorRT.texture, blendIntensity, blendMode)`.

### Types

```cpp
struct LichenConfig {
  bool enabled = false;

  // Reaction-diffusion
  float feedRate = 0.019f;          // Substrate replenishment (0.005-0.08)
  float killRateBase = 0.084f;      // Base decay rate (0.01-0.12)
  float couplingStrength = 0.04f;   // Cyclic coupling strength (0.0-0.2)
  float predatorAdvantage = 1.07f;  // >1 favors prey persistence (0.8-1.5)

  // Coordinate warp
  float warpIntensity = 0.1f;       // Boundary stirring amplitude (0.0-0.5)
  float warpSpeed = 2.5f;           // Warp animation rate (0.0-5.0)

  // Diffusion
  float activatorRadius = 2.5f;     // Activator (.x) sample radius in pixels (0.5-5.0)
  float inhibitorRadius = 1.2f;     // Inhibitor (.y) sample radius in pixels (0.5-3.0)

  // Reaction
  int reactionSteps = 25;           // Reaction iterations per frame (5-50)
  float reactionRate = 0.4f;        // Time step per iteration (0.1-0.8)

  // Output level
  float brightness = 2.0f;          // LUT-color amplifier (0.5-4.0)

  // Audio (FFT)
  float baseFreq = 55.0f;           // FFT low bound Hz (27.5-440)
  float maxFreq = 14000.0f;         // FFT high bound Hz (1000-16000)
  float gain = 2.0f;                // FFT gain (0.1-10)
  float curve = 1.5f;               // FFT contrast curve (0.1-3)
  float baseBright = 0.15f;         // Brightness floor (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define LICHEN_CONFIG_FIELDS                                                   \
  enabled, feedRate, killRateBase, couplingStrength, predatorAdvantage,        \
      warpIntensity, warpSpeed, activatorRadius, inhibitorRadius,              \
      reactionSteps, reactionRate, brightness, baseFreq, maxFreq, gain, curve, \
      baseBright, gradient, blendMode, blendIntensity

typedef struct LichenEffect {
  Shader stateShader;                  // Reaction-diffusion + warp + diffusion shader
  Shader shader;                       // Color output shader (sampled by compositor)
  ColorLUT *gradientLUT;
  RenderTexture2D statePingPong0[2];   // RGBA32F: species 0 (rg) + species 1 (ba)
  RenderTexture2D statePingPong1[2];   // RGBA32F: species 2 (rg)
  RenderTexture2D colorRT;             // RGBA32F: per-frame color output
  int readIdx0;
  int readIdx1;

  float time;                          // CPU phase accumulator for warp animation

  // State shader uniform locations
  int stateResolutionLoc;
  int stateTimeLoc;
  int statePassIndexLoc;
  int stateFeedRateLoc;
  int stateKillRateBaseLoc;
  int stateCouplingStrengthLoc;
  int statePredatorAdvantageLoc;
  int stateWarpIntensityLoc;
  int stateWarpSpeedLoc;
  int stateActivatorRadiusLoc;
  int stateInhibitorRadiusLoc;
  int stateReactionStepsLoc;
  int stateReactionRateLoc;
  int stateTex1Loc;                    // Other state texture (species 2)

  // Color shader uniform locations
  int colorResolutionLoc;
  int colorBrightnessLoc;
  int colorStateTex0Loc;
  int colorStateTex1Loc;
  int colorGradientLUTLoc;
  int colorFftTextureLoc;
  int colorSampleRateLoc;
  int colorBaseFreqLoc;
  int colorMaxFreqLoc;
  int colorGainLoc;
  int colorCurveLoc;
  int colorBaseBrightLoc;
} LichenEffect;
```

### Algorithm

#### State Shader (`shaders/lichen_state.fs`)

Computes one frame's update for all three species and outputs either species 0+1 (passIndex=0) or species 2 (passIndex=1) to the bound render target.

```glsl
// Based on "rps cell nomming :3" by frisk256
// https://www.shadertoy.com/view/NcjXRh
// License: CC BY-NC-SA 3.0 Unported
// Modified: 3 species packed into 2 RGBA textures, single shader with passIndex,
//           parameterized via uniforms (feed/kill/coupling/warp/diffusion/reaction)
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // statePingPong0[readIdx0] (species 0 rg, species 1 ba)
uniform sampler2D stateTex1;      // statePingPong1[readIdx1] (species 2 rg)
uniform vec2 resolution;
uniform float time;
uniform int passIndex;            // 0 = output species 0+1; 1 = output species 2

uniform float feedRate;
uniform float killRateBase;
uniform float couplingStrength;
uniform float predatorAdvantage;
uniform float warpIntensity;
uniform float warpSpeed;
uniform float activatorRadius;
uniform float inhibitorRadius;
uniform int reactionSteps;
uniform float reactionRate;

const float PI = 3.14159265358979;

void main() {
    vec2 r = resolution;
    vec2 p = fragTexCoord * r;

    // 8-octave sinusoidal coordinate warp - deterministic q sequence from reference
    float q = 1.0;
    for (int i = 0; i < 8; i++) {
        p.x += sin(p.y / r.y * PI * 2.0 * q + time * (fract(q / 4.0) - 0.3) * warpSpeed) / q * warpIntensity;
        p.y -= sin(p.x / r.x * PI * 2.0 * q + time * (fract(q / 5.0) - 0.2) * warpSpeed) / q * warpIntensity;
        q *= -1.681;
        q = fract(q / 10.0) * 10.0 - 5.0;
        q = floor(q) + 1.0;
    }

    // Center samples - used for cyclic coupling (predator/prey activator densities)
    vec4 self0 = texture(texture0, p / r);
    vec4 self1 = texture(stateTex1, p / r);
    float c0Center = self0.x;  // species 0 activator
    float c1Center = self0.z;  // species 1 activator
    float c2Center = self1.x;  // species 2 activator

    // Asymmetric diffusion - activator at activatorRadius, inhibitor at inhibitorRadius.
    // For each species, sum self + 4 cardinal neighbors then divide by 5.
    vec2 c0 = self0.xy;
    c0.x += texture(texture0, (p + vec2(activatorRadius, 0.0)) / r).x;
    c0.x += texture(texture0, (p + vec2(0.0, activatorRadius)) / r).x;
    c0.x += texture(texture0, (p - vec2(activatorRadius, 0.0)) / r).x;
    c0.x += texture(texture0, (p - vec2(0.0, activatorRadius)) / r).x;
    c0.y += texture(texture0, (p + vec2(inhibitorRadius, 0.0)) / r).y;
    c0.y += texture(texture0, (p + vec2(0.0, inhibitorRadius)) / r).y;
    c0.y += texture(texture0, (p - vec2(inhibitorRadius, 0.0)) / r).y;
    c0.y += texture(texture0, (p - vec2(0.0, inhibitorRadius)) / r).y;
    c0 /= 5.0;

    vec2 c1 = self0.zw;
    c1.x += texture(texture0, (p + vec2(activatorRadius, 0.0)) / r).z;
    c1.x += texture(texture0, (p + vec2(0.0, activatorRadius)) / r).z;
    c1.x += texture(texture0, (p - vec2(activatorRadius, 0.0)) / r).z;
    c1.x += texture(texture0, (p - vec2(0.0, activatorRadius)) / r).z;
    c1.y += texture(texture0, (p + vec2(inhibitorRadius, 0.0)) / r).w;
    c1.y += texture(texture0, (p + vec2(0.0, inhibitorRadius)) / r).w;
    c1.y += texture(texture0, (p - vec2(inhibitorRadius, 0.0)) / r).w;
    c1.y += texture(texture0, (p - vec2(0.0, inhibitorRadius)) / r).w;
    c1 /= 5.0;

    vec2 c2 = self1.xy;
    c2.x += texture(stateTex1, (p + vec2(activatorRadius, 0.0)) / r).x;
    c2.x += texture(stateTex1, (p + vec2(0.0, activatorRadius)) / r).x;
    c2.x += texture(stateTex1, (p - vec2(activatorRadius, 0.0)) / r).x;
    c2.x += texture(stateTex1, (p - vec2(0.0, activatorRadius)) / r).x;
    c2.y += texture(stateTex1, (p + vec2(inhibitorRadius, 0.0)) / r).y;
    c2.y += texture(stateTex1, (p + vec2(0.0, inhibitorRadius)) / r).y;
    c2.y += texture(stateTex1, (p - vec2(inhibitorRadius, 0.0)) / r).y;
    c2.y += texture(stateTex1, (p - vec2(0.0, inhibitorRadius)) / r).y;
    c2 /= 5.0;

    // Cyclic coupling on the activator (.x) channel - matches reference's GLSL truncation
    // of vec2(scalar, vec4) which takes .x of the vec4 expression.
    // species 0: predator = c2, prey = c1
    // species 1: predator = c0, prey = c2
    // species 2: predator = c1, prey = c0
    // K is frozen across the reaction loop (just like hp/hn are sampled outside the loop in reference).
    float k0 = couplingStrength * (c2Center - c1Center * predatorAdvantage) - killRateBase;
    float k1 = couplingStrength * (c0Center - c2Center * predatorAdvantage) - killRateBase;
    float k2 = couplingStrength * (c1Center - c0Center * predatorAdvantage) - killRateBase;

    // Gray-Scott reaction: du = -u*v^2 + F*(1-u); dv = +u*v^2 + K*v
    for (int n = 0; n < reactionSteps; n++) {
        c0 += (vec2(-1.0, 1.0) * c0.x * c0.y * c0.y + vec2(feedRate, k0) * vec2(1.0 - c0.x, c0.y)) * reactionRate;
        c1 += (vec2(-1.0, 1.0) * c1.x * c1.y * c1.y + vec2(feedRate, k1) * vec2(1.0 - c1.x, c1.y)) * reactionRate;
        c2 += (vec2(-1.0, 1.0) * c2.x * c2.y * c2.y + vec2(feedRate, k2) * vec2(1.0 - c2.x, c2.y)) * reactionRate;
    }

    c0 = clamp(c0, 0.0, 1.0);
    c1 = clamp(c1, 0.0, 1.0);
    c2 = clamp(c2, 0.0, 1.0);

    if (passIndex == 0) {
        finalColor = vec4(c0.x, c0.y, c1.x, c1.y);
    } else {
        finalColor = vec4(c2.x, c2.y, 0.0, 0.0);
    }
}
```

#### Color Shader (`shaders/lichen.fs`)

Maps each species to its own 1/3 slice of the gradient LUT, with a per-species log-spaced FFT band driving brightness within that slice.

```glsl
// Lichen color output - per-species LUT slice with per-band FFT brightness
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // unused (raylib auto-binds quad source)
uniform sampler2D stateTex0;      // species 0 (rg) + species 1 (ba)
uniform sampler2D stateTex1;      // species 2 (rg)
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform vec2 resolution;
uniform float brightness;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

float bandEnergy(float t0, float t1) {
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    const int BAND_SAMPLES = 4;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
    }
    return pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 s0 = texture(stateTex0, uv);
    vec4 s1 = texture(stateTex1, uv);
    float v0 = s0.y;
    float v1 = s0.w;
    float v2 = s1.y;

    const float SLICE = 1.0 / 3.0;

    float mag0 = bandEnergy(0.0,         SLICE);
    float mag1 = bandEnergy(SLICE,       2.0 * SLICE);
    float mag2 = bandEnergy(2.0 * SLICE, 1.0);

    vec3 col0 = texture(gradientLUT, vec2(0.0         + v0 * SLICE, 0.5)).rgb * v0 * (baseBright + mag0);
    vec3 col1 = texture(gradientLUT, vec2(SLICE       + v1 * SLICE, 0.5)).rgb * v1 * (baseBright + mag1);
    vec3 col2 = texture(gradientLUT, vec2(2.0 * SLICE + v2 * SLICE, 0.5)).rgb * v2 * (baseBright + mag2);

    vec3 col = brightness * (col0 + col1 + col2);
    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
```

#### CPU Seed Function (`InitializeSeed`)

Called from `LichenEffectInit` and `LichenEffectResize` to populate both state textures with the initial pattern: u=1 everywhere, plus three blob seeds at positions (0.16, 0.16), (0.5, 0.5), (0.84, 0.84) (in normalized coords) where v=1 inside a sin-modulated boundary.

Pseudocode:
```
allocate pixels0[w*h*4], pixels1[w*h*4]
dateOffset = (rand() / RAND_MAX)
for y in [0, h), x in [0, w):
    px = (float)x; py = (float)y
    v[3] = {0,0,0}
    for k in [0, 3):
        cx = w * seedPos[k]; cy = h * seedPos[k]    // seedPos[3] = {0.16, 0.5, 0.84}
        dist = sqrt((px-cx)^2 + (py-cy)^2)
        noise = sin(px*py + dateOffset) * 10.5
        if (dist + noise < 28.5):
            v[k] = 1.0
    pixels0[..] = (1.0, v[0], 1.0, v[1])
    pixels1[..] = (1.0, v[2], 0.0, 0.0)
UpdateTexture(statePingPong0[0].texture, pixels0)
UpdateTexture(statePingPong0[1].texture, pixels0)
UpdateTexture(statePingPong1[0].texture, pixels1)
UpdateTexture(statePingPong1[1].texture, pixels1)
free(pixels0); free(pixels1)
```

`28.5` and `10.5` are the literal seed-radius and noise-amplitude from the reference's `iFrame == 0` branch.

#### Texture Allocation

Use `RenderUtilsInitTextureHDR` (RGBA32F with HDR-FBO fallback) for all five render textures: `statePingPong0[0]`, `statePingPong0[1]`, `statePingPong1[0]`, `statePingPong1[1]`, `colorRT`. Match curl_advection's pattern exactly.

#### Time Accumulator

CPU-side phase, accumulated in `LichenEffectSetup`:
```
e->time += deltaTime
const float WRAP = 1000.0f
if (e->time > WRAP) e->time -= WRAP
SetShaderValue(stateShader, stateTimeLoc, &e->time, SHADER_UNIFORM_FLOAT)
```
(Pattern from `random_volumetric.cpp` lines 63-71.)

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| feedRate | float | 0.005-0.08 | 0.019 | yes | Feed Rate |
| killRateBase | float | 0.01-0.12 | 0.084 | yes | Kill Base |
| couplingStrength | float | 0.0-0.2 | 0.04 | yes | Coupling |
| predatorAdvantage | float | 0.8-1.5 | 1.07 | yes | Pred Advantage |
| warpIntensity | float | 0.0-0.5 | 0.1 | yes | Warp Intensity |
| warpSpeed | float | 0.0-5.0 | 2.5 | yes | Warp Speed |
| activatorRadius | float | 0.5-5.0 | 2.5 | yes | Activator Radius |
| inhibitorRadius | float | 0.5-3.0 | 1.2 | yes | Inhibitor Radius |
| reactionSteps | int | 5-50 | 25 | no (`SliderInt`) | Reaction Steps |
| reactionRate | float | 0.1-0.8 | 0.4 | yes | Reaction Rate |
| brightness | float | 0.5-4.0 | 2.0 | yes | Brightness |
| baseFreq | float | 27.5-440 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10 | 2.0 | yes | Gain |
| curve | float | 0.1-3 | 1.5 | yes | Contrast |
| baseBright | float | 0-1 | 0.15 | yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | (handled by `STANDARD_GENERATOR_OUTPUT`) |

### UI Layout

Colocated `DrawLichenParams(EffectConfig*, const ModSources*, ImU32)` with sections in this order:

1. `ImGui::SeparatorText("Audio")` — Base Freq, Max Freq, Gain, Contrast, Base Bright (per CLAUDE.md FFT conventions, use `"##lichen"` ID suffix on every slider)
2. `ImGui::SeparatorText("Reaction")` — Feed Rate, Kill Base, Coupling, Pred Advantage, Reaction Rate, Reaction Steps (`ImGui::SliderInt`)
3. `ImGui::SeparatorText("Diffusion")` — Activator Radius, Inhibitor Radius
4. `ImGui::SeparatorText("Warp")` — Warp Intensity, Warp Speed
5. `ImGui::SeparatorText("Output")` — Brightness

Use `STANDARD_GENERATOR_OUTPUT(lichen)` for the gradient + Blend Intensity + Blend Mode section. The macro requires `gradient`, `blendIntensity`, `blendMode` field names (already present).

### Constants

- Enum name: `TRANSFORM_LICHEN` (add to `TransformEffectType` in `effect_config.h` before `TRANSFORM_EFFECT_COUNT`)
- Display name: `"Lichen"`
- Category: section index `13` (Field), badge `"GEN"` (auto-applied by `REGISTER_GENERATOR_FULL`)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-applied by `REGISTER_GENERATOR_FULL`)

### Bridge Functions

At the bottom of `lichen.cpp`, immediately above the registration macro:

```cpp
void SetupLichen(PostEffect *pe);          // scratchSetup - binds state shader uniforms
void SetupLichenBlend(PostEffect *pe);     // setup - calls BlendCompositorApply(colorRT)
void RenderLichen(PostEffect *pe);         // render - the multi-pass orchestrator
```

These are non-static (referenced by the macro). The colocated `DrawLichenParams` and `DrawOutput_lichen` (generated by `STANDARD_GENERATOR_OUTPUT(lichen)`) are static.

Registration macro:
```cpp
// clang-format off
STANDARD_GENERATOR_OUTPUT(lichen)
REGISTER_GENERATOR_FULL(TRANSFORM_LICHEN, Lichen, lichen,
                        "Lichen", SetupLichenBlend,
                        SetupLichen, RenderLichen, 13,
                        DrawLichenParams, DrawOutput_lichen)
// clang-format on
```

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create LichenConfig + LichenEffect header

**Files**: `src/effects/lichen.h`
**Creates**: Config struct, Effect runtime struct, public function declarations, `LICHEN_CONFIG_FIELDS` macro

**Do**: Create `src/effects/lichen.h` matching the layout in the Design / Types section above. Include guard `#ifndef LICHEN_EFFECT_H`. Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `typedef struct ColorLUT ColorLUT;`.

Public function declarations (all non-static):
- `bool LichenEffectInit(LichenEffect *e, const LichenConfig *cfg, int width, int height);`
- `void LichenEffectSetup(LichenEffect *e, const LichenConfig *cfg, float deltaTime, const Texture2D &fftTexture);`
- `void LichenEffectRender(LichenEffect *e, int screenWidth, int screenHeight);`
- `void LichenEffectResize(LichenEffect *e, int width, int height);`
- `void LichenEffectReset(LichenEffect *e, int width, int height);`
- `void LichenEffectUninit(LichenEffect *e);`
- `void LichenRegisterParams(LichenConfig *cfg);`

Pattern reference: `src/effects/curl_advection.h` (dual ping-pong + extra colorRT).

**Verify**: `cmake.exe --build build` — file is currently unreferenced so the build is unaffected; just confirm no parse errors by including it in a temporary unit. (Final compile happens after Wave 2.)

---

#### Task 1.2: Create reaction-diffusion state shader

**Files**: `shaders/lichen_state.fs`
**Creates**: The state-update fragment shader

**Do**: Create `shaders/lichen_state.fs` containing exactly the GLSL in the Design / Algorithm / State Shader section above. Begin with the four-line attribution comment block (Based on / URL / License / Modified) before `#version 330`.

Notes:
- `for (int i = 0; i < 8; i++)` is fixed-count and OK; `for (int n = 0; n < reactionSteps; n++)` uses a uniform int — GLSL 330 supports dynamic loop bounds, do not unroll or hardcode a max.
- The 8-octave warp uses pixel-space coordinates with bottom-left origin (matches `fragTexCoord * resolution`); the operations are additive offsets so no centering is needed (this matches the reference Shadertoy convention and the reference's seed positions).
- `q *= -1.681; q = fract(q / 10.0) * 10.0 - 5.0; q = floor(q) + 1.0;` is a deterministic octave sequence — keep verbatim.

**Verify**: Build with `cmake.exe --build build` after Wave 2 lands; confirm no shader-load failure at runtime (raylib will TraceLog if compilation fails).

---

#### Task 1.3: Create color output shader

**Files**: `shaders/lichen.fs`
**Creates**: The color-output fragment shader

**Do**: Create `shaders/lichen.fs` containing exactly the GLSL in the Design / Algorithm / Color Shader section above. No attribution block needed (it's our own shader, not derived).

Notes:
- `texture0` is declared (raylib's quad path auto-binds it) but not read.
- `bandEnergy(t0, t1)` integrates 4 sub-samples across the species' frequency slice — `BAND_SAMPLES = 4` matches the convention in `memory/generator_patterns.md` and `random_volumetric.fs:262-266`.
- The same `t = sliceOffset_i + v_i * SLICE` drives both the gradientLUT lookup and the FFT band — color and audio reactivity stay correlated per the gradient_patterns convention.

**Verify**: Same as Task 1.2.

---

### Wave 2: Wiring

All four tasks below depend on Wave 1's `lichen.h`. The four files are disjoint, so the tasks run in parallel.

#### Task 2.1: Create effect module implementation

**Files**: `src/effects/lichen.cpp`
**Depends on**: Wave 1 complete (`lichen.h`, both shaders)

**Do**: Create `src/effects/lichen.cpp`. Mirror the structure of `src/effects/curl_advection.cpp`:

1. Includes (clang-format will sort within groups):
   - `"lichen.h"`
   - `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `"rlgl.h"`
   - `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
   - `<math.h>`, `<stddef.h>`, `<stdlib.h>`

2. File-static helpers (mirror curl_advection):
   - `static void InitTextures(LichenEffect *e, int w, int h)` — calls `RenderUtilsInitTextureHDR` 5 times for the 4 state RTs + colorRT
   - `static void UnloadTextures(const LichenEffect *e)` — `UnloadRenderTexture` x5
   - `static void InitializeSeed(LichenEffect *e, int w, int h)` — implements the seed pseudocode in Design / Algorithm / CPU Seed Function (uses `rand()`/`RAND_MAX` for `dateOffset`; add `// NOLINTNEXTLINE(concurrency-mt-unsafe)` per curl_advection)
   - `static void CacheStateLocations(LichenEffect *e)` — caches all `stateXxxLoc` fields
   - `static void CacheColorLocations(LichenEffect *e)` — caches all `colorXxxLoc` fields
   - `static void BindStateUniforms(const LichenEffect *e, const LichenConfig *cfg, const float *resolution)` — sets all state-shader scalar uniforms (resolution, time already set in Setup, feedRate, killRateBase, couplingStrength, predatorAdvantage, warpIntensity, warpSpeed, activatorRadius, inhibitorRadius, reactionSteps, reactionRate)
   - `static void BindColorUniforms(const LichenEffect *e, const LichenConfig *cfg, const float *resolution)` — sets resolution, brightness, sampleRate, and FFT scalars (baseFreq, maxFreq, gain, curve, baseBright)

3. Public functions:
   - `LichenEffectInit`: load `shaders/lichen_state.fs` (check id), load `shaders/lichen.fs` (check, unload state on fail), `CacheStateLocations`, `CacheColorLocations`, `e->gradientLUT = ColorLUTInit(&cfg->gradient)` (cleanup both shaders on fail), `InitTextures(e, w, h)`, `InitializeSeed(e, w, h)`, `RenderUtilsClearTexture(&e->colorRT)`, `e->readIdx0 = 0; e->readIdx1 = 0; e->time = 0.0f;`, return true. Cascade unload pattern from `curl_advection.cpp:100-134`.
   - `LichenEffectSetup`: accumulate `e->time += deltaTime` with `WRAP = 1000.0f`. `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`. Compute `resolution[2]`. `BindStateUniforms` and `SetShaderValue(stateShader, stateTimeLoc, &e->time, FLOAT)`. `BindColorUniforms`. Bind FFT texture: `SetShaderValueTexture(e->shader, e->colorFftTextureLoc, fftTexture)` and `sampleRate = (float)AUDIO_SAMPLE_RATE`. Bind gradientLUT: `SetShaderValueTexture(e->shader, e->colorGradientLUTLoc, ColorLUTGetTexture(e->gradientLUT))`.
   - `LichenEffectRender`: implements the 4-step render flow from Design / Render Flow Per Frame. Both simulation passes share the same `stateShader`; pass 0's `BeginTextureMode(statePingPong0[writeIdx0])` ends before pass 1 begins. Use `rlDisableColorBlend()` / `rlEnableColorBlend()` around each simulation pass. Bind `passIndex` between passes via `SetShaderValue(stateShader, statePassIndexLoc, &passIdx, SHADER_UNIFORM_INT)`. The color pass uses `e->shader` (already has FFT/LUT/scalars bound from Setup); only the per-frame state texture pointers need binding here.
   - `LichenEffectResize`: `UnloadTextures(e)`, `InitTextures(e, w, h)`, `InitializeSeed(e, w, h)`, `RenderUtilsClearTexture(&e->colorRT)`, `e->readIdx0 = 0; e->readIdx1 = 0;`.
   - `LichenEffectReset`: `RenderUtilsClearTexture(&e->colorRT)`, `InitializeSeed(e, w, h)`, `e->readIdx0 = 0; e->readIdx1 = 0;` (do not unload textures).
   - `LichenEffectUninit`: `UnloadShader(e->stateShader)`, `UnloadShader(e->shader)`, `ColorLUTUninit(e->gradientLUT)`, `UnloadTextures(e)`.
   - `LichenRegisterParams`: register every modulatable param with the bounds in the Parameters table. Param IDs use prefix `"lichen."`.

4. Bridge functions (non-static, referenced by macro):
   - `void SetupLichen(PostEffect *pe)`: calls `LichenEffectSetup(&pe->lichen, &pe->effects.lichen, pe->currentDeltaTime, pe->fftTexture)`.
   - `void SetupLichenBlend(PostEffect *pe)`: calls `BlendCompositorApply(pe->blendCompositor, pe->lichen.colorRT.texture, pe->effects.lichen.blendIntensity, pe->effects.lichen.blendMode)`.
   - `void RenderLichen(PostEffect *pe)`: calls `LichenEffectRender(&pe->lichen, pe->screenWidth, pe->screenHeight)`.

5. UI section (`// === UI ===`) below bridges:
   - `static void DrawLichenParams(EffectConfig *e, const ModSources *modSources, ImU32 categoryGlow)` with the section ordering specified in Design / UI Layout. All slider IDs use `"##lichen"` suffix. `reactionSteps` is `int` and uses `ImGui::SliderInt`.
   - `STANDARD_GENERATOR_OUTPUT(lichen)` macro generates `DrawOutput_lichen` automatically.

6. Final clang-format off/on block with the registration macro.

Pattern references:
- Multi-pass render orchestration: `curl_advection.cpp:205-241`
- FFT uniform plumbing + Setup signature with `const Texture2D &fftTexture`: `random_volumetric.cpp:60-104`, `constellation.cpp:80-114`
- Bridge functions and macro: `curl_advection.cpp:299-381`

**Verify**: `cmake.exe --build build` succeeds with the file in place (after Wave 2 completes); effect appears in the Field section of the UI; enabling it does not crash.

---

#### Task 2.2: Wire `LichenConfig` into `EffectConfig`

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1's `lichen.h`

**Do**: Three edits to `src/config/effect_config.h`:

1. Add `#include "effects/lichen.h"` in the alphabetical includes block (between `lego_bricks.h` and `lens_space.h` or wherever clang-format places it).
2. Add `TRANSFORM_LICHEN,` to the `TransformEffectType` enum, immediately before `TRANSFORM_ACCUM_COMPOSITE,`.
3. Add `LichenConfig lichen;` as a member of `EffectConfig` (placement: anywhere; pick a location near other generators like after `RandomVolumetricConfig randomVolumetric;`). Include a one-line comment matching the convention.

Do NOT touch `TransformOrderConfig::TransformOrderConfig()` — the default ctor iterates `[0, TRANSFORM_EFFECT_COUNT)` so the new enum is picked up automatically.

**Verify**: Compile checks at end of Wave 2.

---

#### Task 2.3: Wire `LichenEffect` into `PostEffect`

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1's `lichen.h`

**Do**: Two edits to `src/render/post_effect.h`:

1. Add `#include "effects/lichen.h"` in the alphabetical includes block.
2. Add `LichenEffect lichen;` as a member of the `PostEffect` struct, placed near `CurlAdvectionEffect curlAdvection;` (line 224) so the dual-state-texture generators are co-located.

No changes to `post_effect.cpp` are needed — the descriptor loop handles init/uninit/resize/registerParams via the function pointers in `EFFECT_DESCRIPTORS[TRANSFORM_LICHEN]`.

**Verify**: Compile checks at end of Wave 2.

---

#### Task 2.4: Wire `LichenConfig` into preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1's `lichen.h`

**Do**: Three edits to `src/config/effect_serialization.cpp`:

1. Add `#include "effects/lichen.h"` in the alphabetical includes block at the top.
2. Add the JSON macro alongside the others (alphabetically between `LegoBricksConfig` and `LightMedleyConfig`):
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LichenConfig,
                                                   LICHEN_CONFIG_FIELDS)
   ```
3. Add `X(lichen)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (anywhere; the order is whitespace-formatted, place near other generators).

Pattern: see `RandomVolumetricConfig` at `effect_serialization.cpp:565-566` and `X(randomVolumetric)` at line 771.

**Verify**: Compile checks at end of Wave 2; round-trip a preset save/load and confirm Lichen settings persist.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings.
- [ ] `Lichen` appears in the Field category in the effects UI.
- [ ] Enabling Lichen produces three colored blobs that grow into each other within ~1 second on a fresh state.
- [ ] FFT-reactive: brightness response visible when audio is playing; bass band lifts species 0, mid lifts species 1, treble lifts species 2 (depending on gradient ordering).
- [ ] Modulating `feedRate`, `couplingStrength`, `warpIntensity`, `predatorAdvantage` from the modulation engine drives visible behavior changes.
- [ ] Window resize reseeds without crashing; patterns regenerate at new resolution.
- [ ] Preset save/load preserves all Lichen config fields including the gradient.
- [ ] No global FFT brightness; the audio response is per-species.
- [ ] No tonemap, no Reinhard; only the final `clamp(col, 0.0, 1.0)`.
- [ ] No `// Original:`, `// Reference:`, or other forbidden comments in the shader files (only the four-line attribution block at the top of `lichen_state.fs`).

---

## Implementation Notes

Deviations from the plan made during integration:

### State texture sampler state
- Wrap set to `TEXTURE_WRAP_REPEAT` after `RenderUtilsInitTextureHDR` (default is `CLAMP`). The reference relies on toroidal sampling for the warp offsets and diffusion taps near the edges.
- Filter left at `BILINEAR` (the default). `POINT` filtering produced visible grid artifacts because diffusion samples at fractional pixel offsets (e.g. `p + vec2(2.5, 0)`).

### Cyclic coupling samples taken pre-warp
The state shader samples the predator/prey activator (`hp0`, `hp1`) BEFORE the 8-octave warp loop, matching the reference's `vec4 hp = texture(iChannel1, p / r)` ordering. Sampling them after the warp spatially scrambled the coupling and let regions runaway-decay.

### `reactionSteps` default reduced from 25 to 10
The reference's 25 iterations × dt 0.4 sits at the edge of forward-Euler stability. At non-Shadertoy window sizes the system overshot into death (blobs expanded then `v` decayed to 0). 10 iterations is stable across resolutions; users can crank up via the slider.

### FFT texture cached on the effect struct
`SetShaderValueTexture` only tracks slots inside an active `BeginShaderMode` block. The plan called for binding the FFT texture in `Setup` (outside any shader-mode block), which silently failed. Setup now stores `fftTexture` on `LichenEffect`, and the color pass binds it inside `BeginShaderMode` alongside the state textures and gradient LUT.

### Color shader rewrite for per-clump variation and FFT correlation
The plan's per-pixel `t = sliceOffset + v * SLICE` produced a single visible color per species because `v` is essentially binary in stable Gray-Scott (0 outside, ~1 inside).

Final form: per-pixel `t = sliceOffset + valueNoise(uv * colorScatter) * SLICE`. A 2D value-noise hash is sampled at the pixel UV, giving each clump its own static gradient position within the species' 1/3 slice. The same `t` drives both the gradient lookup and the per-pixel FFT bin (`fftAt(t)`), keeping color and audio reactivity locked.

### New `colorScatter` parameter
Added to `LichenConfig` (range 1–120, default 40, modulatable). Controls the spatial-noise scale: low values give broad color zones spanning many clumps, high values give every clump its own hue.
