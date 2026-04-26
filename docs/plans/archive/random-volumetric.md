# Random Volumetric

A stochastic generative-art generator that flies a volumetric raymarched camera through a tanh-modulated tube while a random expression tree chains 28 basis values (polynomials, trig, simplex noise, FFT bands, raymarch hit coordinates) through 10 random operations for 4-40 iterations. Each integer seed produces a unique mathematical universe; `cycleSpeed` auto-advances seeds. 9 internal palette modes, optional gradient LUT blend, and feedback trails with directional drift create smoky persistence. The feature is a "slot machine of mathematical art."

**Research**: `docs/research/random_volumetric.md`

**Reference**: "Random Volumetric V2" by Cotterzz - https://www.shadertoy.com/view/3XGXRW (CC BY-NC-SA 3.0)

## Design

### Types

`src/effects/random_volumetric.h`:

```cpp
#ifndef RANDOM_VOLUMETRIC_EFFECT_H
#define RANDOM_VOLUMETRIC_EFFECT_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct ColorLUT ColorLUT;

struct RandomVolumetricConfig {
  bool enabled = false;

  // Camera (volumetric raymarched tube)
  float tubeSpeed = 4.0f;       // Camera traversal speed (0.5-10.0)
  float tubeRadius = 1.5f;      // Tube cross-section radius (0.5-5.0)
  float tubeAmplitude = 8.0f;   // Lateral swing amplitude (1.0-20.0)
  float rollAmount = 3.0f;      // Camera barrel roll intensity (0.0-6.0)
  float zoom = 4.0f;            // Pattern magnification (1.0-20.0)
  float zoomPulse = 1.5f;       // Auto zoom-breathing amplitude (0.0-5.0)

  // Pattern (expression tree)
  float seed = 0.0f;            // Base seed (0.0-1000.0)
  float cycleSpeed = 0.4f;      // Seeds per second auto-advance (0.0-10.0)
  float iterMin = 4.0f;         // Min expression-tree iterations (1-20, stored as float for modulation)
  float iterMax = 40.0f;        // Max expression-tree iterations (5-80, stored as float for modulation)

  // Color
  float paletteRandomness = 1.0f; // 0=gradient LUT only, 1=stochastic palette only (0.0-1.0)
  float contrast = 1.5f;          // Output contrast enhancement (0.5-3.0)

  // Feedback trail
  float driftX = 0.0f;          // Horizontal trail drift (-5.0-5.0, pixels per frame)
  float driftY = 0.0f;          // Vertical trail drift (-5.0-5.0, pixels per frame)
  float decay = 0.95f;          // Trail persistence (0.8-1.0; 0.8=fast fade, 1.0=infinite)

  // Audio (FFT basis values 24-27)
  float baseFreq = 55.0f;       // Lowest FFT band (27.5-440)
  float maxFreq = 14000.0f;     // Highest FFT band (1000-16000)
  float gain = 2.0f;            // FFT energy amplification (0.1-10)
  float curve = 1.5f;           // FFT response curve exponent (0.1-3, UI label "Contrast")
  float baseBright = 0.15f;     // Minimum FFT-band brightness floor (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define RANDOM_VOLUMETRIC_CONFIG_FIELDS                                        \
  enabled, tubeSpeed, tubeRadius, tubeAmplitude, rollAmount, zoom, zoomPulse,  \
      seed, cycleSpeed, iterMin, iterMax, paletteRandomness, contrast,         \
      driftX, driftY, decay, baseFreq, maxFreq, gain, curve, baseBright,       \
      gradient, blendMode, blendIntensity

typedef struct RandomVolumetricEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // Owned ping-pong feedback textures (allocated at init/resize)
  RenderTexture2D pingPong[2];
  int readIdx;

  // CPU-accumulated phases (per memory/feedback_speed_accumulation - never multiply by speed in shader)
  float time;          // Raw accumulator (deltaTime each frame)
  float cameraPhase;   // tubeSpeed-weighted accumulator (tubeSpeed * deltaTime)

  // Uniform locations
  int resolutionLoc;
  int timeLoc;
  int cameraPhaseLoc;
  int tubeRadiusLoc;
  int tubeAmplitudeLoc;
  int rollAmountLoc;
  int zoomLoc;
  int zoomPulseLoc;
  int seedLoc;
  int cycleSpeedLoc;
  int iterMinLoc;
  int iterMaxLoc;
  int paletteRandomnessLoc;
  int contrastLoc;
  int driftXLoc;
  int driftYLoc;
  int decayLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} RandomVolumetricEffect;

bool RandomVolumetricEffectInit(RandomVolumetricEffect *e,
                                const RandomVolumetricConfig *cfg, int width,
                                int height);
void RandomVolumetricEffectSetup(RandomVolumetricEffect *e,
                                 const RandomVolumetricConfig *cfg,
                                 float deltaTime, const Texture2D &fftTexture);
void RandomVolumetricEffectRender(RandomVolumetricEffect *e,
                                  const RandomVolumetricConfig *cfg,
                                  int screenWidth, int screenHeight);
void RandomVolumetricEffectResize(RandomVolumetricEffect *e, int width,
                                  int height);
void RandomVolumetricEffectUninit(RandomVolumetricEffect *e);
void RandomVolumetricRegisterParams(RandomVolumetricConfig *cfg);

#endif
```

