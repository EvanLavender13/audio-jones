# Galaxy

A spiral galaxy generator filling the screen — concentric elliptical rings with progressive rotation create spiral arm structure. Inner rings carry bass energy, outer rings carry treble. Dust detail from value noise, embedded point stars with twinkle and rare supernova flashes, and a Gaussian central bulge. Style params (twist, stretch, ring width) are exposed directly so the shape can range from tight spiral to diffuse elliptical.

**Research**: `docs/research/galaxy.md`

## Design

### Types

```c
// galaxy.h

struct GalaxyConfig {
  bool enabled = false;

  // Geometry
  int layers = 20;               // Ring count (8-25)
  float twist = 1.0f;            // Spiral winding per ring (0.0-2.0)
  float innerStretch = 2.0f;     // Inner ring X elongation (1.0-4.0)
  float ringWidth = 15.0f;       // Gaussian ring sharpness (4.0-30.0)
  float diskThickness = 0.04f;   // Ring Y perturbation (0.01-0.15)
  float tilt = 0.3f;             // Viewing angle in radians (0.0-1.57)
  float rotation = 0.0f;         // In-plane rotation in radians (-PI..PI)

  // Animation
  float orbitSpeed = 0.1f;       // Orbital animation speed (-1.0-1.0)

  // Dust & Stars
  float dustContrast = 0.5f;     // Dust pow() exponent (0.1-1.5)
  float starDensity = 8.0f;      // Star grid resolution per ring (2.0-16.0)
  float starBright = 0.2f;       // Star point intensity (0.05-1.0)

  // Bulge
  float bulgeSize = 25.0f;       // Center glow Gaussian tightness (5.0-50.0)
  float bulgeBright = 1.2f;      // Center glow intensity (0.0-3.0)

  // Audio
  float baseFreq = 55.0f;        // Lowest FFT frequency (27.5-440.0)
  float maxFreq = 14000.0f;      // Highest FFT frequency (1000-16000)
  float gain = 2.0f;             // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;            // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f;      // Ring brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define GALAXY_CONFIG_FIELDS                                                   \
  enabled, layers, twist, innerStretch, ringWidth, diskThickness, tilt,        \
      rotation, orbitSpeed, dustContrast, starDensity, starBright, bulgeSize,   \
      bulgeBright, baseFreq, maxFreq, gain, curve, baseBright, gradient,       \
      blendMode, blendIntensity

typedef struct GalaxyEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;           // CPU-accumulated animation time
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int layersLoc;
  int twistLoc;
  int innerStretchLoc;
  int ringWidthLoc;
  int diskThicknessLoc;
  int tiltLoc;
  int rotationLoc;
  int orbitSpeedLoc;
  int dustContrastLoc;
  int starDensityLoc;
  int starBrightLoc;
  int bulgeSizeLoc;
  int bulgeBrightLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} GalaxyEffect;
```

### Algorithm

The shader is a single-pass fragment shader. All GLSL below is the authoritative source — agents implement from this section, not from the research doc.

#### Noise utilities

```glsl
float hashN2(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

float valueNoise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hashN2(i + vec2(0.0, 0.0)), hashN2(i + vec2(1.0, 0.0)), u.x),
               mix(hashN2(i + vec2(0.0, 1.0)), hashN2(i + vec2(1.0, 1.0)), u.x), u.y);
}
```

#### Constants

```glsl
#define GAL_TAU 6.2831853
#define GAL_MAX_RADIUS 1.5
#define GAL_MIN_COS_TILT 0.15
#define GAL_RING_PHASE_OFFSET 100.0
#define GAL_DUST_UV_SCALE 0.2
#define GAL_DUST_NOISE_FREQ 4.0
#define GAL_STAR_GLOW_RADIUS 0.5
#define GAL_SUPERNOVA_THRESH 0.9999
#define GAL_SUPERNOVA_MULT 10.0
#define GAL_INNER_RADIUS 0.1
#define GAL_OUTER_RADIUS 1.0
#define GAL_MAX_RINGS 25
#define GAL_RING_DECORR_A 563.2
#define GAL_RING_DECORR_B 673.2
#define GAL_STAR_OFFSET_A 17.3
#define GAL_STAR_OFFSET_B 31.7
#define GAL_TWINKLE_FREQ 784.0
#define GAL_SUPERNOVA_TIME_SCALE 0.05
```

