# Prism Shatter

Procedural generator that ray-casts through a 3D sinusoidal color field with nonlinear cross-product displacement, producing dense crystalline geometry of sharp neon edges and dark faceted voids. Camera drifts through infinite crystal space while the structure animates independently. FFT maps hue to frequency so different crystal facets react to different audio bands.

**Research**: `docs/research/prism-shatter.md`

## Design

### Types

**PrismShatterConfig** (in `src/effects/prism_shatter.h`):

```
bool enabled = false

// Geometry
float displacementScale = 4.0f    // Cross-product displacement magnitude (1.0-10.0)
float stepSize = 2.0f             // Ray march step distance (0.5-5.0)
int iterations = 128              // Ray march steps (64-256)

// Camera
float cameraSpeed = 0.125f        // Camera orbit speed (-1.0 to 1.0)
float orbitRadius = 16.0f         // Camera distance from origin (4.0-32.0)
float fov = 1.5f                  // Focal length / field of view (0.5-3.0)

// Structure
float structureSpeed = 0.0f       // Crystal phase animation speed (-1.0 to 1.0)

// Rendering
float brightness = 1.0f           // Overall brightness multiplier (0.5-3.0)
float saturationPower = 2.0f      // Edge-to-void contrast exponent (1.0-4.0)

// Audio
float baseFreq = 55.0f            // Lowest FFT frequency mapped to hue (27.5-440.0)
float maxFreq = 14000.0f          // Highest FFT frequency mapped to hue (1000-16000)
float gain = 2.0f                 // FFT brightness amplification (0.1-10.0)
float curve = 1.5f                // FFT response curve exponent (0.1-3.0)
float baseBright = 0.3f           // Minimum brightness without audio (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**PrismShatterEffect** (in `src/effects/prism_shatter.h`):

```
Shader shader
ColorLUT *gradientLUT
float cameraTime       // CPU-accumulated camera orbit phase
float structureTime    // CPU-accumulated structure animation phase
int resolutionLoc, fftTextureLoc, sampleRateLoc
int cameraTimeLoc, structureTimeLoc
int displacementScaleLoc, stepSizeLoc, iterationsLoc
int orbitRadiusLoc, fovLoc
int brightnessLoc, saturationPowerLoc
int baseFreqLoc, maxFreqLoc, gainLoc, curveLoc, baseBrightLoc
int gradientLUTLoc
```

Config fields macro: `PRISM_SHATTER_CONFIG_FIELDS` listing all serializable fields.

Init signature: `bool PrismShatterEffectInit(PrismShatterEffect *e, const PrismShatterConfig *cfg)` — takes config for ColorLUT init. Use `REGISTER_GENERATOR`.

### Algorithm

The shader is a volumetric ray caster. The complete GLSL for `shaders/prism_shatter.fs`:

**Uniforms:**
```glsl
uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float cameraTime;
uniform float structureTime;
uniform float displacementScale;
uniform float stepSize;
uniform int iterations;
uniform float orbitRadius;
uniform float fov;
uniform float brightness;
uniform float saturationPower;
uniform float baseFreq, maxFreq, gain, curve, baseBright;
uniform sampler2D gradientLUT;
```

**Color field** — maps 3D position to RGB via sine, with phase offset for structure animation:
```glsl
vec3 value(vec3 pos, float phase) {
    return vec3(sin(pos.x + phase), sin(pos.y + phase), sin(pos.z + phase));
}
```

**Ray march** — accumulate color along ray with cross-product displacement and linear falloff:
```glsl
vec3 scan(vec3 pos, vec3 dir, float phase) {
    float itf = float(iterations);
    float str = 1.5 / sqrt(itf);
    vec3 c = vec3(0.5);
    for (int i = 0; i < iterations; i++) {
        float f = (1.0 - float(i) / itf) * str;

        vec3 posMod = value(pos, phase);
        vec3 newPos;
        newPos.x = posMod.y * posMod.z;
        newPos.y = posMod.x * posMod.z;
        newPos.z = posMod.x * posMod.y;

        c += value(pos + newPos * displacementScale, phase) * f;
        pos += dir * stepSize;
    }
    return c;
}
```

**Camera** — Lissajous orbit with irrational frequency ratios (never repeats):
```glsl
float a = cameraTime;
vec3 pos = vec3(cos(a / 4.0 + sin(a / 2.0)), cos(a * 0.2), sin(a * 0.31) / 4.5) * orbitRadius;
vec3 n = normalize(pos + vec3(cos(a * 2.3), cos(a * 2.61), cos(a * 1.62)));
vec3 crossRight = normalize(cross(n, vec3(0.0, 0.0, 1.0)));
vec3 crossUp = normalize(cross(n, crossRight));
n = n * fov + crossRight * uv.x + crossUp * uv.y;
```

**Coloring** — extract hue from raw accumulated RGB, sample gradient LUT, apply saturation-based brightness and FFT audio reactivity:
```glsl
vec3 raw = scan(pos, normalize(n), structureTime);