### Algorithm

**Custom render flow** (per `ripple_tank` precedent):

1. CPU `RenderRandomVolumetric(PostEffect *pe)` calls `RandomVolumetricEffectRender(e, cfg, screenWidth, screenHeight)`.
2. `Render` does: `writeIdx = 1 - readIdx`; `BeginTextureMode(pingPong[writeIdx])`; `BeginShaderMode(shader)`; bind FFT/LUT textures (already bound by Setup); draw fullscreen quad sampling `pingPong[readIdx]` via raylib auto-bound `texture0`; `EndShaderMode`; `EndTextureMode`; `readIdx = writeIdx`.
3. Then `SetupRandomVolumetricBlend` calls `BlendCompositorApply(pe->blendCompositor, pingPong[readIdx].texture, blendIntensity, blendMode)` to composite the just-written buffer onto the pipeline output.

**Phase accumulation** (CPU, in `RandomVolumetricEffectSetup`):

```cpp
e->time += deltaTime;
e->cameraPhase += cfg->tubeSpeed * deltaTime;
const float WRAP = 1000.0f;
if (e->time > WRAP) e->time -= WRAP;
if (e->cameraPhase > WRAP) e->cameraPhase -= WRAP;
```

**FFT injection**: 4 log-spaced bands from `baseFreq` to `maxFreq` populate `values[24..27]`. Each band: `freq = baseFreq * pow(maxFreq/baseFreq, b/3)`; `bin = freq / (sampleRate * 0.5)`; `energy = texture(fftTexture, vec2(bin, 0.5)).r`; `values[24+b] = baseBright + pow(clamp(energy * gain, 0, 1), curve)`.

**Gradient LUT blend**: When `paletteRandomness < 1.0`, sample `gradientLUT` at `lutT = fract(abs(total))` and `mix(lutCol, stochasticCol, paletteRandomness)`.

**Contrast fold-in**: Original Shadertoy used a separate Image pass for `contrast(color, 1.5)`. Here, fold into the main shader before final output: `mixed = 0.5 + contrast * (mixed - 0.5)`.

**Full shader** (`shaders/random_volumetric.fs`):