#### Rotation helper

```glsl
mat2 galRot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}
```

#### Tilt

```glsl
vec2 applyTilt(vec2 uv, float tiltAngle) {
    uv.y /= max(abs(cos(tiltAngle)), GAL_MIN_COS_TILT);
    return uv;
}
```

#### Bulge

```glsl
vec3 renderBulge(vec2 uv, float size, float brightness, vec3 tint) {
    return vec3(exp(-0.5 * dot(uv, uv) * size)) * brightness * tint;
}
```

#### FFT band lookup (per ring)

Standard BAND_SAMPLES=4 pattern from generator conventions:

```glsl
float t0 = float(j) / float(layers);
float t1 = float(j + 1) / float(layers);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) {
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
}
float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```

#### Ring loop (main body)

```glsl
vec3 renderRings(vec2 uv, float time) {
    vec3 col = vec3(0.0);
    float flip = 1.0;
    float t = time * orbitSpeed;

    for (int j = 0; j < GAL_MAX_RINGS; j++) {
        float i = float(j) / float(layers);
        if (i >= 1.0) break;
        flip *= -1.0;

        // FFT band lookup for this ring (see above)
        // ... yields `brightness`

        // GradientLUT color for this ring
        vec3 dustCol = texture(gradientLUT, vec2(i, 0.5)).rgb;

        float z = mix(diskThickness, 0.0, i) * flip
                  * fract(sin(i * GAL_RING_DECORR_A) * GAL_RING_DECORR_B);
        float r = mix(GAL_INNER_RADIUS, GAL_OUTER_RADIUS, i);
        vec2 ringUv = uv + vec2(0.0, z * 0.5);
        vec2 st = ringUv * galRot(i * GAL_TAU * twist);
        st.x *= mix(innerStretch, 1.0, i);
        float ell = exp(-0.5 * abs(dot(st, st) - r) * ringWidth);
        vec2 texUv = GAL_DUST_UV_SCALE * st * galRot(i * GAL_RING_PHASE_OFFSET + t / r);
        vec3 dust = vec3(valueNoise2D((texUv + vec2(i)) * GAL_DUST_NOISE_FREQ));
        vec3 dL = pow(max(ell * dust / r, vec3(0.0)), vec3(0.5 + dustContrast));
        col += dL * dustCol * brightness;

        // Point Stars
        vec2 starId = floor(texUv * starDensity);
        vec2 starUv = fract(texUv * starDensity) - 0.5;
        float n = hashN2(starId + vec2(i * GAL_STAR_OFFSET_A, i * GAL_STAR_OFFSET_B));
        float starDist = length(starUv);
        float sL = smoothstep(GAL_STAR_GLOW_RADIUS, 0.0, starDist)
                   * pow(max(dL.r, 0.0), 2.0) * starBright
                   / max(starDist, 0.001);
        float sN = sL;
        sL *= sin(n * GAL_TWINKLE_FREQ + time) * 0.5 + 0.5;
        sL += sN * smoothstep(GAL_SUPERNOVA_THRESH, 1.0,
              sin(n * GAL_TWINKLE_FREQ + time * GAL_SUPERNOVA_TIME_SCALE))
              * GAL_SUPERNOVA_MULT;

        if (i > 3.0 / starDensity) {
            vec3 starCol = mix(dustCol, vec3(1.0), 0.3 + n * 0.5);
            col += sL * starCol * brightness;
        }
    }

    col /= float(layers);
    return col;
}
```

