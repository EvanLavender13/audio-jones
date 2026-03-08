# Shell

A raymarched hollow sphere rendered as luminous outline contours. Each march step rotates the view slightly via the `outlineSpread` parameter, spreading the surface into near-miss curves that reveal the shape as a wireframe silhouette rather than a solid fill. Turbulence distorts the sphere into organic nautilus-like forms with flowing contour lines. Per-step FFT reactivity maps frequency bands to march steps for audio-driven glow modulation. Ping-pong trail buffer with decay/blur provides persistence (same pattern as Muons).

**Research**: `docs/research/shell.md`

## Design

### Types

```cpp
// src/effects/shell.h

struct ShellConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 60;              // Ray budget — more steps = denser contours (4-200)
  int turbulenceOctaves = 4;        // Distortion layers (2-12)
  float turbulenceGrowth = 2.0f;    // Octave frequency multiplier (1.2-3.0)
  float sphereRadius = 3.0f;        // Hollow sphere size (0.5-10.0)
  float ringThickness = 0.1f;       // Step size multiplier (0.01-0.5)
  float cameraDistance = 9.0f;      // Depth into volume (3.0-20.0)
  float phaseX = 0.0f;             // Rotation axis X phase offset (-PI_F to PI_F)
  float phaseY = 2.0f;             // Rotation axis Y phase offset (-PI_F to PI_F)
  float phaseZ = 4.0f;             // Rotation axis Z phase offset (-PI_F to PI_F)
  float outlineSpread = 0.1f;      // Per-step rotation amount — 0 solid, higher wireframe (0.0-0.5)

  // Trail persistence
  float decayHalfLife = 2.0f;       // Trail persistence seconds (0.1-10.0)
  float trailBlur = 1.0f;           // Trail blur amount (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;           // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f;         // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;                // FFT sensitivity multiplier (0.1-10.0)
  float curve = 1.5f;               // FFT contrast curve exponent (0.1-3.0)
  float baseBright = 0.15f;         // Minimum brightness floor (0.0-1.0)

  // Color
  float colorSpeed = 0.5f;          // LUT scroll rate (0.0-2.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  float colorStretch = 1.0f;        // Spatial color frequency through depth (0.1-5.0)

  // Tonemap
  float brightness = 1.0f;          // Intensity multiplier before tonemap (0.1-5.0)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;      // (0.0-5.0)
};

#define SHELL_CONFIG_FIELDS                                                    \
  enabled, marchSteps, turbulenceOctaves, turbulenceGrowth, sphereRadius,     \
      ringThickness, cameraDistance, phaseX, phaseY, phaseZ, outlineSpread,   \
      decayHalfLife, trailBlur, baseFreq, maxFreq, gain, curve, baseBright,   \
      colorSpeed, colorStretch, brightness, gradient, blendMode,              \
      blendIntensity
```

```cpp
typedef struct ShellEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float time;
  float colorPhase;
  int resolutionLoc;
  int timeLoc;
  int marchStepsLoc;
  int turbulenceOctavesLoc;
  int turbulenceGrowthLoc;
  int sphereRadiusLoc;
  int ringThicknessLoc;
  int cameraDistanceLoc;
  int phaseLoc;
  int outlineSpreadLoc;
  int colorStretchLoc;
  int colorPhaseLoc;
  int brightnessLoc;
  int gradientLUTLoc;
  int previousFrameLoc;
  int decayFactorLoc;
  int trailBlurLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ShellEffect;
```

Function signatures follow Muons exactly:

```
bool ShellEffectInit(ShellEffect *e, const ShellConfig *cfg, int width, int height);
void ShellEffectSetup(ShellEffect *e, const ShellConfig *cfg, float deltaTime, Texture2D fftTexture);
void ShellEffectRender(ShellEffect *e, const ShellConfig *cfg, float deltaTime, int screenWidth, int screenHeight);
void ShellEffectResize(ShellEffect *e, int width, int height);
void ShellEffectUninit(ShellEffect *e);
ShellConfig ShellConfigDefault(void);
void ShellRegisterParams(ShellConfig *cfg);
```

### Algorithm

Single-pass raymarcher with trail persistence. The shader handles both the raymarching and the ping-pong trail blend in one pass (same structure as `muons.fs`).