```glsl
// Based on "Random Volumetric V2" by Cotterzz
// https://www.shadertoy.com/view/3XGXRW
// License: CC BY-NC-SA 3.0 Unported

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;       // previous frame (ping-pong, auto-bound by raylib)
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float time;               // CPU-accumulated raw phase
uniform float cameraPhase;        // CPU-accumulated tubeSpeed-weighted phase

uniform float tubeRadius;
uniform float tubeAmplitude;
uniform float rollAmount;
uniform float zoom;
uniform float zoomPulse;

uniform float seed;
uniform float cycleSpeed;
uniform float iterMin;
uniform float iterMax;

uniform float paletteRandomness;
uniform float contrast;

uniform float driftX;
uniform float driftY;
uniform float decay;

uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

#define rot(a) mat2(cos(a + vec4(0,33,11,0)))
#define P(z) (vec3(tanh(cos((z) * .15) * 1.) * tubeAmplitude, \
                   tanh(cos((z) * .12) * 1.) * tubeAmplitude, (z)))

float hash1(vec2 x) {
    uvec2 t = floatBitsToUint(x);
    uint h = 0xc2b2ae3du * t.x + 0x165667b9u;
    h = (h << 17u | h >> 15u) * 0x27d4eb2fu;
    h += 0xc2b2ae3du * t.y;
    h = (h << 17u | h >> 15u) * 0x27d4eb2fu;
    h ^= h >> 15u;
    h *= 0x85ebca77u;
    h ^= h >> 13u;
    h *= 0xc2b2ae3du;
    h ^= h >> 16u;
    return uintBitsToFloat(h >> 9u | 0x3f800000u) - 1.0;
}

vec2 hash2(vec2 x) {
    float k = 6.283185307 * hash1(x);
    return vec2(cos(k), sin(k));
}

float noise2(in vec2 p) {
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;
    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x);
    vec2 o = vec2(m, 1.0 - m);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);
    vec3 n = h*h*h * vec3(dot(a, hash2(i + 0.0)),
                           dot(b, hash2(i + o)),
                           dot(c, hash2(i + 1.0)));
    return dot(n, vec3(32.99));
}

float pnoise(vec2 uv) {
    float f = 0.0;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    f  = 0.5000 * noise2(uv); uv = m * uv;
    f += 0.2500 * noise2(uv); uv = m * uv;
    f += 0.1250 * noise2(uv); uv = m * uv;
    f += 0.0625 * noise2(uv);
    f = 0.5 + 0.5 * f;
    return f;
}

float rand(float n) {
    return fract(cos(n * 89.42) * 343.42);
}

float map(vec3 p) {
    return tubeRadius - length(p - P(p.z));
}

float rMix(float a, float b, float s) {
    s = rand(s);
    return s>0.9?sin(a):s>0.8?sqrt(abs(a)):s>0.7?a+b:s>0.6?a-b:s>0.5?b-a:s>0.4?b/(a==0.?0.01:a):s>0.3?pnoise(vec2(a,b)):s>0.2?a/(b==0.?0.01:b):s>0.1?a*b:cos(a);
}

vec3 hsl2rgb(in vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

vec3 contrastFn(vec3 color, float value) {
    return 0.5 + value * (color - 0.5);
}

vec3 fhexRGB(float fh) {
    if(isinf(fh) || fh > 100000.) { fh = 0.; }
    fh = abs(fh * 10000000.);
    float r = fract(fh / 65536.);
    float g = fract(fh / 256.);
    float b = fract(fh / 16777216.);
    return hsl2rgb(vec3(r,g,b));
}

int PALETTE = 0;

vec3 addColor(float num, float seedV, float alt) {
    if(isinf(num)) { num = alt * seedV; }
    if(PALETTE == 7) {
        return fhexRGB(num);
    } else if(PALETTE > 2 || (PALETTE == 1 && rand(seedV+19.) > 0.3)) {
        float sat = 1.;
        if(num < 0.) { sat = 1. - (1./(abs(num)+1.)); }
        float light = 1.0 - (1./(abs(num)+1.));
        vec3 col = hsl2rgb(vec3(fract(abs(num)), sat, light));
        if(PALETTE == 1) { col *= 2.; }
        return col;
    } else {
        vec3 col = vec3(fract(abs(num)), 1./num, 1.-fract(abs(num)));
        if(rand(seedV*2.) > 0.5) { col = col.gbr; }
        if(rand(seedV*3.) > 0.5) { col = col.gbr; }
        if(PALETTE == 1) { col += (1.+cos(rand(num)+vec3(4,2,1))) / 2.; }
        return col;
    }
}

vec3 sanitize(vec3 dc) {
    dc.r = min(1., abs(dc.r));
    dc.g = min(1., abs(dc.g));
    dc.b = min(1., abs(dc.b));
    if(!(dc.r >= 0.) && !(dc.r < 0.)) { return vec3(1,0,0); }
    else if(!(dc.g >= 0.) && !(dc.g < 0.)) { return vec3(1,0,0); }
    else if(!(dc.b >= 0.) && !(dc.b < 0.)) { return vec3(1,0,0); }
    return dc;
}

void main() {
    vec3 r = vec3(resolution, 0.);
    vec2 fragCoord = fragTexCoord * resolution;
    vec2 u = (fragCoord - r.xy/2.) / r.y;

    // Volumetric raymarch (80 steps)
    float s = .002, d = 0.;
    vec3 p = P(cameraPhase), ro = p,
         Z = normalize(P(cameraPhase + 1.) - p),
         X = normalize(vec3(Z.z, 0, -Z.x)),
         D = vec3(rot(tanh(sin(p.z*.03)*8.)*rollAmount) * u, 1)
             * mat3(-X, cross(X, Z), Z);
    for(int rm = 0; rm < 80 && s > .001; rm++) {
        p = ro + D * d;
        s = map(p) * .8;
        d += s;
    }
    p = ro + D * d;

    // Pattern UV (centered, with auto zoom-breathing pulse)
    vec2 uv = fragCoord / resolution.y;
    uv.x -= 0.5 * resolution.x / resolution.y;
    uv.y -= 0.5;
    float zoomVal = zoom + (zoomPulse * (sin(time) + 1.));
    vec2 guv = uv * zoomVal;
    float x = guv.x;
    float y = guv.y;

    // Effective seed (auto-advances at cycleSpeed)
    float effSeed = seed + floor(time * cycleSpeed);

    // Internal palette: always randomized (one of 0..7) per seed
    PALETTE = int(floor(8.0 * rand(effSeed + 66.0)));

    // 28 basis values (24 original + 4 FFT bands)
    const int v = 28;
    float values[v];
    values[0]  = 1.0;
    values[1]  = p.x;
    values[2]  = x;
    values[3]  = y;
    values[4]  = x*x;
    values[5]  = y*y;
    values[6]  = x*x*x;
    values[7]  = y*y*y;
    values[8]  = x*x*x*x;
    values[9]  = y*y*y*y;
    values[10] = x*y*x;
    values[11] = y*y*x;
    values[12] = sin(y);
    values[13] = cos(y);
    values[14] = sin(x);
    values[15] = cos(x);
    // values[16]/[17] are written twice in the original; the second write wins.
    // This bug is preserved verbatim - the effective basis omits sin^2(y)/cos^2(y).
    values[16] = sin(y)*sin(y);
    values[17] = cos(y)*cos(y);
    values[16] = sin(x)*sin(x);
    values[17] = cos(x)*cos(x);
    values[18] = p.y;
    values[19] = distance(vec2(x,y), vec2(0));
    values[20] = p.z;
    values[21] = atan(x, y) * 4.;
    values[22] = pnoise(vec2(x,y) / 2.);
    values[23] = pnoise(vec2(y,x) * 10.);

    // FFT log-spaced bands
    for (int b = 0; b < 4; b++) {
        float t = float(b) / 3.0;
        float freq = baseFreq * pow(maxFreq / baseFreq, t);
        float bin = freq / (sampleRate * 0.5);
        float energy = 0.0;
        if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
        values[24 + b] = baseBright + pow(clamp(energy * gain, 0.0, 1.0), curve);
    }

    // Stochastic expression tree
    float total = 0.;
    float sub = 0.;
    int iterMaxI = int(iterMax);
    int iterMinI = int(iterMin);
    int iterations = min(iterMaxI,
                         iterMinI + int(floor(rand(effSeed * 6.6) * float(iterMaxI - iterMinI))));

    vec3 col = vec3(0);
    float cn = 1.;

    for(int i = 0; i < iterations; i++) {
        float NEWVALUE  = values[int(floor(float(v) * rand(effSeed + float(i))))]
                          * (sin(time * rand(effSeed + float(i))) * rand(effSeed + float(i)));
        float NEWVALUE2 = values[int(floor(float(v) * rand(effSeed + float(i+5))))]
                          * (sin(time * rand(effSeed + float(i))) * rand(effSeed + float(i+5)));

        if(rand(effSeed + float(i+3)) > rand(effSeed)) {
            sub = sub == 0. ? rMix(NEWVALUE, NEWVALUE2, effSeed + float(i+4))
                            : rMix(sub, rMix(NEWVALUE, NEWVALUE2, effSeed + float(i+4)),
                                   effSeed + float(i));
        } else {
            sub = sub == 0. ? NEWVALUE
                            : rMix(sub, NEWVALUE, effSeed + float(i));
        }
        if(rand(effSeed + float(i)) > rand(effSeed) / 2.) {
            total = total == 0. ? sub : rMix(total, sub, effSeed + float(i*2));
            sub = 0.;
            if(rand(effSeed + float(i+30)) > rand(effSeed)) {
                col += addColor(total, effSeed + float(i), values[21]);
                cn += 1.;
            }
        }
    }
    total = sub == 0. ? total : rMix(total, sub, effSeed);
    col += addColor(total, effSeed, values[21]);
    col /= cn;

    // Palette post-processing (verbatim from research)
    if(PALETTE < 3) { col /= (3. * (0.5 + rand(effSeed + 13.))); }
    if(PALETTE == 4) { col = pow(col, 1./col) * 1.5; }
    if(PALETTE == 2 || PALETTE == 5) { col = hsl2rgb(col); }
    if(PALETTE == 6) {
        col = hsl2rgb(hsl2rgb(col));
        if(rand(effSeed + 17.) > 0.5) { col = col.gbr; }
        if(rand(effSeed + 19.) > 0.5) { col = col.gbr; }
    }

    col = sanitize(col);

    // Gradient LUT blend (active when paletteRandomness < 1)
    if (paletteRandomness < 1.0) {
        float lutT = fract(abs(total));
        vec3 lutCol = texture(gradientLUT, vec2(lutT, 0.5)).rgb;
        col = mix(lutCol, col, paletteRandomness);
    }

    // Feedback trail with directional drift and decay
    uv += vec2(0.25, 0.15);  // preserved from reference (dead code; uv not read after)
    vec2 driftPx = vec2(driftX, driftY);
    vec3 old = textureLod(texture0, (fragCoord - driftPx) / resolution, 0.).rgb * decay;
    float alph = (col.r + col.g + col.b) / 5.;
    vec3 mixed = mix(old, col, alph);

    // Output contrast (folded from original Image-pass contrast(.,1.5))
    mixed = contrastFn(mixed, contrast);

    finalColor = vec4(mixed, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Section | UI Label | Widget |
|-----------|------|-------|---------|-------------|------------|----------|--------|
| tubeSpeed | float | 0.5-10.0 | 4.0 | yes | Camera | Tube Speed | ModulatableSlider %.2f |
| tubeRadius | float | 0.5-5.0 | 1.5 | yes | Camera | Tube Radius | ModulatableSlider %.2f |
| tubeAmplitude | float | 1.0-20.0 | 8.0 | yes | Camera | Tube Amplitude | ModulatableSlider %.2f |
| rollAmount | float | 0.0-6.0 | 3.0 | yes | Camera | Roll Amount | ModulatableSlider %.2f |
| zoom | float | 1.0-20.0 | 4.0 | yes | Camera | Zoom | ModulatableSlider %.2f |
| zoomPulse | float | 0.0-5.0 | 1.5 | yes | Camera | Zoom Pulse | ModulatableSlider %.2f |
| seed | float | 0.0-1000.0 | 0.0 | yes | Pattern | Seed | ModulatableSlider %.1f |
| cycleSpeed | float | 0.0-10.0 | 0.4 | yes | Pattern | Cycle Speed | ModulatableSlider %.2f |
| iterMin | float | 1-20 | 4 | yes | Pattern | Iter Min | ModulatableSliderInt |
| iterMax | float | 5-80 | 40 | yes | Pattern | Iter Max | ModulatableSliderInt |
| paletteRandomness | float | 0.0-1.0 | 1.0 | yes | Color | Palette Randomness | ModulatableSlider %.2f |
| contrast | float | 0.5-3.0 | 1.5 | yes | Color | Output Contrast | ModulatableSlider %.2f |
| driftX | float | -5.0-5.0 | 0.0 | yes | Feedback | Drift X | ModulatableSlider %.2f |
| driftY | float | -5.0-5.0 | 0.0 | yes | Feedback | Drift Y | ModulatableSlider %.2f |
| decay | float | 0.8-1.0 | 0.95 | yes | Feedback | Decay | ModulatableSlider %.3f |
| baseFreq | float | 27.5-440 | 55.0 | yes | Audio | Base Freq (Hz) | ModulatableSlider %.1f |
| maxFreq | float | 1000-16000 | 14000 | yes | Audio | Max Freq (Hz) | ModulatableSlider %.0f |
| gain | float | 0.1-10 | 2.0 | yes | Audio | Gain | ModulatableSlider %.1f |
| curve | float | 0.1-3 | 1.5 | yes | Audio | Contrast | ModulatableSlider %.2f |
| baseBright | float | 0-1 | 0.15 | yes | Audio | Base Bright | ModulatableSlider %.2f |
| gradient | ColorConfig | mode=GRADIENT | - | - | Output | (color widget) | ImGuiDrawColorMode |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Output | Blend Intensity | ModulatableSlider %.2f (in STANDARD_GENERATOR_OUTPUT) |
| blendMode | enum | SCREEN default | - | - | Output | Blend Mode | ImGui::Combo (in STANDARD_GENERATOR_OUTPUT) |

UI section order: **Audio -> Camera -> Pattern -> Color -> Feedback -> Output**.

### Constants

- Enum: `TRANSFORM_RANDOM_VOLUMETRIC`
- Display name: `"Random Volumetric"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR_FULL`)
- Section: `13` (Field)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-set by `REGISTER_GENERATOR_FULL`)
- Param prefix: `randomVolumetric.` (auto-derived from `field` argument)

### Intentional deviations from research substitution table

1. **`time * tubeSpeed` -> `cameraPhase` uniform** (CPU-accumulated as `cameraPhase += tubeSpeed * deltaTime`). Per `memory/feedback_speed_accumulation`: speed is always accumulated on CPU, never multiplied in shader. Two CPU phases on the Effect struct: `time` (raw) and `cameraPhase` (tubeSpeed-weighted).
2. **`prevFrame` sampler2D -> raylib-auto-bound `texture0`**. Per `ripple_tank` precedent, the previous-frame ping-pong is bound implicitly by `RenderUtilsDrawFullscreenQuad` and read via `texture0`. No separate `prevFrame` uniform.
3. **Output Contrast UI label**. The standard Audio block labels the FFT `curve` slider as "Contrast"; to avoid a duplicate-label collision, the post-feedback `contrast` param uses the label "Output Contrast". Field name remains `contrast`.
4. **Camera right-axis `vec3(Z.z, 0, -Z)` -> `vec3(Z.z, 0, -Z.x)`**. The original Shadertoy expression is malformed GLSL (a vec3 inside a vec3 constructor where a scalar is expected) and would not compile under GLSL 330. The mathematically correct right-axis for a Y-up camera basis is `cross(up, forward) = vec3(Z.z, 0, -Z.x)`, so `.x` is the intended component.

### Research substitution-table entries that are intentionally ignored

- **`Alpha divisor 5.` -> `(col.r+col.g+col.b) / trailOpacity`** (research substitution table). The research doc suggests making this divisor a uniform named `trailOpacity` "adjustable via decay interaction", but never adds it to the Add table, Parameters table, or Modulation Candidates. The plan keeps `5.` verbatim from the reference; trail intensity is already controllable via `decay` and `blendIntensity`. No `trailOpacity` uniform is introduced.

---

## Tasks

### Wave 1: Header (foundation)

#### Task 1.1: Create RandomVolumetric header

**Files**: `src/effects/random_volumetric.h`
**Creates**: `RandomVolumetricConfig`, `RandomVolumetricEffect`, `RANDOM_VOLUMETRIC_CONFIG_FIELDS` macro, lifecycle/RegisterParams declarations.

**Do**: Create the header verbatim from the Design > Types section. Follow the layout of `src/effects/ripple_tank.h` for ordering (config struct, fields macro, effect struct with ping-pong + accumulators + uniform locations, function declarations). Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `typedef struct ColorLUT ColorLUT;` so the header doesn't need `color_lut.h`.

**Verify**: `cmake.exe --build build` succeeds (header compiles when included; header alone does not produce code yet).

---

### Wave 2: Implementation (parallel - no file overlap)

#### Task 2.1: Implement RandomVolumetric effect module

**Files**: `src/effects/random_volumetric.cpp`
**Depends on**: Task 1.1 (header types).

**Do**: Implement the full effect module following `src/effects/ripple_tank.cpp` as the structural template (ping-pong-owning generator with custom render). Use `src/effects/geode.cpp` and `src/effects/dancing_lines.cpp` as references for FFT audio binding, gradient LUT init/update/bind, and the standard Audio UI block.

Includes (alphabetized within groups, clang-format will sort):
- Own header: `"random_volumetric.h"`
- Project: `"audio/audio.h"` (for `AUDIO_SAMPLE_RATE`), `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `"rlgl.h"`
- ImGui/UI: `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`
- System: `<math.h>`, `<stddef.h>`