// Hue extraction from raw accumulation (before any processing)
float gray = (raw.r + raw.g + raw.b) / 3.0;
float sat = abs(raw.r - gray) + abs(raw.g - gray) + abs(raw.b - gray);
float hue = atan(raw.g - raw.b, raw.r - (raw.g + raw.b) * 0.5) / (2.0 * 3.14159265) + 0.5;
float bright = pow(sat, saturationPower);

// FFT: hue maps to frequency band
float freq = baseFreq * pow(maxFreq / baseFreq, hue);
float bin = freq / (sampleRate * 0.5);
float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
energy = pow(clamp(energy * gain, 0.0, 1.0), curve);
float audioBright = baseBright + energy;

// Final color
vec3 color = texture(gradientLUT, vec2(hue, 0.5)).rgb * bright * audioBright * brightness;
finalColor = vec4(color, 1.0);
```

**UV centering:**
```glsl
vec2 uv = (gl_FragCoord.xy - resolution * 0.5) / resolution.y;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| displacementScale | float | 1.0-10.0 | 4.0 | Yes | Displacement |
| stepSize | float | 0.5-5.0 | 2.0 | Yes | Step Size |
| iterations | int | 64-256 | 128 | No | Iterations |
| cameraSpeed | float | -1.0 to 1.0 | 0.125 | Yes | Camera Speed |
| orbitRadius | float | 4.0-32.0 | 16.0 | Yes | Orbit Radius |
| fov | float | 0.5-3.0 | 1.5 | Yes | FOV |
| structureSpeed | float | -1.0 to 1.0 | 0.0 | Yes | Structure Speed |
| brightness | float | 0.5-3.0 | 1.0 | Yes | Brightness |
| saturationPower | float | 1.0-4.0 | 2.0 | Yes | Edge Contrast |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.3 | Yes | Base Bright |

### Constants

- Enum: `TRANSFORM_PRISM_SHATTER_BLEND`
- Display name: `"Prism Shatter"`
- Category: `"GEN"`, section 12 (Texture)
- Flags: `EFFECT_FLAG_BLEND` (hardcoded by `REGISTER_GENERATOR`)
- Config field: `prismShatter`
- Effect field: `prismShatter`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/prism_shatter.h` (create)
**Creates**: `PrismShatterConfig`, `PrismShatterEffect` types, `PRISM_SHATTER_CONFIG_FIELDS` macro, function declarations

**Do**: Create header following motherboard.h pattern exactly. Declare `PrismShatterConfig` with all fields from Design > Types (including `ColorConfig gradient` and `EffectBlendMode blendMode`). Declare `PrismShatterEffect` with all uniform location ints. Declare Init (takes `Effect*, Config*`), Setup (takes `Effect*, Config*, deltaTime, fftTexture`), Uninit, ConfigDefault, RegisterParams. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`.

**Verify**: `cmake.exe --build build` compiles (header-only, included by later tasks).

---

### Wave 2: Parallel implementation

#### Task 2.1: Create fragment shader

**Files**: `shaders/prism_shatter.fs` (create)
**Depends on**: None (shader is standalone)

**Do**: Implement the full Algorithm section from this plan. Use `#version 330`, `in vec2 fragTexCoord` (unused — use `gl_FragCoord.xy` for UV centering per the algorithm). All uniforms from the Algorithm section. The `value()`, `scan()`, camera setup, coloring, and FFT sections are all specified verbatim in the Algorithm section above — implement them exactly. Output to `finalColor`.