#### Main

```glsl
void main() {
    vec2 centered = fragTexCoord * resolution - resolution * 0.5;
    vec2 uv = centered / (min(resolution.x, resolution.y) * 0.5);

    // Apply in-plane rotation and tilt
    uv = applyTilt(uv * galRot(rotation), tilt);

    // Early-out for fragments beyond galaxy radius
    if (length(uv) > GAL_MAX_RADIUS) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 col = renderRings(uv, time);

    // Bulge: tinted toward warm white via gradientLUT center sample
    vec3 bulgeTint = mix(vec3(1.0, 0.9, 0.8),
                         texture(gradientLUT, vec2(0.0, 0.5)).rgb, 0.6);
    col += renderBulge(uv, bulgeSize, bulgeBright, bulgeTint);

    finalColor = vec4(col, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| layers | int | 8-25 | 20 | No | Rings |
| twist | float | 0.0-2.0 | 1.0 | Yes | Twist |
| innerStretch | float | 1.0-4.0 | 2.0 | Yes | Inner Stretch |
| ringWidth | float | 4.0-30.0 | 15.0 | Yes | Ring Width |
| diskThickness | float | 0.01-0.15 | 0.04 | Yes | Disk Thickness |
| tilt | float | 0.0-1.57 | 0.3 | Yes | Tilt |
| rotation | float | -PI..PI | 0.0 | Yes | Rotation |
| orbitSpeed | float | -1.0-1.0 | 0.1 | Yes | Orbit Speed |
| dustContrast | float | 0.1-1.5 | 0.5 | Yes | Dust Contrast |
| starDensity | float | 2.0-16.0 | 8.0 | Yes | Star Density |
| starBright | float | 0.05-1.0 | 0.2 | Yes | Star Bright |
| bulgeSize | float | 5.0-50.0 | 25.0 | Yes | Bulge Size |
| bulgeBright | float | 0.0-3.0 | 1.2 | Yes | Bulge Bright |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |

### Constants

- Enum name: `TRANSFORM_GALAXY_BLEND`
- Display name: `"Galaxy"`
- Category section: 13 (Field)
- Registration: `REGISTER_GENERATOR`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create galaxy.h

**Files**: `src/effects/galaxy.h` (create)
**Creates**: `GalaxyConfig`, `GalaxyEffect` types, function declarations

**Do**: Create the header following the Types section above exactly. Follow spectral_arcs.h as structural pattern:
- Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`
- Forward declare `ColorLUT`
- Define `GalaxyConfig` struct with all fields and defaults from Types section
- Define `GALAXY_CONFIG_FIELDS` macro
- Define `GalaxyEffect` typedef struct with all uniform location ints
- Declare: `GalaxyEffectInit(GalaxyEffect*, const GalaxyConfig*)`, `GalaxyEffectSetup(GalaxyEffect*, const GalaxyConfig*, float, Texture2D)`, `GalaxyEffectUninit(GalaxyEffect*)`, `GalaxyConfigDefault(void)`, `GalaxyRegisterParams(GalaxyConfig*)`

**Verify**: `cmake.exe --build build` compiles (header-only, included later).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create galaxy.cpp

**Files**: `src/effects/galaxy.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the effect module following spectral_arcs.cpp as structural pattern:

Includes (in order): `"galaxy.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`