Implement these functions:

1. `static void InitPingPong(RandomVolumetricEffect *e, int w, int h)`: call `RenderUtilsInitTextureHDR(&e->pingPong[0], w, h, "RANDOM_VOLUMETRIC")` and `[1]`, then `RenderUtilsClearTexture` on both. (HDR format prevents banding in long-lived feedback.)

2. `static void UnloadPingPong(const RandomVolumetricEffect *e)`: `UnloadRenderTexture(e->pingPong[0])` and `[1]`.

3. `static void CacheLocations(RandomVolumetricEffect *e)`: cache every `*Loc` field via `GetShaderLocation`. Uniform names match the shader: `"resolution"`, `"time"`, `"cameraPhase"`, `"tubeRadius"`, `"tubeAmplitude"`, `"rollAmount"`, `"zoom"`, `"zoomPulse"`, `"seed"`, `"cycleSpeed"`, `"iterMin"`, `"iterMax"`, `"paletteRandomness"`, `"contrast"`, `"driftX"`, `"driftY"`, `"decay"`, `"fftTexture"`, `"sampleRate"`, `"baseFreq"`, `"maxFreq"`, `"gain"`, `"curve"`, `"baseBright"`, `"gradientLUT"`.

4. `bool RandomVolumetricEffectInit(RandomVolumetricEffect *e, const RandomVolumetricConfig *cfg, int w, int h)`:
   - `LoadShader(NULL, "shaders/random_volumetric.fs")` -> check `shader.id == 0` -> return false on fail.
   - `e->gradientLUT = ColorLUTInit(&cfg->gradient)` -> if NULL, `UnloadShader` and return false.
   - `CacheLocations(e)`.
   - `InitPingPong(e, w, h)`. Set `e->readIdx = 0`.
   - Zero `e->time` and `e->cameraPhase`.
   - Return true.