**Key differences from Muons:**
- Turbulence uses **subtraction** (`a -=`) and **`.zxy` swizzle** (Muons uses addition and `.yzx`)
- `outlineSpread * float(i)` applied to both the rotation axis (+) AND turbulence phase (−) — these opposite signs create the outline coherence
- Hollow sphere distance: `abs(length(a) - sphereRadius)` (Muons uses `abs(sin(s))`)
- No mode/turbulenceMode variants — single algorithm
- Tonemap divisor is `1000.0` (Muons uses `2e7` for additive or `300 * marchSteps` for winner)
- Turbulence starts at frequency 0.6 with configurable growth (Muons starts at 1.0 with +1 increments)

```glsl
void main() {
    vec2 fragCoord = fragTexCoord * resolution;
    // Ray direction: vec3(2*frag, 0) - vec3(res.x, res.y, res.y) — matches reference (I+I - res.xyy)
    vec3 rayDir = normalize(vec3(fragCoord * 2.0, 0.0) - vec3(resolution.x, resolution.y, resolution.y));

    float z = 0.0;
    float d;
    vec3 color = vec3(0.0);
    float stepCount = float(max(marchSteps - 1, 1));

    for (int i = 0; i < marchSteps; i++) {
        // Sample point along ray
        vec3 p = z * rayDir;

        // Per-step rotation axis — outlineSpread creates the wireframe look
        // Reference: normalize(cos(vec3(0,2,4)+t+.1*i))
        vec3 a = normalize(cos(phase + time + outlineSpread * float(i)));

        // Camera offset
        p.z += cameraDistance;

        // Rodrigues rotation
        a = a * dot(a, p) - cross(a, p);

        // Turbulence — SUBTRACTION and .zxy swizzle (differs from Muons)
        // Reference: for(d=.6;d<9.;d+=d) a-=cos(a*d+t-.1*i).zxy/d
        // Note: turbulence phase uses NEGATIVE outlineSpread (opposite to rotation axis)
        float freq = 0.6;
        for (int oct = 0; oct < turbulenceOctaves; oct++) {
            a -= cos(a * freq + time - outlineSpread * float(i)).zxy / freq;
            freq *= turbulenceGrowth;
        }

        // Distance to hollow sphere
        // Reference: d=.1*abs(length(a)-3.)  (sphere displacement dropped)
        d = ringThickness * abs(length(a) - sphereRadius);
        z += d;

        // Per-step FFT — map step index to frequency band (BAND_SAMPLES pattern)
        float t0 = float(i) / stepCount;
        float t1 = float(i + 1) / stepCount;
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        for (int bs = 0; bs < 4; bs++) {
            float bin = mix(binLo, binHi, (float(bs) + 0.5) / 4.0);
            if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
        energy = pow(clamp(energy / 4.0 * gain, 0.0, 1.0), curve);
        float fftBrightness = baseBright + energy;

        // Gradient LUT coloring with 1/d glow attenuation
        // Replaces reference: (cos(i*.1+t+vec4(6,1,2,0))+1.)/d
        vec3 stepColor = textureLod(gradientLUT, vec2(fract(z * colorStretch + colorPhase), 0.5), 0.0).rgb;
        color += stepColor * fftBrightness / max(d, 0.001);
    }

    // Brightness and tanh tonemap — reference: tanh(O/1e3)
    color = tanh(color * brightness / 1000.0);

    // === Trail buffer (identical to Muons) ===
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec3 raw = texelFetch(previousFrame, coord, 0).rgb;
    vec3 blurred  = 0.25   * raw;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2(-1, 0), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 1, 0), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 0,-1), 0).rgb;
    blurred += 0.125  * texelFetch(previousFrame, coord + ivec2( 0, 1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2(-1,-1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2( 1,-1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2(-1, 1), 0).rgb;
    blurred += 0.0625 * texelFetch(previousFrame, coord + ivec2( 1, 1), 0).rgb;
    vec3 prev = mix(raw, blurred, trailBlur);
    if (any(isnan(prev))) prev = vec3(0.0);
    finalColor = vec4(max(color, prev * decayFactor), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| marchSteps | int | 4-200 | 60 | Yes | March Steps |
| turbulenceOctaves | int | 2-12 | 4 | Yes | Octaves |
| turbulenceGrowth | float | 1.2-3.0 | 2.0 | Yes | Growth |
| sphereRadius | float | 0.5-10.0 | 3.0 | Yes | Radius |
| ringThickness | float | 0.01-0.5 | 0.1 | Yes | Thickness |
| cameraDistance | float | 3.0-20.0 | 9.0 | Yes | Camera Dist |
| phaseX | float | -PI_F to PI_F | 0.0 | Yes | Phase X |
| phaseY | float | -PI_F to PI_F | 2.0 | Yes | Phase Y |
| phaseZ | float | -PI_F to PI_F | 4.0 | Yes | Phase Z |
| outlineSpread | float | 0.0-0.5 | 0.1 | Yes | Outline Spread |
| decayHalfLife | float | 0.1-10.0 | 2.0 | Yes | Decay |
| trailBlur | float | 0.0-1.0 | 1.0 | Yes | Trail Blur |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| colorSpeed | float | 0.0-2.0 | 0.5 | Yes | Color Speed |
| colorStretch | float | 0.1-5.0 | 1.0 | Yes | Color Stretch |
| brightness | float | 0.1-5.0 | 1.0 | Yes | Brightness |
| blendMode | enum | - | SCREEN | No | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_SHELL_BLEND`
- Display name: `"Shell"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR_FULL`)
- Section: `11` (Filament)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-set by `REGISTER_GENERATOR_FULL`)