Functions:
- `GalaxyEffectInit`: Load shader `"shaders/galaxy.fs"`, cache all uniform locations, init gradientLUT via `ColorLUTInit(&cfg->gradient)`, set `time = 0.0f`. On failure, unload shader before returning false.
- `GalaxyEffectSetup`: Accumulate `time += deltaTime` (raw time, not scaled by orbitSpeed — orbitSpeed is a shader uniform). Call `ColorLUTUpdate`. Bind all uniforms: resolution, time, fftTexture (via `SetShaderValueTexture`), sampleRate (`AUDIO_SAMPLE_RATE`), gradientLUT texture, layers (int), and all float params.
- `GalaxyEffectUninit`: Unload shader and gradientLUT.
- `GalaxyConfigDefault`: Return `GalaxyConfig{}`.
- `GalaxyRegisterParams`: Register all modulatable float params with `ModEngineRegisterParam("galaxy.<field>", ...)`. Use `ROTATION_OFFSET_MAX` for rotation bounds. Skip `layers` (int, not modulatable).

Bridge functions (non-static):
- `SetupGalaxy(PostEffect* pe)`: delegates to `GalaxyEffectSetup`
- `SetupGalaxyBlend(PostEffect* pe)`: calls `BlendCompositorApply` with `pe->generatorScratch.texture`, `blendIntensity`, `blendMode`

UI section (`// === UI ===`):
- `static void DrawGalaxyParams(EffectConfig*, const ModSources*, ImU32)`:
  - Audio section: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order/format per conventions)
  - Geometry section: Rings (`ImGui::SliderInt`, 8-25), Twist, Inner Stretch, Ring Width, Disk Thickness
  - View section: Tilt, Rotation (`ModulatableSliderAngleDeg`)
  - Animation section: Orbit Speed
  - Dust & Stars section: Dust Contrast, Star Density, Star Bright
  - Bulge section: Bulge Size, Bulge Bright

Registration at bottom:
```cpp
STANDARD_GENERATOR_OUTPUT(galaxy)
REGISTER_GENERATOR(TRANSFORM_GALAXY_BLEND, Galaxy, galaxy,
                   "Galaxy", SetupGalaxyBlend,
                   SetupGalaxy, 13, DrawGalaxyParams, DrawOutput_galaxy)
```

**Verify**: Compiles.

---

#### Task 2.2: Create galaxy.fs shader

**Files**: `shaders/galaxy.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Create the fragment shader implementing the Algorithm section above.

Attribution header (before `#version`):
```glsl
// Based on "Galaxy Generator Study" by guinetik (technique from "Megaparsecs" by BigWings)
// https://www.shadertoy.com/view/scl3DH
// License: CC BY-NC-SA 3.0 Unported
// Modified: single galaxy, gradientLUT coloring, FFT per-ring brightness, AudioJones uniforms
```

Standard generator uniforms: `resolution` (vec2), `time` (float), `fftTexture` (sampler2D), `sampleRate` (float), `gradientLUT` (sampler2D).

Effect-specific uniforms: `layers` (int), `twist`, `innerStretch`, `ringWidth`, `diskThickness`, `tilt`, `rotation`, `orbitSpeed`, `dustContrast`, `starDensity`, `starBright`, `bulgeSize`, `bulgeBright`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright` (all float).

Implement all functions from the Algorithm section: `hashN2`, `valueNoise2D`, `galRot`, `applyTilt`, `renderBulge`, `renderRings` (with inline FFT band lookup and gradientLUT sampling), and `main`.

Key implementation notes:
- Coordinates: `fragTexCoord * resolution - resolution * 0.5` centered, normalized by `min(resolution.x, resolution.y) * 0.5`
- Early-out at `GAL_MAX_RADIUS 1.5`
- `brightness` from FFT multiplied into both `dL * dustCol` and `sL * starCol`
- `dustCol` from `texture(gradientLUT, vec2(i, 0.5)).rgb` where `i = float(j) / float(layers)`
- Bulge tint blends warm white `(1.0, 0.9, 0.8)` with gradientLUT center `(0.0, 0.5)` at 0.6

**Verify**: Shader file is syntactically valid GLSL 330.

---

#### Task 2.3: Integration — effect_config.h, post_effect.h, CMakeLists.txt, effect_serialization.cpp

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/galaxy.h"` with other generator includes
   - Add `TRANSFORM_GALAXY_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
   - Add `TRANSFORM_GALAXY_BLEND` to `TransformOrderConfig::order` array (at end, before closing brace)
   - Add `GalaxyConfig galaxy;` to `EffectConfig` struct

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/galaxy.h"`
   - Add `GalaxyEffect galaxy;` to `PostEffect` struct (near other generator effects)