5. `void RandomVolumetricEffectSetup(RandomVolumetricEffect *e, const RandomVolumetricConfig *cfg, float deltaTime, const Texture2D &fftTexture)`:
   - Accumulate `e->time += deltaTime;` and `e->cameraPhase += cfg->tubeSpeed * deltaTime;` Wrap both at `1000.0f` to prevent FP precision loss.
   - `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`.
   - Bind all uniforms: `resolution`, `time`, `cameraPhase`, all camera/pattern/color/feedback floats from cfg, FFT params (`baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`), `sampleRate = (float)AUDIO_SAMPLE_RATE`, `fftTexture`, `gradientLUT = ColorLUTGetTexture(e->gradientLUT)`. Use `SetShaderValue` (`SHADER_UNIFORM_FLOAT` / `SHADER_UNIFORM_VEC2`) and `SetShaderValueTexture`.

6. `void RandomVolumetricEffectRender(RandomVolumetricEffect *e, const RandomVolumetricConfig *cfg, int screenWidth, int screenHeight)`:
   - `(void)cfg;`
   - `const int writeIdx = 1 - e->readIdx;`
   - `BeginTextureMode(e->pingPong[writeIdx]); BeginShaderMode(e->shader);`
   - `RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth, screenHeight);` (this binds the read buffer as `texture0`).
   - `EndShaderMode(); EndTextureMode();`
   - `e->readIdx = writeIdx;`

