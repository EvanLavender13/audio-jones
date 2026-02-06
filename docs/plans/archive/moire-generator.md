# Moire Generator

Procedural moire pattern generator that overlays 2-4 high-frequency gratings with independent frequency, angle, speed, warp, scale, and phase per layer. Multiplicative superposition of nearly-matched gratings produces visible low-frequency beat fringes. Three base pattern shapes (stripes, circles, grid), sharp/smooth toggle, and gradient LUT color remapping.

**Research**: `docs/research/moire-generator.md`

## Design

### Types

**MoireLayerConfig** (nested struct, one per layer):

```
struct MoireLayerConfig {
  float frequency = 20.0f;       // Grating line density (1.0-100.0)
  float angle = 0.0f;            // Static rotation offset in radians
  float rotationSpeed = 0.0f;    // Continuous rotation rate in radians/second
  float warpAmount = 0.0f;       // Sinusoidal UV distortion amplitude (0.0-0.5)
  float scale = 1.0f;            // Zoom level (0.5-4.0)
  float phase = 0.0f;            // Wave phase offset in radians
};
```

**MoireGeneratorConfig** (top-level effect config):

```
struct MoireGeneratorConfig {
  bool enabled = false;

  // Global
  int patternMode = 0;           // 0=Stripes, 1=Circles, 2=Grid
  int layerCount = 3;            // Active layers (2-4)
  bool sharpMode = false;        // Square-wave vs sinusoidal gratings
  float colorIntensity = 0.0f;   // Blend grayscale <-> LUT color (0.0-1.0)
  float globalBrightness = 1.0f; // Overall output brightness (0.0-2.0)

  // Per-layer (4 named members, staggered defaults)
  MoireLayerConfig layer0;       // freq=20, angle=0
  MoireLayerConfig layer1;       // freq=22, angle=5deg
  MoireLayerConfig layer2;       // freq=24, angle=10deg
  MoireLayerConfig layer3;       // freq=26, angle=15deg

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

Staggered defaults for layers 1-3:
- `layer1`: frequency=22.0f, angle=0.0873f (5 deg)
- `layer2`: frequency=24.0f, angle=0.1745f (10 deg)
- `layer3`: frequency=26.0f, angle=0.2618f (15 deg)

**MoireGeneratorEffect** (runtime state):

```
typedef struct MoireGeneratorEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float layerAngles[4];          // Per-layer rotation accumulators
  float time;                    // Global time accumulator for warp animation

  // Uniform locations — global
  int resolutionLoc;
  int patternModeLoc;
  int layerCountLoc;
  int sharpModeLoc;
  int colorIntensityLoc;
  int globalBrightnessLoc;
  int timeLoc;
  int gradientLUTLoc;

  // Uniform locations — per-layer (4 each)
  int frequencyLoc[4];
  int angleLoc[4];
  int warpAmountLoc[4];
  int scaleLoc[4];
  int phaseLoc[4];
} MoireGeneratorEffect;
```

Note: Uniform location arrays in the Effect struct are internal — never serialized. Only the config uses named layer members. The `Init` function builds location names like `"layer0.frequency"`, `"layer1.frequency"`, etc.

### Algorithm

The shader implements three grating types selected by `patternMode`. Each layer transforms its UV, evaluates the grating, and the results multiply together.

**Grating functions** (in shader):
- Stripes smooth: `0.5 + 0.5 * cos(2*PI * coord * freq + phase)`
- Stripes sharp: `step(0.5, fract(coord * freq + phase / (2*PI)))`
- Circles: same as stripes but `coord = length(uv - 0.5)`
- Grid: product of two orthogonal stripe gratings

**Per-layer UV transform** (in shader, order matters):
1. Scale from center: `uv = (uv - 0.5) * scale + 0.5`
2. Rotate by accumulated angle: `uv = rotate2d(angle) * (uv - 0.5) + 0.5`
3. Warp: `uv.x += warpAmount * sin(uv.y * 5.0 + time * 2.0 + phase)` — the `time` term animates the warp distortion continuously

**Combination**: All active layers multiply, then `pow(result, 1.0/layerCount)` normalizes brightness.

**Color**: Monochrome intensity indexes the gradient LUT. `colorIntensity` blends between grayscale and LUT output.

The CPU-side Setup function accumulates `layerAngles[i] += rotationSpeed * deltaTime` and `time += deltaTime` each frame. Passes the total angle (static offset + accumulated rotation) per layer and the global `time` to the shader.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| patternMode | int | 0-2 | 0 | No | Pattern |
| layerCount | int | 2-4 | 3 | No | Layers |
| sharpMode | bool | on/off | off | No | Sharp |
| colorIntensity | float | 0.0-1.0 | 0.0 | Yes | Color Mix |
| globalBrightness | float | 0.0-2.0 | 1.0 | Yes | Brightness |
| layer{N}.frequency | float | 1.0-100.0 | 20/22/24/26 | Yes | Frequency |
| layer{N}.angle | float | 0-2PI rad | 0/5/10/15 deg | Yes | Angle |
| layer{N}.rotationSpeed | float | -PI..PI rad/s | 0.0 | Yes | Rotation Speed |
| layer{N}.warpAmount | float | 0.0-0.5 | 0.0 | Yes | Warp |
| layer{N}.scale | float | 0.5-4.0 | 1.0 | Yes | Scale |
| layer{N}.phase | float | 0-2PI rad | 0.0 | Yes | Phase |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_MOIRE_GENERATOR_BLEND`
- Display name: `"Moire Generator Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` (same group as Constellation, Plasma, Interference)
- Config member: `moireGenerator` in `EffectConfig`
- Effect member: `moireGenerator` in `PostEffect`