3. **`CMakeLists.txt`**:
   - Add `src/effects/galaxy.cpp` to `EFFECTS_SOURCES` (alphabetical placement near other g-names)

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/galaxy.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GalaxyConfig, GALAXY_CONFIG_FIELDS)` with other per-config macros
   - Add `X(galaxy)` to `EFFECT_CONFIG_FIELDS` X-macro

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Implementation Notes

Deviations from the original plan's Algorithm section, discovered during tuning:

### orbitSpeed is CPU-accumulated
The plan had `orbitSpeed` as a shader uniform with `float t = time * orbitSpeed` in the ring loop. This causes time jumps when the user changes speed. Fixed: `time += deltaTime * orbitSpeed` on CPU, removed `orbitSpeed` uniform entirely. Shader uses `float t = time` directly.

### ringWidth and bulgeSize are inverted on CPU
Both params control Gaussian tightness in the shader (higher = tighter/smaller), but the UI labels suggest the opposite ("Ring Width" = wider, "Bulge Size" = bigger). The CPU sends `(min + max) - value` to the shader so the slider direction matches the label. ringWidth: `52 - value` (range 2-50). bulgeSize: `55 - value` (range 5-50).

### Removed `/ r` from dust formula
The reference `dL = pow(ell * dust / r, ...)` dims outer rings proportional to radius. At fullscreen scale with `GAL_OUTER_RADIUS 4.0`, this made outer rings invisible. Removed: `dL = pow(ell * dust, ...)`. Outer ring brightness is now driven purely by ring proximity and dust noise.

### `GAL_OUTER_RADIUS 4.0` (was 1.0)
The ring Gaussian uses `dot(st, st)` (squared distance), so a ring at `r` visually appears at `sqrt(r)`. To fill the screen corners on 16:9 (UV distance ~2.0), outer radius must be `2.0² = 4.0`.

### `GAL_MAX_RADIUS 2.5` (was 1.5)
Early-out radius expanded to accommodate the larger ring extent.

### Normalization: `col /= sqrt(float(layers))` (was `float(layers)`)
With rings spread over a larger radius, only 1-3 overlap at any pixel. Dividing by the full layer count (20) killed brightness. `sqrt(layers)` scales sub-linearly — brightness stays reasonable without blowing out dense regions.

### Star randomization
Stars are offset randomly within their grid cell (`starOffset * 0.7`) to break the visible grid pattern at high density. Brightness multiplied by `n * n` (hash squared) gives natural variation — most stars dim, few bright.

### Tuned defaults
| Param | Plan | Final | Reason |
|-------|------|-------|--------|
| ringWidth | 15 | 42 | Diffuse rings overlap into cloud structure (shader sees 10) |
| bulgeSize | 25 | 20 | Less blobby center (shader sees 35) |
| bulgeBright | 1.2 | 0.6 | Subtler core glow |
| starDensity | 8 | 24 | More stars at fullscreen scale (range expanded to 2-64) |
| diskThickness | 0.04 | 0.04 | Range expanded to 0.01-0.5 (was 0.01-0.15) |

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Galaxy appears in Field section of effects panel
- [ ] Galaxy shows "GEN" badge
- [ ] Enabling Galaxy renders spiral galaxy on screen
- [ ] Audio reactivity: rings brighten with music
- [ ] Twist, tilt, rotation controls visibly change morphology
- [ ] Gradient widget recolors the galaxy
- [ ] Blend mode/intensity controls work
- [ ] Preset save/load preserves all Galaxy settings
- [ ] Modulation routes to registered parameters