7. `void RandomVolumetricEffectResize(RandomVolumetricEffect *e, int w, int h)`: `UnloadPingPong(e); InitPingPong(e, w, h); e->readIdx = 0;`

8. `void RandomVolumetricEffectUninit(RandomVolumetricEffect *e)`: `UnloadShader(e->shader); ColorLUTUninit(e->gradientLUT); UnloadPingPong(e);`

9. `void RandomVolumetricRegisterParams(RandomVolumetricConfig *cfg)`: register every modulatable param via `ModEngineRegisterParam("randomVolumetric.<field>", &cfg-><field>, min, max)` with the bounds from the Parameters table. Register: `tubeSpeed, tubeRadius, tubeAmplitude, rollAmount, zoom, zoomPulse, seed, cycleSpeed, iterMin, iterMax, paletteRandomness, contrast, driftX, driftY, decay, baseFreq, maxFreq, gain, curve, baseBright, blendIntensity`. Do NOT register `iterMin`/`iterMax` ranges as their stored-as-float ranges (1..20 and 5..80); register them at those float bounds.

10. **Bridge functions (NON-static, just above the registration macro)**:

    ```cpp
    void SetupRandomVolumetric(PostEffect *pe) {
      RandomVolumetricEffectSetup(&pe->randomVolumetric, &pe->effects.randomVolumetric,
                                  pe->currentDeltaTime, pe->fftTexture);
    }

    void SetupRandomVolumetricBlend(PostEffect *pe) {
      BlendCompositorApply(pe->blendCompositor,
                           pe->randomVolumetric.pingPong[pe->randomVolumetric.readIdx].texture,
                           pe->effects.randomVolumetric.blendIntensity,
                           pe->effects.randomVolumetric.blendMode);
    }

    void RenderRandomVolumetric(PostEffect *pe) {
      RandomVolumetricEffectRender(&pe->randomVolumetric, &pe->effects.randomVolumetric,
                                   pe->screenWidth, pe->screenHeight);
    }
    ```