**Verify**: File exists. No build step needed (runtime compiled).

---

#### Task 2.2: Create effect module

**Files**: `src/effects/prism_shatter.cpp` (create)
**Depends on**: Wave 1 (needs header types)

**Do**: Implement following motherboard.cpp as pattern:
- `PrismShatterEffectInit`: Load shader from `"shaders/prism_shatter.fs"`, cache all uniform locations, init `ColorLUT` from `cfg->gradient`. Init `cameraTime = 0.0f`, `structureTime = 0.0f`. Cleanup on failure (unload shader if LUT fails).
- `PrismShatterEffectSetup`: Accumulate `cameraTime += cfg->cameraSpeed * deltaTime`, `structureTime += cfg->structureSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms via `SetShaderValue`/`SetShaderValueTexture`. Bind `fftTexture`, `sampleRate` (from `AUDIO_SAMPLE_RATE`), `resolution`, `gradientLUT` texture.
- `PrismShatterEffectUninit`: Unload shader, `ColorLUTUninit`.
- `PrismShatterConfigDefault`: Return `PrismShatterConfig{}`.
- `PrismShatterRegisterParams`: Register all 13 modulatable params (8 effect + 5 FFT) with dot-separated IDs `"prismShatter.*"`. Include `blendIntensity`.
- `SetupPrismShatter(PostEffect*)`: Bridge to `PrismShatterEffectSetup`.
- `SetupPrismShatterBlend(PostEffect*)`: Call `BlendCompositorApply` with `generatorScratch.texture`, `blendIntensity`, `blendMode`.
- Colocated UI section (`// === UI ===`):
  - `DrawPrismShatterParams`: Audio section (standard 5 sliders), Geometry section (`SliderInt` for iterations, `ModulatableSlider` for displacementScale, stepSize), Camera section (cameraSpeed, orbitRadius, fov), Structure section (structureSpeed), Rendering section (brightness, saturationPower via `ModulatableSlider`).
  - `STANDARD_GENERATOR_OUTPUT(prismShatter)`
  - `REGISTER_GENERATOR(TRANSFORM_PRISM_SHATTER_BLEND, PrismShatter, prismShatter, "Prism Shatter", SetupPrismShatterBlend, SetupPrismShatter, 12, DrawPrismShatterParams, DrawOutput_prismShatter)`

Includes: Same as motherboard.cpp — own header, `audio/audio.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_config.h`, `config/effect_descriptor.h`, `imgui.h`, `render/blend_compositor.h`, `render/blend_mode.h`, `render/color_lut.h`, `render/post_effect.h`, `ui/imgui_panels.h`, `ui/modulatable_slider.h`, `ui/ui_units.h`, `<stddef.h>`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 (needs header)

**Do**:
1. Add `#include "effects/prism_shatter.h"` with the other effect includes.
2. Add `TRANSFORM_PRISM_SHATTER_BLEND,` before `TRANSFORM_EFFECT_COUNT` in the enum.
3. Add `PrismShatterConfig prismShatter;` to `EffectConfig` struct (near the other generator configs).

Note: `TransformOrderConfig::order` auto-populates from the enum count — no explicit array edit needed.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 (needs header)

**Do**:
1. Add `#include "effects/prism_shatter.h"` with the other effect includes.
2. Add `PrismShatterEffect prismShatter;` to `PostEffect` struct (near the other generator effect members).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: None

**Do**: Add `src/effects/prism_shatter.cpp` to `EFFECTS_SOURCES` list.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 (needs header for CONFIG_FIELDS macro)

**Do**:
1. Add `#include "effects/prism_shatter.h"` with the other effect includes.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PrismShatterConfig, PRISM_SHATTER_CONFIG_FIELDS)` with the other per-config macros.
3. Add `X(prismShatter) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Texture generators section of UI
- [ ] Effect shows "GEN" badge
- [ ] Enabling renders crystalline geometry with gradient coloring
- [ ] Camera orbits through crystal space
- [ ] Audio makes different facets pulse at different frequencies
- [ ] Preset save/load preserves all settings
- [ ] All 13+1 params appear in modulation routing list