### Shader Uniforms

The `phase` config fields (`phaseX`, `phaseY`, `phaseZ`) are packed into a single `uniform vec3 phase` in the shader, following the Muons pattern. The Setup function packs them:

```cpp
float phase[3] = {cfg->phaseX, cfg->phaseY, cfg->phaseZ};
SetShaderValue(e->shader, e->phaseLoc, phase, SHADER_UNIFORM_VEC3);
```

The `colorPhase` accumulator is computed CPU-side: `e->colorPhase += cfg->colorSpeed * deltaTime`. The accumulated value is sent as a uniform (not `colorSpeed`). Same pattern as Muons.

The `decayFactor` is computed CPU-side: `exp(-0.693147f * deltaTime / cfg->decayHalfLife)`. Sent as a uniform.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Shell header

**Files**: `src/effects/shell.h` (create)
**Creates**: `ShellConfig`, `ShellEffect` types, `SHELL_CONFIG_FIELDS` macro, function declarations

**Do**: Create the header following the Design section's Types layout exactly. Use `muons.h` as the structural template:
- Include guards `SHELL_H`
- Includes: `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`
- Top comment: `// Shell effect module` / `// Raymarched hollow sphere with outline contours from per-step view rotation`
- `ShellConfig` struct with all fields and defaults from Design section
- `SHELL_CONFIG_FIELDS` macro from Design section
- Forward-declare `ColorLUT`: `typedef struct ColorLUT ColorLUT;`
- `ShellEffect` struct from Design section
- Function declarations from Design section (same signatures as Muons with Shell types)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Shell implementation

**Files**: `src/effects/shell.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation following `muons.cpp` as the structural template. All lifecycle functions mirror Muons with Shell-specific uniforms:

**Includes** (same set as Muons):
- Own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `render/render_utils.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<math.h>`, `<stddef.h>`

**Static helpers**: `InitPingPong()` and `UnloadPingPong()` — identical to Muons.

**Init** (`ShellEffectInit`): Load `shaders/shell.fs`, cache all uniform locations (from Design), allocate `ColorLUT`, init HDR ping-pong via `RenderUtilsInitTextureHDR`, clear both buffers, init `readIdx=0`, `time=0`, `colorPhase=0`. Return false with cleanup cascade on shader load failure.

**Setup** (`ShellEffectSetup`):
- Accumulate: `e->time += deltaTime`, `e->colorPhase += cfg->colorSpeed * deltaTime`
- Update ColorLUT
- Compute `decayFactor = expf(-0.693147f * deltaTime / cfg->decayHalfLife)`
- Pack phase: `float phase[3] = {cfg->phaseX, cfg->phaseY, cfg->phaseZ}; SetShaderValue(..., SHADER_UNIFORM_VEC3)`
- Bind all other uniforms individually
- Store FFT texture: `e->currentFFTTexture = fftTexture`

**Render** (`ShellEffectRender`): Ping-pong swap, render to write buffer binding `previousFrame` + `gradientLUT` + `fftTexture`, fullscreen quad, swap readIdx. Identical flow to Muons.

**Resize** (`ShellEffectResize`): Unload old ping-pong, init new, reset readIdx. Identical to Muons.

**Uninit** (`ShellEffectUninit`): Unload shader, uninit ColorLUT, unload ping-pong.

**RegisterParams** (`ShellRegisterParams`): Register all float config fields with `ModEngineRegisterParam("shell.<field>", ...)`. Use ranges from Design section. Register `marchSteps` and `turbulenceOctaves` as int params. Phase fields use `ROTATION_OFFSET_MAX` bounds.

**ConfigDefault**: `return ShellConfig{};`

**Three bridge functions** (non-static):
- `SetupShell(PostEffect *pe)` — calls `ShellEffectSetup` with `pe->shell`, `pe->effects.shell`, `pe->currentDeltaTime`, `pe->fftTexture`
- `SetupShellBlend(PostEffect *pe)` — calls `BlendCompositorApply` with `pe->shell.pingPong[pe->shell.readIdx].texture`, `pe->effects.shell.blendIntensity`, `pe->effects.shell.blendMode`
- `RenderShell(PostEffect *pe)` — calls `ShellEffectRender` with screen dimensions

**UI section** (`// === UI ===`):
- `static void DrawShellParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`:
  - Geometry section: `ModulatableSliderInt` for marchSteps (4-200), turbulenceOctaves (2-12); `ModulatableSlider` for turbulenceGrowth, sphereRadius, ringThickness, cameraDistance; `ModulatableSliderAngleDeg` for phaseX/Y/Z (ROTATION_OFFSET_MAX); `ModulatableSlider` for outlineSpread
  - Trail section: `ModulatableSlider` for decayHalfLife, trailBlur
  - Audio section: Standard audio controls (baseFreq, maxFreq, gain, curve, baseBright) — follow conventions exactly
  - Color section: `ModulatableSlider` for colorSpeed, colorStretch, brightness

