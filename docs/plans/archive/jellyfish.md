# Jellyfish

A drifting swarm of glowing jellyfish suspended in a deep blue water column. Bell bodies pulse, ringed by halos and trailing fans of tapered tentacles waving in seeded sine drift. Marine snow particles rise upward through the column, and a Worley caustic shimmer plays across mid-depths. Each jellyfish samples its hue from the active gradient at a hashed seed, and gets an additive brightness boost from a per-seed log-spaced FFT band so the swarm reads as a moving spectral display.

**Research**: `docs/research/jellyfish.md`

**Reference**: "Bioluminescent Deep Ocean" by hagy (https://www.shadertoy.com/view/7c2Xz3, CC BY-NC-SA 3.0).

## Design

### Types

`src/effects/jellyfish.h`:

```cpp
struct JellyfishConfig {
  bool enabled = false;

  // FFT mapping (per-jellyfish log-spaced band)
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Swarm
  int jellyCount = 8;          // 1-16

  // Bell shape and pulse
  float sizeBase = 0.052f;     // 0.02-0.10
  float sizeVariance = 0.026f; // 0.0-0.06
  float pulseDepth = 0.14f;    // 0.0-0.4
  float pulseSpeed = 1.0f;     // 0.1-3.0

  // Drift path
  float driftAmp = 1.0f;       // 0.0-3.0 (scales seeded jellyPath amplitudes)
  float driftSpeed = 1.0f;     // 0.0-3.0 (scales seeded jellyPath frequencies)

  // Tentacles
  float tentacleWave = 0.007f; // 0.0-0.025

  // Glow stack
  float bellGlow = 0.40f;      // 0.0-1.0 (replaces bellHalo constant)
  float rimGlow = 0.45f;       // 0.0-1.0
  float tentacleGlow = 0.55f;  // 0.0-1.0

  // Marine snow
  float snowDensity = 0.5f;    // 0.0-1.0 (larger = denser)
  float snowBrightness = 0.14f;// 0.0-0.4

  // Caustics
  float causticIntensity = 0.26f; // 0.0-0.6

  // Backdrop
  float backdropDepth = 1.0f;  // 0.0-2.0 (depth gradient strength multiplier)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define JELLYFISH_CONFIG_FIELDS                                                \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, jellyCount, sizeBase,   \
      sizeVariance, pulseDepth, pulseSpeed, driftAmp, driftSpeed, tentacleWave,\
      bellGlow, rimGlow, tentacleGlow, snowDensity, snowBrightness,            \
      causticIntensity, backdropDepth, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct JellyfishEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;

  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;

  int jellyCountLoc;
  int sizeBaseLoc;
  int sizeVarianceLoc;
  int pulseDepthLoc;
  int pulseSpeedLoc;
  int driftAmpLoc;
  int driftSpeedLoc;
  int tentacleWaveLoc;
  int bellGlowLoc;
  int rimGlowLoc;
  int tentacleGlowLoc;
  int snowDensityLoc;
  int snowBrightnessLoc;
  int causticIntensityLoc;
  int backdropDepthLoc;
} JellyfishEffect;

bool JellyfishEffectInit(JellyfishEffect *e, const JellyfishConfig *cfg);
void JellyfishEffectSetup(JellyfishEffect *e, const JellyfishConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture);
void JellyfishEffectUninit(JellyfishEffect *e);
void JellyfishRegisterParams(JellyfishConfig *cfg);
JellyfishEffect *GetJellyfishEffect(struct PostEffect *pe);
```

### Algorithm

The reference is a complete procedural scene. Adaptation is mechanical substitution: keep the helpers and SDF stack verbatim; replace the five hardcoded `jellyfish(...)` calls with a `for (s = 0; s < jellyCount; s++)` loop where each iteration samples the gradient LUT for hue and a log-spaced FFT band for additive brightness boost; delete the water-surface band and the Reinhard tonemap.

Final shader content (`shaders/jellyfish.fs`):

```glsl
// Based on "Bioluminescent Deep Ocean" by hagy
// https://www.shadertoy.com/view/7c2Xz3
// License: CC BY-NC-SA 3.0 Unported
// Modified: surface band removed, Reinhard tonemap removed,
// per-jellyfish FFT log-spaced band brightness, gradient-LUT seeded hues,
// dynamic jellyCount loop, hardcoded brightness constants exposed as uniforms.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;

uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

uniform int jellyCount;
uniform float sizeBase;
uniform float sizeVariance;
uniform float pulseDepth;
uniform float pulseSpeed;
uniform float driftAmp;
uniform float driftSpeed;
uniform float tentacleWave;
uniform float bellGlow;
uniform float rimGlow;
uniform float tentacleGlow;
uniform float snowDensity;
uniform float snowBrightness;
uniform float causticIntensity;
uniform float backdropDepth;

float hash21(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

vec2 hash22(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

mat2 rot2D(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash21(i),                hash21(i + vec2(1.0, 0.0)), u.x),
        mix(hash21(i + vec2(0.0,1.0)),hash21(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2 r = rot2D(0.5);
    for (int i = 0; i < 5; i++) {
        v += a * valueNoise(p);
        p = r * p * 2.0;
        a *= 0.5;
    }
    return v;
}

vec2 worley(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    float d1 = 1e10, d2 = 1e10;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 nb = vec2(float(x), float(y));
            vec2 df = nb + hash22(i + nb) - f;
            float d = dot(df, df);
            if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
        }
    }
    return sqrt(vec2(d1, d2));
}

float caustics(vec2 p, float t) {
    vec2 w1 = worley(p * 3.2 + vec2( 0.7, 0.3) + t * 0.15);
    vec2 w2 = worley(p * 2.1 + vec2( 1.4, 0.9) - t * 0.09);
    return pow(1.0 - w1.x, 4.0) * 0.55 + pow(1.0 - w2.x, 3.0) * 0.45;
}

float sdEllipse(vec2 p, vec2 ab) {
    float k = length(p / ab);
    return (k - 1.0) * min(ab.x, ab.y);
}

float sdSeg(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));
}

float smax(float a, float b, float k) {
    float h = max(k - abs(a - b), 0.0) / k;
    return max(a, b) + h * h * k * 0.25;
}

vec2 jellyPath(float s, float t) {
    float ax = (0.09 + hash21(vec2(s, 1.5)) * 0.07) * driftAmp;
    float ay = (0.06 + hash21(vec2(s, 1.6)) * 0.05) * driftAmp;
    float fx = (0.22 + hash21(vec2(s, 1.1)) * 0.16) * driftSpeed;
    float fy = (0.17 + hash21(vec2(s, 1.2)) * 0.12) * driftSpeed;
    float px = hash21(vec2(s, 1.3)) * 6.283;
    float py = hash21(vec2(s, 1.4)) * 6.283;
    return vec2(ax * sin(t * fx + px), ay * sin(t * fy + py));
}

vec3 jellyfish(vec2 uv, vec2 ctr, float s, vec3 hue, float phase, float t) {
    float sz   = sizeBase + hash21(vec2(s, 9.2)) * sizeVariance;
    float pSpd = (0.55 + hash21(vec2(s, 9.1)) * 0.35) * pulseSpeed;

    float pulse = 1.0 + pulseDepth * sin(t * pSpd * 3.14159 + phase);
    float pz    = sz * pulse;
    vec2  pos   = ctr + jellyPath(s, t);
    vec2  lp    = uv - pos;

    float bellBody = sdEllipse(lp, vec2(pz * 0.76, pz * 0.38));
    float bellD    = smax(bellBody, lp.y + pz * 0.07, pz * 0.10);

    float cavD = sdEllipse(lp - vec2(0.0, pz * 0.10), vec2(pz * 0.46, pz * 0.29));

    float tentD = 100.0;
    float waveScale = tentacleWave / 0.007;
    for (int i = 0; i < 8; i++) {
        float fi   = float(i);
        float ang  = (fi / 7.0 - 0.5) * 1.5;
        float tLen = (0.05 + hash21(vec2(s, fi + 20.0)) * 0.04) * 1.5;
        vec2  prev = pos + vec2(sin(ang) * pz * 0.65, -pz * 0.06);
        for (int j = 0; j < 4; j++) {
            float fj  = float(j);
            float wX  = (sin(t * 1.1 + fi * 1.4 + fj * 0.9 + s * 4.7) * 0.007
                       + sin(t * 2.3 + fi * 0.8 + fj * 1.5 + s * 2.1) * 0.004) * waveScale;
            vec2  nxt = prev + vec2(wX, -tLen * 0.25);
            float taper = mix(0.0045, 0.0012, fj / 3.0);
            tentD = min(tentD, sdSeg(uv, prev, nxt) - taper);
            prev  = nxt;
        }
    }

    float rimD       = abs(sdEllipse(lp + vec2(0.0, pz * 0.07), vec2(pz * 0.68, pz * 0.265)));
    float rimGlowVal = exp(-rimD * rimD * 5500.0) * rimGlow;

    float cavMask  = smoothstep(-pz * 0.02, pz * 0.04, cavD);
    float cavRim   = exp(-cavD  * cavD  * 3500.0) * 0.12;
    float interior = smoothstep(0.0, -pz * 0.42, bellD) * (0.04 + 0.10 * cavMask);

    float bellRim     = exp(-bellD * bellD * 300.0);
    float bellHaloVal = exp(-max(bellD, 0.0) * 14.0) * bellGlow;
    float tentGlowVal = exp(-max(tentD, 0.0) * 60.0) * tentacleGlow;

    float nVar = valueNoise(uv * 11.0 + vec2(s * 2.3)) * 0.3 + 0.7;

    vec3 glow  = hue * (bellRim + bellHaloVal + interior + cavRim + tentGlowVal) * nVar;
    glow      += mix(hue, vec3(1.0), 0.7) * rimGlowVal;
    return glow;
}

vec3 marineSnow(vec2 uv, float t) {
    vec3  col = vec3(0.0);
    float cs  = mix(0.18, 0.04, snowDensity);
    vec2  cb  = floor(uv / cs);

    for (int cy = -1; cy <= 1; cy++) {
        for (int cx = -1; cx <= 1; cx++) {
            vec2  ci  = cb + vec2(float(cx), float(cy));
            vec2  rng = hash22(ci);
            float spd = 0.018 + hash21(ci + vec2(3.17, 1.43)) * 0.012;

            vec2  pt = vec2(
                (ci.x + rng.x) * cs,
                (ci.y + fract(rng.y + t * spd / cs)) * cs
            );

            float twk = 0.45 + 0.55 * sin(t * (0.35 + rng.x * 0.7) + rng.y * 6.283);
            float d   = length(uv - pt);
            float brt = 0.55 + 0.45 * smoothstep(-0.45, 0.40, pt.y);

            col += vec3(0.62, 0.82, 1.0) * exp(-d * d * 55000.0) * twk * snowBrightness * brt;
        }
    }
    return col;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;
    float t = time;

    float depth = clamp(uv.y + 0.5, 0.0, 1.0);

    vec3  abyssCol = vec3(0.005, 0.010, 0.040);
    vec3  deepCol  = vec3(0.012, 0.035, 0.130);
    vec3  surfCol  = vec3(0.030, 0.080, 0.250);
    float dAbove   = smoothstep(0.0, 0.30, depth);
    vec3  col      = mix(abyssCol, mix(deepCol, surfCol, pow(depth, 1.3)), dAbove);
    col += vec3(0.0, 0.007, 0.022) * fbm(uv * 2.8 + vec2(t * 0.07, t * 0.04));
    col *= backdropDepth;

    col += marineSnow(uv, t);

    for (int s = 0; s < jellyCount; s++) {
        float fs = float(s) + 1.0;
        vec2  ctr   = (hash22(vec2(fs, 7.7)) - 0.5) * vec2(0.8, 0.6);
        float phase = hash21(vec2(fs, 8.3)) * 6.283;
        float lutT  = hash21(vec2(fs, 5.5));
        vec3  hue   = texture(gradientLUT, vec2(lutT, 0.5)).rgb;

        float t0 = float(s) / float(jellyCount);
        float t1 = float(s + 1) / float(jellyCount);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int b = 0; b < BAND_SAMPLES; b++) {
            float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        float fftBoost = baseBright + mag;

        col += jellyfish(uv, ctr, fs, hue, phase, t) * fftBoost;
    }

    float causticMask = smoothstep(0.12, 0.85, depth);
    float caus = caustics(uv * 2.3 + vec2(t * 0.07), t * 0.22);
    col += vec3(0.6, 0.8, 1.0) * caus * causticMask * causticIntensity;

    vec2  vUV = gl_FragCoord.xy / resolution;
    vec2  vq  = vUV * (1.0 - vUV);
    col *= mix(0.22, 1.0, pow(vq.x * vq.y * 15.0, 0.32));

    finalColor = vec4(col, 1.0);
}
```

Substitutions applied vs reference:
- `iTime`/`iResolution` -> `time`/`resolution` uniforms; fragment input is `gl_FragCoord.xy`.
- Five hardcoded `jellyfish(...)` scene calls replaced with `for (s = 0; s < jellyCount; ++s)` loop. Per iteration: `ctr` from `(hash22(vec2(fs, 7.7)) - 0.5) * vec2(0.8, 0.6)`, `phase` from `hash21(vec2(fs, 8.3)) * 6.283`, `hue` from `texture(gradientLUT, vec2(hash21(vec2(fs, 5.5)), 0.5)).rgb`. `fs = float(s) + 1.0` reproduces reference seed offsets when `jellyCount = 5`.
- Per-jellyfish log-spaced FFT band: divide `[baseFreq, maxFreq]` into `jellyCount` bands, take band index `s`, average 4 bins (`BAND_SAMPLES`), apply `gain` then `curve`, add to `baseBright`. `fftBoost = baseBright + mag` multiplies the jellyfish's returned `glow`.
- `surfY`, `surfaceH`, `gerstnerH`, `belowSurf`, `surfBand`, `aboveMask`, `canopyCaus` and the `* belowSurf` masks deleted entirely. Snow/jellyfish/caustics render across the full frame with the depth gradient running from `abyssCol` (bottom) to `surfCol` (top).
- Reinhard `1.0 - exp(-col * 1.3)` line deleted (project rule: no tonemap in shaders).
- `0.052` size base -> `sizeBase`; `0.026` variance -> `sizeVariance`; `0.14` pulse depth -> `pulseDepth`.
- `0.55 + hash21(...) * 0.35` pulse-speed seeded random multiplied by `pulseSpeed` scalar.
- `jellyPath` amplitude constants (`0.09, 0.07, 0.06, 0.05`) scaled by `driftAmp`; frequency constants (`0.22, 0.16, 0.17, 0.12`) scaled by `driftSpeed`. Phases unchanged.
- Tentacle `wX` expression scaled by `tentacleWave / 0.007` to preserve the 7:4 ratio between the two harmonics; default `tentacleWave = 0.007` reproduces reference.
- `0.40` bellHalo -> `bellGlow`; `0.45` rimGlow -> `rimGlow` (uniform name same); `0.55` tentGlow -> `tentacleGlow`.
- `0.09` cell size in `marineSnow` -> `mix(0.18, 0.04, snowDensity)` (larger UI value = smaller cell = denser).
- `0.14` marineSnow brightness -> `snowBrightness`.
- `0.26` caustic scalar -> `causticIntensity`.
- New `backdropDepth` multiplier on the assembled backdrop col (depth gradient + fbm tint).
- **Implementation note (post-plan)**: `pulseSpeed` and `driftSpeed` are CPU-accumulated into `pulsePhase` and `driftPhase` floats on `JellyfishEffect`, bound as uniforms instead of the speed scalars. The shader uses `sin(pulsePhase * pSpd * 3.14159 + phase)` where `pSpd = 0.55 + hash21*0.35` (seeded variation only), and `jellyPath(s, driftPhase)` with `fx = 0.22 + hash21*0.16` (no driftSpeed factor). Speed in shader is forbidden — see `memory/feedback_speed_accumulation.md`.
- Hardcoded scalars kept verbatim (no parameter): `bellRim` `300.0`, `cavRim` `0.12`, `interior` `0.04 + 0.10 * cavMask`, `nVar` `0.3 + 0.7`, `rimGlowVal` exp shaper `5500.0`, vignette `mix(0.22, 1.0, pow(vq.x * vq.y * 15.0, 0.32))`, fbm tint `vec3(0.0, 0.007, 0.022)`, snow color `vec3(0.62, 0.82, 1.0)`, caustic tint `vec3(0.6, 0.8, 1.0)`.
- The five jellyfish-call hue constants are replaced by gradient-LUT samples; the `mix(hue, vec3(1.0), 0.7)` rim-color shift is preserved so silhouettes stay legible.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| jellyCount | int | 1-16 | 8 | no | Jelly Count |
| sizeBase | float | 0.02-0.10 | 0.052 | yes | Size Base |
| sizeVariance | float | 0.0-0.06 | 0.026 | yes | Size Variance |
| pulseDepth | float | 0.0-0.4 | 0.14 | yes | Pulse Depth |
| pulseSpeed | float | 0.1-3.0 | 1.0 | yes | Pulse Speed |
| driftAmp | float | 0.0-3.0 | 1.0 | yes | Drift Amp |<!-- Intentional deviation: research's parameter-table row for driftAmp (0.0-0.30 / 0.10) contradicts its substitution-table wording ("scales seeded amp"). User confirmed multiplier semantics matching driftSpeed — research param row treated as transcription error. -->


| driftSpeed | float | 0.0-3.0 | 1.0 | yes | Drift Speed |
| tentacleWave | float | 0.0-0.025 | 0.007 | yes | Tentacle Wave |
| bellGlow | float | 0.0-1.0 | 0.40 | yes | Bell Glow |
| rimGlow | float | 0.0-1.0 | 0.45 | yes | Rim Glow |
| tentacleGlow | float | 0.0-1.0 | 0.55 | yes | Tentacle Glow |
| snowDensity | float | 0.0-1.0 | 0.5 | yes | Snow Density |
| snowBrightness | float | 0.0-0.4 | 0.14 | yes | Snow Bright |
| causticIntensity | float | 0.0-0.6 | 0.26 | yes | Caustics |
| backdropDepth | float | 0.0-2.0 | 1.0 | yes | Backdrop |<!-- Intentional deviation: research describes backdropDepth as "Vertical depth gradient strength multiplier (abyss-to-surf mix)" but specifies no shader implementation. Plan implements as `col *= backdropDepth` after the depth gradient and fbm tint are added. -->


| baseFreq | float | 27.5-440 | 55 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000 | yes | Max Freq (Hz) |
| gain | float | 0.1-10 | 2.0 | yes | Gain |
| curve | float | 0.1-3 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | (output) |

### Constants

- Enum: `TRANSFORM_JELLYFISH_BLEND` (added before `TRANSFORM_ACCUM_COMPOSITE`)
- Display name: `"Jellyfish"`
- Category badge: `"GEN"` (set by `REGISTER_GENERATOR`)
- Section index: `15` (Scatter)
- Effect field name on `EffectConfig` and modulation prefix: `jellyfish`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create jellyfish.h

**Files**: `src/effects/jellyfish.h` (new)
**Creates**: `JellyfishConfig`, `JellyfishEffect`, `JELLYFISH_CONFIG_FIELDS`, lifecycle declarations.

**Do**:
- Use `src/effects/nebula.h` as the structural model (FFT generator with `ColorConfig` gradient and blend-compositor fields).
- Define `JellyfishConfig` exactly as in the Design > Types section above. Include inline range comments on each field.
- Define `JELLYFISH_CONFIG_FIELDS` macro listing every field in declaration order (omit nothing including `enabled`, `gradient`, `blendMode`, `blendIntensity`).
- Define `JellyfishEffect` runtime struct with `Shader shader`, `ColorLUT *gradientLUT`, `float time`, and one int `*Loc` per uniform.
- Declare `JellyfishEffectInit`, `JellyfishEffectSetup`, `JellyfishEffectUninit`, `JellyfishRegisterParams`, `GetJellyfishEffect`.
- Includes: `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `struct PostEffect;` and `typedef struct ColorLUT ColorLUT;`.
- Add include guards `#ifndef JELLYFISH_H ... #endif`.

**Verify**: `cmake.exe --build build` still compiles (header only, not yet referenced).

---

### Wave 2: Parallel — depend on Wave 1 complete

#### Task 2.1: Wire jellyfish into effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete (jellyfish.h exists).

**Do**:
- Add `#include "effects/jellyfish.h"` in the existing alphabetical include block (between `effects/isoflow.h` and `effects/kaleidoscope.h`).
- Add enum entry `TRANSFORM_JELLYFISH_BLEND,` to `TransformEffectType` immediately before `TRANSFORM_ACCUM_COMPOSITE`.
- Add config member `JellyfishConfig jellyfish;` to `EffectConfig` near the other generator entries (group with the recent additions like `apollonianTunnel`, `lichen`).
- The `TransformOrderConfig::TransformOrderConfig()` constructor populates `order[i] = (TransformEffectType)i` for all values, so no manual order entry is needed.

**Verify**: `cmake.exe --build build` compiles. Effect link will fail in this wave because jellyfish.cpp does not yet exist; that is expected and resolved by 2.4. (If individual file compiles, that's enough for this task.)

#### Task 2.2: Create jellyfish fragment shader

**Files**: `shaders/jellyfish.fs` (new)
**Depends on**: Wave 1 complete (no actual code dep, but ordering keeps wave structure clean).

**Do**:
- Create `shaders/jellyfish.fs` containing the exact shader code in the Design > Algorithm section above.
- Begin the file with the attribution comment block before `#version 330`.
- Use `for (int s = 0; s < jellyCount; s++)` and `for (int b = 0; b < BAND_SAMPLES; b++)` directly (GLSL 330 supports dynamic loop bounds).
- Do not add any tonemap, exposure, or gamma on the final color.
- Coordinates use centered convention: `vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;` so that hashed centers in `[-0.4, 0.4] x [-0.3, 0.3]` land in the visible area.

**Verify**: File compiles when the runtime loads it (validated in 2.4 verify step). Manual check: no `iTime`, `iResolution`, `belowSurf`, `surfaceH`, `gerstnerH`, `aboveMask`, or Reinhard tonemap line remain.

#### Task 2.3: Add JSON serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete (needs `JELLYFISH_CONFIG_FIELDS` macro from header).

**Do**:
- Add `#include "effects/jellyfish.h"` in the alphabetical include block (between `effects/isoflow.h` and `effects/kaleidoscope.h`).
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(JellyfishConfig, JELLYFISH_CONFIG_FIELDS)` near the other per-config macros (place beside other generator configs).
- Add `X(jellyfish)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table on the same row as other recent generator fields.

**Verify**: `cmake.exe --build build` compiles. Save and reload a preset that touches jellyfish: the `jellyfish` block round-trips.

---

### Wave 3: Implementation

#### Task 3.1: Create jellyfish.cpp

**Files**: `src/effects/jellyfish.cpp` (new)
**Depends on**: Wave 2 complete (`EffectConfig::jellyfish` member must exist for the registration macro and UI callback).

**Do**:
- Use `src/effects/nebula.cpp` as the structural model.
- Includes (alphabetized within groups; clang-format will sort within groups):
  - Own header: `"jellyfish.h"`
  - Audio: `"audio/audio.h"` (for `AUDIO_SAMPLE_RATE`)
  - Project: `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`
  - UI: `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`
  - System: `<stddef.h>`
- `JellyfishEffectInit`: load `shaders/jellyfish.fs`, on `e->shader.id == 0` return false. Cache every `*Loc` via `GetShaderLocation`. Allocate `e->gradientLUT = ColorLUTInit(&cfg->gradient);` and unload the shader if that returns NULL. Set `e->time = 0.0f`. Return true.
- `JellyfishEffectSetup`:
  - Accumulate `e->time += deltaTime;` (no pulseSpeed multiplier on time itself; pulseSpeed scales the per-jellyfish `pSpd` factor inside the shader).
  - `ColorLUTUpdate(e->gradientLUT, &cfg->gradient);`
  - Bind `resolution` from `GetScreenWidth/Height`, `time`, `sampleRate = (float)AUDIO_SAMPLE_RATE`, `fftTexture` via `SetShaderValueTexture`, `gradientLUT` via `SetShaderValueTexture` using `ColorLUTGetTexture(e->gradientLUT)`.
  - Bind every config field to its uniform location with `SHADER_UNIFORM_FLOAT` (jellyCount uses `SHADER_UNIFORM_INT`).
- `JellyfishEffectUninit`: `UnloadShader(e->shader); ColorLUTUninit(e->gradientLUT);`.
- `JellyfishRegisterParams`: register every modulatable parameter from the parameter table with the bounds shown there. Bounds must match the inline range comments. Always register `blendIntensity` with range `0.0-5.0`.
- `GetJellyfishEffect`: returns `(JellyfishEffect *)pe->effectStates[TRANSFORM_JELLYFISH_BLEND]`.
- `SetupJellyfish(PostEffect *pe)` bridge: calls `JellyfishEffectSetup(GetJellyfishEffect(pe), &pe->effects.jellyfish, pe->currentDeltaTime, pe->fftTexture)`. Non-static.
- `SetupJellyfishBlend(PostEffect *pe)` bridge: calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.jellyfish.blendIntensity, pe->effects.jellyfish.blendMode);`. Non-static.
- `// === UI ===` section with static `DrawJellyfishParams(EffectConfig *e, const ModSources *modSources, ImU32 categoryGlow)`:
  - `(void)categoryGlow;`
  - Sections in order: Audio, Swarm, Bell, Drift, Tentacles, Glow, Atmosphere.
  - **Audio** (`SeparatorText("Audio")`): Base Freq (Hz) `%.1f`, Max Freq (Hz) `%.0f`, Gain `%.1f`, Contrast `%.2f`, Base Bright `%.2f`. Use `ModulatableSlider` for all.
  - **Swarm** (`SeparatorText("Swarm")`): `ImGui::SliderInt("Jelly Count##jellyfish", &n->jellyCount, 1, 16);`.
  - **Bell** (`SeparatorText("Bell")`): Size Base `%.3f`, Size Variance `%.3f`, Pulse Depth `%.2f`, Pulse Speed `%.2f`.
  - **Drift** (`SeparatorText("Drift")`): Drift Amp `%.2f`, Drift Speed `%.2f`.
  - **Tentacles** (`SeparatorText("Tentacles")`): Tentacle Wave `%.4f`.
  - **Glow** (`SeparatorText("Glow")`): Bell Glow `%.2f`, Rim Glow `%.2f`, Tentacle Glow `%.2f`.
  - **Atmosphere** (`SeparatorText("Atmosphere")`): Snow Density `%.2f`, Snow Bright `%.2f`, Caustics `%.2f`, Backdrop `%.2f`.
  - All `ModulatableSlider` calls take label `"<text>##jellyfish"` and param id `"jellyfish.<field>"`.
- Bottom of file (`// clang-format off` / `// clang-format on`):
  ```cpp
  STANDARD_GENERATOR_OUTPUT(jellyfish)
  REGISTER_GENERATOR(TRANSFORM_JELLYFISH_BLEND, Jellyfish, jellyfish, "Jellyfish",
                     SetupJellyfishBlend, SetupJellyfish, 15, DrawJellyfishParams, DrawOutput_jellyfish)
  ```

**Verify**:
1. `cmake.exe --build build` compiles the full executable with no warnings.
2. Launch `./build/AudioJones.exe`. Open the Field section in the effects panel.
3. Enable **Jellyfish**. Confirm: pipeline lists "Jellyfish" with the GEN badge; sliders modify the visual in real time; the gradient widget changes hues across the swarm; modulating `pulseDepth` shows breathing bells; modulating `causticIntensity` brightens the shimmer.
4. Save preset, restart, load preset: jellyfish settings preserved.
5. Play audio: per-jellyfish flicker varies across the swarm with frequency content (low jellies pulse on bass, high jellies twinkle on cymbals).

---

## Final Verification

- [ ] Build succeeds with no warnings.
- [ ] `Jellyfish` appears under the Field section in the UI with the GEN badge.
- [ ] Enabling adds it to the transform pipeline list and renders the bioluminescent scene.
- [ ] Drag-drop reorders the effect in the pipeline.
- [ ] All listed parameters are modulatable (visible in the modulation source picker for `jellyfish.*` ids).
- [ ] Preset save/load round-trips every field.
- [ ] No water-surface band, no Reinhard tonemap artifacts.
- [ ] No `* belowSurf` or `* aboveMask` masks remain in the shader; snow, jellyfish, and caustics fill the frame.
- [ ] FFT reactivity: different jellyfish flare with different frequency bands (visible by playing tones at different pitches).
