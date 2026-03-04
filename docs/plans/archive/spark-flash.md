# Spark Flash

Glowing four-arm crosshair sparks spawn at random positions, flashing bright as their arms extend then retracting and fading in a sine-shaped lifecycle. Multiple sparks overlap with staggered timing, creating a field of twinkling neon crosses with bright star points at their centers. FFT energy drives per-spark brightness; gradient LUT provides per-spark color.

**Research**: `docs/research/spark-flash.md`

## Design

### Types

**SparkFlashConfig** (`src/effects/spark_flash.h`):
```
bool enabled = false

// Geometry
int layers = 7                       // Concurrent sparks (2-16)
float lifetime = 0.2f                // Spark lifecycle duration in seconds (0.05-2.0)
float armThickness = 0.00002f        // Cross arm glow epsilon (0.00001-0.001)
float starSize = 0.00005f            // Center star epsilon (0.00001-0.001)
float armBrightness = 0.00035f       // Arm glow intensity scalar (0.0001-0.002)
float starBrightness = 0.00018f      // Star point intensity scalar (0.00005-0.001)
float armReach = 1.0f                // Arm extension distance multiplier (0.1-2.0)

// Audio
float baseFreq = 55.0f              // (27.5-440.0)
float maxFreq = 14000.0f            // (1000-16000)
float gain = 2.0f                   // (0.1-10.0)
float curve = 1.0f                  // (0.1-3.0)
float baseBright = 0.1f             // (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**SparkFlashEffect** (`src/effects/spark_flash.h`):
```
Shader shader
ColorLUT *gradientLUT
float time               // master time accumulator
int resolutionLoc
int fftTextureLoc
int sampleRateLoc
int timeLoc
int layersLoc
int lifetimeLoc
int armThicknessLoc
int starSizeLoc
int armBrightnessLoc
int starBrightnessLoc
int armReachLoc
int baseFreqLoc
int maxFreqLoc
int gainLoc
int curveLoc
int baseBrightLoc
int gradientLUTLoc
```

### Algorithm

The shader is a near-verbatim port of the reference with uniforms replacing constants and BAND_SAMPLES FFT replacing single-bin lookup.

```glsl
// Based on "Extreme_spark" by Sheena
// https://www.shadertoy.com/view/7fs3Rr
// License: CC BY-NC-SA 3.0 Unported
// Modified: uniforms replace constants, BAND_SAMPLES FFT, gradientLUT color
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float time;
uniform int layers;
uniform float lifetime;
uniform float armThickness;
uniform float starSize;
uniform float armBrightness;
uniform float starBrightness;
uniform float armReach;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform sampler2D gradientLUT;

#define PI 3.14159265359