11. **`// === UI ===` section**:
    - `static void DrawRandomVolumetricParams(EffectConfig *e, const ModSources *modSources, ImU32 categoryGlow)`. `(void)categoryGlow;` Get `RandomVolumetricConfig *cfg = &e->randomVolumetric;`.
    - Section order: **Audio** (5 standard sliders verbatim from `dancing_lines.cpp` audio block, with `##rv` ID suffix and `randomVolumetric.<field>` param IDs), **Camera** (tubeSpeed, tubeRadius, tubeAmplitude, rollAmount, zoom, zoomPulse), **Pattern** (seed, cycleSpeed, iterMin via `ModulatableSliderInt`, iterMax via `ModulatableSliderInt`), **Color** (paletteRandomness, contrast labelled "Output Contrast"), **Feedback** (driftX, driftY, decay).
    - Below, `STANDARD_GENERATOR_OUTPUT(randomVolumetric)` (auto-renders gradient widget + Output separator + Blend Intensity + Blend Mode).

12. **Registration macro** (last lines):

    ```cpp
    // clang-format off
    STANDARD_GENERATOR_OUTPUT(randomVolumetric)
    REGISTER_GENERATOR_FULL(TRANSFORM_RANDOM_VOLUMETRIC, RandomVolumetric, randomVolumetric,
                            "Random Volumetric",
                            SetupRandomVolumetricBlend, SetupRandomVolumetric,
                            RenderRandomVolumetric, 13,
                            DrawRandomVolumetricParams, DrawOutput_randomVolumetric)
    // clang-format on
    ```

**Verify**: `cmake.exe --build build` succeeds; the registration macro forward-declares the three bridge functions (so they must be non-static); UI dispatch hits this effect under section 13 (Field).

---

#### Task 2.2: Create the Random Volumetric shader

**Files**: `shaders/random_volumetric.fs`
**Depends on**: nothing structural; can run in parallel with 2.1, 2.3, 2.4, 2.5.

**Do**: Create the shader verbatim from the Design > Algorithm section's "Full shader" block. Includes the attribution comment header (Cotterzz / Shadertoy URL / CC BY-NC-SA 3.0). Do not add Reinhard tonemap. Do not introduce additional uniforms beyond those listed. Do not change any of the verbatim hash/noise/rMix/addColor/sanitize bodies. Preserve the `values[16]/[17]` double-write bug (it is documented in the research as deliberately preserved). Preserve the `iterMin`/`iterMax` cast to int. Preserve the GLSL 330 features (`floatBitsToUint`, `uintBitsToFloat`, `tanh`, `isinf`, `textureLod`).

**Verify**: Shader file exists at the correct path; the shader compiles at runtime when the effect is loaded (verified by `RandomVolumetricEffectInit` returning true and the effect rendering without `shader.id == 0` early-out).