**Registration** (bottom of file):
```cpp
STANDARD_GENERATOR_OUTPUT(shell)

// clang-format off
REGISTER_GENERATOR_FULL(TRANSFORM_SHELL_BLEND, Shell, shell,
                        "Shell", SetupShellBlend, SetupShell, RenderShell,
                        11, DrawShellParams, DrawOutput_shell)
// clang-format on
```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Shell shader

**Files**: `shaders/shell.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Create the fragment shader. Implement the Algorithm section from the Design.

**Attribution header** (before `#version`):
```glsl
// Based on "Speak [470]" by Xor
// https://www.shadertoy.com/view/33tSzl
// License: CC BY-NC-SA 3.0
// Modified: ported to GLSL 330; parameterized march steps, turbulence,
// sphere radius, camera distance, outline spread; replaced cos() coloring
// with gradient LUT; replaced audio sphere displacement with per-step FFT
// glow modulation; added trail buffer with decay/blur.
```

**Uniforms**: `resolution`, `time`, `marchSteps`, `turbulenceOctaves`, `turbulenceGrowth`, `sphereRadius`, `ringThickness`, `cameraDistance`, `phase` (vec3), `outlineSpread`, `colorStretch`, `colorPhase`, `brightness`, `gradientLUT`, `previousFrame`, `decayFactor`, `trailBlur`, `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`

**Main function**: Copy the Algorithm section from Design verbatim. The full GLSL is provided there — raymarching loop, FFT band sampling, gradient LUT coloring, tanh tonemap, and trail buffer with 3x3 blur kernel.

**Verify**: File is valid GLSL 330 with no syntax errors (build will validate at runtime).

---

#### Task 2.3: Effect config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/shell.h"` in the include section (after `muons.h`)
2. Add `TRANSFORM_SHELL_BLEND,` to `TransformEffectType` enum (after `TRANSFORM_FARADAY_BLEND`, before `TRANSFORM_EFFECT_COUNT`)
3. Add `ShellConfig shell;` to `EffectConfig` struct (after `GalaxyConfig galaxy;`)

The `TransformOrderConfig::order` array auto-populates from the enum — no manual edit needed.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/shell.h"` in the include section (after `muons.h`)
2. Add `ShellEffect shell;` to `PostEffect` struct (after `GalaxyEffect galaxy;`)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/shell.cpp` to `EFFECTS_SOURCES` (after `rainbow_road.cpp`).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/shell.h"` in the include section (after `muons.h`)
2. Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShellConfig, SHELL_CONFIG_FIELDS)` (after the `WoodblockConfig` line)
3. Add `X(shell)` to the `EFFECT_CONFIG_FIELDS` X-macro table (after `X(rainbowRoad)`)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Shell appears in Filament generator section of transform UI
- [ ] Enabling Shell shows the raymarched hollow sphere
- [ ] `outlineSpread = 0` renders solid shape; `outlineSpread = 0.1+` renders wireframe outlines
- [ ] Turbulence distorts the sphere into organic flowing forms
- [ ] Trail buffer produces smooth decay/blur of previous frames
- [ ] FFT reactivity modulates per-step glow brightness
- [ ] Gradient LUT colors the contours
- [ ] Blend compositor composites Shell output onto the scene
- [ ] Preset save/load preserves all Shell settings
- [ ] All parameters route to modulation engine