float hash12(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    vec3 col = vec3(0.0);

    for (int i = 0; i < layers; i++) {
        float fi = float(i);
        float offset = fi * lifetime / float(layers);
        float phase = time + offset;

        float seed = floor(phase / lifetime);
        float lt = fract(phase / lifetime);

        // Random spark position
        float px = hash12(vec2(seed, fi));
        float py = hash12(vec2(seed, fi + 50.0));

        // Sine lifecycle for arm length and brightness base
        float armLen = sin(lt * PI) * armReach;
        float bright = sin(lt * PI);

        // FFT energy per spark — BAND_SAMPLES multi-sample averaging
        float t0 = float(i) / float(max(layers - 1, 1));
        float t1 = float(i + 1) / float(max(layers, 1));
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

        // Smoothstep fade envelope on arm length
        float vFade = smoothstep(armLen, armLen * 0.55, abs(uv.y - py));
        float hFade = smoothstep(armLen, armLen * 0.55, abs(uv.x - px));

        // Inverse-square glow for arms
        float vGlow = 1.0 / (pow(abs(uv.x - px), 2.0) + armThickness);
        float hGlow = 1.0 / (pow(abs(uv.y - py), 2.0) + armThickness);

        // Inverse-distance star point
        vec2 d = uv - vec2(px, py);
        d.x *= aspect;
        float star = 1.0 / (dot(d, d) + starSize);

        // Gradient LUT color per spark
        float colorT = float(i) / float(max(layers - 1, 1));
        vec3 sparkColor = texture(gradientLUT, vec2(colorT, 0.5)).rgb;
        vec3 starColor = clamp(sparkColor * 1.4 + vec3(0.25), 0.0, 2.0);

        col += vGlow * vFade * sparkColor * brightness * armBrightness;
        col += hGlow * hFade * sparkColor * brightness * armBrightness;
        col += star * starColor * brightness * starBrightness;
    }

    // Reinhard tonemapping
    col = col / (col + 1.0);
    finalColor = vec4(col, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| layers | int | 2-16 | 7 | No | Layers |
| lifetime | float | 0.05-2.0 | 0.2 | Yes | Lifetime |
| armThickness | float | 0.00001-0.001 | 0.00002 | Yes | Arm Thickness |
| starSize | float | 0.00001-0.001 | 0.00005 | Yes | Star Size |
| armBrightness | float | 0.0001-0.002 | 0.00035 | Yes | Arm Brightness |
| starBrightness | float | 0.00005-0.001 | 0.00018 | Yes | Star Brightness |
| armReach | float | 0.1-2.0 | 1.0 | Yes | Arm Reach |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.0 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.1 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_SPARK_FLASH_BLEND`
- Display name: `"Spark Flash"`
- Category badge: `"GEN"` (auto via `REGISTER_GENERATOR`)
- Section index: 13 (Atmosphere)
- Field name: `sparkFlash`
- Macro: `REGISTER_GENERATOR`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create SparkFlash config and effect header

**Files**: `src/effects/spark_flash.h` (create)
**Creates**: `SparkFlashConfig`, `SparkFlashEffect` types, `SPARK_FLASH_CONFIG_FIELDS` macro, function declarations

**Do**: Create header following nebula.h as pattern. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Define `SparkFlashConfig` struct with all fields from Design section. Define `SPARK_FLASH_CONFIG_FIELDS` macro listing all serializable fields. Define `SparkFlashEffect` typedef struct with all `*Loc` ints and `time` accumulator. Declare `SparkFlashEffectInit(SparkFlashEffect*, const SparkFlashConfig*)`, `SparkFlashEffectSetup(SparkFlashEffect*, const SparkFlashConfig*, float, Texture2D)`, `SparkFlashEffectUninit(SparkFlashEffect*)`, `SparkFlashConfigDefault(void)`, `SparkFlashRegisterParams(SparkFlashConfig*)`. Top comment: `// Spark Flash — twinkling four-arm crosshair sparks with sine lifecycle and FFT-reactive brightness`.

**Verify**: `cmake.exe --build build` compiles (header not yet included anywhere).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create SparkFlash shader

**Files**: `shaders/spark_flash.fs` (create)

**Do**: Implement the Algorithm section verbatim. The full GLSL is provided in the Design — copy it exactly. Attribution comment block at top before `#version` line.

**Verify**: File exists, has attribution header, `#version 330`, all uniforms match the Design section's `SparkFlashEffect` loc list.

---

#### Task 2.2: Create SparkFlash effect module

**Files**: `src/effects/spark_flash.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow nebula.cpp as pattern. Includes: `"spark_flash.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `<stddef.h>`.

**Init**: Load `"shaders/spark_flash.fs"`, cache all uniform locations, init `ColorLUT` from config gradient, set `time = 0.0f`. On shader fail return false. On LUT fail, unload shader then return false.

**Setup**: Accumulate `time += deltaTime` (no speed multiplier — lifetime controls pacing). Bind all uniforms. `layers` as `SHADER_UNIFORM_INT`. All others as `SHADER_UNIFORM_FLOAT` or `SHADER_UNIFORM_VEC2`. Bind `fftTexture` and `gradientLUT` textures. Call `ColorLUTUpdate`.

**Uninit**: Unload shader, `ColorLUTUninit`.

**ConfigDefault**: Return `SparkFlashConfig{}`.

**RegisterParams**: Register all float params from the Parameters table. Use `"sparkFlash.<field>"` IDs. Also register `blendIntensity`. Do NOT register `layers` (true int, not modulatable).

**UI section** (`DrawSparkFlashParams`):
- `ImGui::SeparatorText("Geometry")` — `ImGui::SliderInt("Layers##sparkFlash", &cfg->layers, 2, 16)`, then modulatable sliders for lifetime, armThickness (log, `"%.5f"`), starSize (log, `"%.5f"`), armBrightness (log, `"%.5f"`), starBrightness (log, `"%.5f"`), armReach (`"%.2f"`)
- `ImGui::SeparatorText("Audio")` — standard order: Base Freq, Max Freq, Gain, Contrast, Base Bright. Use `ModulatableSlider` with formats from conventions (`baseFreq "%.1f"`, `maxFreq "%.0f"`, `gain "%.1f"`, `curve "%.2f"`, `baseBright "%.2f"`)

**Bridge functions**: `SetupSparkFlash(PostEffect* pe)` — calls `SparkFlashEffectSetup` with `pe->sparkFlash`, `pe->effects.sparkFlash`, `pe->currentDeltaTime`, `pe->fftTexture`. `SetupSparkFlashBlend(PostEffect* pe)` — calls `BlendCompositorApply` with `pe->blendCompositor`, `pe->generatorScratch.texture`, blend intensity, blend mode.

**Registration**: `STANDARD_GENERATOR_OUTPUT(sparkFlash)` then `REGISTER_GENERATOR(TRANSFORM_SPARK_FLASH_BLEND, SparkFlash, sparkFlash, "Spark Flash", SetupSparkFlashBlend, SetupSparkFlash, 13, DrawSparkFlashParams, DrawOutput_sparkFlash)`. Wrap in `// clang-format off` / `// clang-format on`.

**Verify**: Compiles with Wave 3 integration files.

---

#### Task 2.3: Integrate into config, post_effect, build, and serialization

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)

**Do**:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/spark_flash.h"` with other effect includes
   - Add `TRANSFORM_SPARK_FLASH_BLEND,` before `TRANSFORM_EFFECT_COUNT` in the enum
   - Add `SparkFlashConfig sparkFlash;` in the `EffectConfig` struct near other atmosphere generators (near `nebula`, `fireworks`)

2. **`src/render/post_effect.h`**:
   - Add `#include "effects/spark_flash.h"` with other effect includes
   - Add `SparkFlashEffect sparkFlash;` in the `PostEffect` struct near `nebula`

3. **`CMakeLists.txt`**:
   - Add `src/effects/spark_flash.cpp` to `EFFECTS_SOURCES`

4. **`src/config/effect_serialization.cpp`**:
   - Add `#include "effects/spark_flash.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SparkFlashConfig, SPARK_FLASH_CONFIG_FIELDS)` with other JSON macros
   - Add `X(sparkFlash)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Atmosphere section of effects panel
- [ ] Crosshair sparks render with staggered timing
- [ ] FFT energy modulates per-spark brightness
- [ ] Gradient LUT colors each spark differently
- [ ] Layers slider changes spark count
- [ ] Lifetime slider changes flash duration
- [ ] Preset save/load round-trips all fields
- [ ] Modulation routes to registered parameters

---

## Implementation Notes

- Renamed `armThickness` → `armSoftness`, `starSize` → `starSoftness` — original names implied the parameter made arms thicker / stars larger, but the inverse-square epsilon actually controls glow diffusion
- Kept original `1/(x² + eps)` glow formulas from the Shadertoy reference; shader maps clean UI ranges (0.1-10.0) to tiny epsilons internally via `* 0.0001`
- Same `* 0.0001` conversion applied to `armBrightness` and `starBrightness` so all four params use 0.1-10.0 UI ranges
- Defaults calibrated to match original Shadertoy constants: `armSoftness=0.2` → eps 0.00002, `starSoftness=0.5` → eps 0.00005, `armBrightness=3.5` → 0.00035, `starBrightness=1.8` → 0.00018