---

#### Task 2.3: Wire RandomVolumetric into EffectConfig

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1 (header).

**Do**: Four-point modification:

1. Add `#include "effects/random_volumetric.h"` in the alphabetical effect-includes block.
2. Add `TRANSFORM_RANDOM_VOLUMETRIC,` to the `TransformEffectType` enum, before `TRANSFORM_EFFECT_COUNT` and any sentinel like `TRANSFORM_ACCUM_COMPOSITE`. Place adjacent to other generators if grouping is consistent; otherwise just before the last entry.
3. The default `TransformOrderConfig` constructor populates `order[i] = (TransformEffectType)i;` for all enum values, so no manual order-array edit is needed beyond adding the enum value.
4. Add `RandomVolumetricConfig randomVolumetric;` member to `EffectConfig` struct (alongside other generator configs). Add a one-line comment: `// Random Volumetric (raymarched tube + stochastic expression tree + feedback trail)`.

**Verify**: `cmake.exe --build build` succeeds.

---

#### Task 2.4: Wire RandomVolumetric into PostEffect

**Files**: `src/render/post_effect.h`
**Depends on**: Task 1.1 (header).

**Do**: Two-point modification:

1. Add `#include "effects/random_volumetric.h"` in the alphabetical effect-includes block.
2. Add `RandomVolumetricEffect randomVolumetric;` member to the `PostEffect` struct, alongside other generator effect members.

**Verify**: `cmake.exe --build build` succeeds.

---

#### Task 2.5: Wire RandomVolumetric into preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Task 1.1 (header for `RANDOM_VOLUMETRIC_CONFIG_FIELDS` macro).

**Do**: Three-point modification:

1. Add `#include "effects/random_volumetric.h"` in the alphabetical includes block at the top of the file.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RandomVolumetricConfig, RANDOM_VOLUMETRIC_CONFIG_FIELDS)` alongside the other generator config registrations.
3. Append `X(randomVolumetric) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (the table consumed by both `to_json` and `from_json` of `EffectConfig`).

**Verify**: `cmake.exe --build build` succeeds; saving a preset with the effect enabled and reloading restores all fields.

---

## Final Verification

- [ ] Build succeeds with no warnings.
- [ ] "Random Volumetric" appears under the **Field** generator section (section 13) with the `GEN` badge.
- [ ] Enabling the effect shows the visual: a volumetric raymarched tube with stochastic expression-tree fragments overlaid, plus a feedback trail.
- [ ] `cycleSpeed > 0` advances seeds at the configured rate; `cycleSpeed = 0` holds a static pattern.
- [ ] `seed` modulation jumps between mathematical universes (discrete jumps are expected, not smooth).
- [ ] `decay = 1.0` retains old content indefinitely; `decay = 0.8` fades quickly.
- [ ] `driftX`/`driftY` smear the trail directionally.
- [ ] `paletteRandomness = 0.0` displays only the gradient LUT colors; `1.0` shows only the stochastic palette.
- [ ] FFT injection responds: increasing `gain` makes the visual pulse with audio (exact response varies by seed - some seeds incorporate FFT bands heavily, others ignore them).
- [ ] `iterMax` reduction cuts GPU cost noticeably; `iterMax = 80` is the heaviest setting.
- [ ] Modulating `decay` and `driftX/Y` from competing audio sources produces the documented bass/treble tension.
- [ ] All modulatable params accept routes from LFOs, mod buses, and audio sources.
- [ ] Save preset with the effect enabled; reload; all 24 fields restore correctly (including `gradient`, `blendMode`, `blendIntensity`).
- [ ] Window resize reallocates ping-pong buffers without crashing.

---

## Changes from plan

- Removed params: `tubeSpeed`, `tubeRadius`, `tubeAmplitude`, `rollAmount`, `contrast`, `decay`, `driftX`, `driftY`.
- Camera constants hardcoded in shader (`tubeAmplitude=8.0`, `tubeRadius=1.5`, `rollAmount=3.0`); `cameraPhase` accumulated CPU-side at `4.0 * deltaTime`.
- Feedback removed: no ping-pong, no custom render, no resize, no `EFFECT_FLAG_NEEDS_RESIZE`. Switched from `REGISTER_GENERATOR_FULL` to `REGISTER_GENERATOR`.
- `paletteRandomness` semantics: `lutCol + paletteRandomness * (stochCol - 0.5)` (LUT is base, stochastic is centered offset). Plan had `mix(lutCol, stochCol, paletteRandomness)`.
- FFT path: `values[24..27]` removed (back to `v = 24`); FFT no longer enters the expression tree. Replaced with shared-`t` band lookup (`baseFreq * pow(maxFreq/baseFreq, t)`, `BAND_SAMPLES = 4` sub-samples) producing `mag`; applied as `col *= baseBright + mag`. FFT affects color brightness only.