---

## Tasks

### Wave 1: Header and Shader

#### Task 1.1: Create effect header

**Files**: `src/effects/moire_generator.h` (create)
**Creates**: `MoireLayerConfig`, `MoireGeneratorConfig`, `MoireGeneratorEffect` types and function declarations

**Do**: Follow `plasma.h` structure. Define `MoireLayerConfig` struct with per-layer fields. Define `MoireGeneratorConfig` with global params + 4 named `MoireLayerConfig` members + `ColorConfig` + blend fields. Define `MoireGeneratorEffect` typedef struct with shader, ColorLUT pointer, `layerAngles[4]`, and all uniform location ints (use `int frequencyLoc[4]` etc for the per-layer locations — arrays are fine in the Effect struct since it's never serialized). Declare `Init`, `Setup`, `Uninit`, `ConfigDefault`, `RegisterParams`. The `ConfigDefault` function returns staggered per-layer defaults.

**Verify**: Header compiles when included.

#### Task 1.2: Create fragment shader

**Files**: `shaders/moire_generator.fs` (create)
**Creates**: The visual effect

**Do**: Follow the algorithm section above. Uniforms: `resolution` (vec2), `patternMode` (int), `layerCount` (int), `sharpMode` (int), `colorIntensity` (float), `globalBrightness` (float), `time` (float — for warp animation), `gradientLUT` (sampler2D), plus per-layer: `layer0.frequency`, `layer0.angle`, `layer0.warpAmount`, `layer0.scale`, `layer0.phase` (and same for layer1-3). Use dot-separated uniform names so `GetShaderLocation(shader, "layer0.frequency")` works. Implement `rotate2d()`, grating functions for each pattern mode, per-layer UV transform, multiplicative combination with pow normalization, and colorIntensity blend between grayscale and LUT.

**Verify**: Shader file is valid GLSL 330.

---

### Wave 2: Implementation and Integration

#### Task 2.1: Create effect implementation

**Files**: `src/effects/moire_generator.cpp` (create)
**Depends on**: Wave 1 (needs header and shader)

**Do**: Follow `plasma.cpp` pattern. `Init`: load shader, cache all uniform locations (loop i=0..3 to build location name strings like `"layer0.frequency"`), init ColorLUT, zero out `layerAngles` and `time`. `Setup`: accumulate `layerAngles[i] += layer.rotationSpeed * deltaTime` for each layer and `time += deltaTime`, update ColorLUT, set all uniforms including `time`. Pass each layer's total angle as `layer.angle + layerAngles[i]`. `Uninit`: unload shader, free ColorLUT. `RegisterParams`: register all modulatable float params (colorIntensity, globalBrightness, blendIntensity, and each layer's frequency/angle/rotationSpeed/warpAmount/scale/phase using `"moireGenerator.layer0.frequency"` naming).

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Effect registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 (needs header)

**Do**: Add `#include "effects/moire_generator.h"`. Add `TRANSFORM_MOIRE_GENERATOR_BLEND` enum entry before `TRANSFORM_EFFECT_COUNT` (place it after the existing generator blend entries near the end). Add name `"Moire Generator Blend"` in `TRANSFORM_EFFECT_NAMES`. Add `MoireGeneratorConfig moireGenerator;` member to `EffectConfig` (near the other generator configs). Add `IsTransformEnabled` case: `return e->moireGenerator.enabled && e->moireGenerator.blendIntensity > 0.0f;`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 (needs header)

**Do**: In `post_effect.h`: add `#include "effects/moire_generator.h"`, add `MoireGeneratorEffect moireGenerator;` member, add `bool moireGeneratorBlendActive;` flag. In `post_effect.cpp`: add `MoireGeneratorEffectInit` call (with `&pe->effects.moireGenerator` for ColorLUT init) in `PostEffectInit` near other generator inits, add `MoireGeneratorEffectUninit` in `PostEffectUninit`, add `MoireGeneratorRegisterParams(&pe->effects.moireGenerator)` in `PostEffectRegisterParams` under the generator section.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader setup (generators category)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 (needs header)

**Do**: In `shader_setup_generators.h`: declare `SetupMoireGenerator(PostEffect *pe)` and `SetupMoireGeneratorBlend(PostEffect *pe)`. In `shader_setup_generators.cpp`: add `#include "effects/moire_generator.h"`, implement `SetupMoireGenerator` (delegates to `MoireGeneratorEffectSetup`), implement `SetupMoireGeneratorBlend` (calls `BlendCompositorApply` with `moireGenerator.blendIntensity` and `moireGenerator.blendMode`). Add case to `GetGeneratorScratchPass` for `TRANSFORM_MOIRE_GENERATOR_BLEND`. In `shader_setup.cpp`: add `TRANSFORM_MOIRE_GENERATOR_BLEND` case to `GetTransformEffect` returning `{&pe->blendCompositor->shader, SetupMoireGeneratorBlend, &pe->moireGeneratorBlendActive}`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Task 2.2, 2.4

**Do**: Add `pe->moireGeneratorBlendActive = (pe->effects.moireGenerator.enabled && pe->effects.moireGenerator.blendIntensity > 0.0f);` alongside other generator blend active flags. Add `type == TRANSFORM_MOIRE_GENERATOR_BLEND` to `IsGeneratorBlendEffect()`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_generators.cpp` (modify), `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Task 2.2

**Do**: In `imgui_effects_generators.cpp`: add `#include "effects/moire_generator.h"`, add `static bool sectionMoireGenerator = false;`, implement `DrawGeneratorsMoireGenerator` helper. The UI should show: Enabled checkbox (with `MoveTransformToEnd` for `TRANSFORM_MOIRE_GENERATOR_BLEND`), Pattern combo (Stripes/Circles/Grid), Layers slider (2-4), Sharp checkbox, then per-layer sections (collapsible or inline for each active layer using a for-like pattern with layer0/layer1/layer2/layer3 pointers) with `ModulatableSlider` for frequency, `ModulatableSliderAngleDeg` for angle, rotationSpeed (`/s` suffix), `ModulatableSlider` for warp/scale/phase, then color section with `ImGuiDrawColorMode` + `ModulatableSlider` for Color Mix, global Brightness slider, blend intensity + blend mode. Call from `DrawGeneratorsCategory`. In `imgui_effects.cpp`: add `TRANSFORM_MOIRE_GENERATOR_BLEND` to `GetTransformCategory` under the generators group.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Task 2.2

**Do**: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireLayerConfig, frequency, angle, rotationSpeed, warpAmount, scale, phase)` and `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireGeneratorConfig, enabled, patternMode, layerCount, sharpMode, colorIntensity, globalBrightness, layer0, layer1, layer2, layer3, gradient, blendMode, blendIntensity)`. Add `to_json`: `if (e.moireGenerator.enabled) { j["moireGenerator"] = e.moireGenerator; }`. Add `from_json`: `e.moireGenerator = j.value("moireGenerator", e.moireGenerator);`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.8: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Task 2.1

**Do**: Add `src/effects/moire_generator.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline as "Moire Generator Blend"
- [ ] Effect shows generators category badge (not "???")
- [ ] Enabling effect shows visible moire pattern immediately (staggered defaults)
- [ ] Pattern mode switches between stripes, circles, grid
- [ ] Sharp toggle changes grating waveform
- [ ] Per-layer frequency/angle sliders produce dramatic fringe changes
- [ ] Rotation speed creates animated fringe drift
- [ ] Color gradient applies via LUT with colorIntensity blend
- [ ] Preset save/load preserves all settings including per-layer params
- [ ] Modulation routes to all registered parameters
