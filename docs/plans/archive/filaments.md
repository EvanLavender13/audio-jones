# Filaments

Radial burst of luminous line segments where each filament represents one semitone. Active notes flare bright while quiet semitones persist as dim embers. Triangle-wave fractal noise displaces the UV field so filaments writhe organically along their length. The whole structure rotates slowly. Additive inverse-distance glow accumulates across all filaments, producing a cohesive radial shape with individually traceable strands. Generator blend effect in the generators category, composited via BlendCompositor like spectral arcs and muons.

**Research**: `docs/research/filaments.md`

> **Implementation divergence**: The plan's geometry model (radial burst with `innerRadius`/`outerRadius` endpoints at fixed angular positions) was wrong. The implementation uses nimitz's cumulative rotation pattern instead: `p1` rotates per-step via `stepAngle`, `p2` fans from `p1` via `spread`. Config fields `radius`, `spread`, `stepAngle` replace `innerRadius`/`outerRadius`. The `falloffExponent` uniform uses `1.0 / falloffExponent` as the exponent so higher values correctly mean faster dropoff (plan's formula was inverted for sub-1 distances). The plan's triangle-noise "UV displacement" description was also wrong — noise produces a scalar that scales `p`, not a direct coordinate displacement. The `segm()` function uses clean IQ `sdSegment` distance without the nimitz FFT warp (which caused audio-driven jitter on the line geometry). The `brightness` uniform from the plan was dropped; `tanh` rolloff uses no multiplier.

---

## Design

### Types

**FilamentsConfig** (in `src/effects/filaments.h`):

```
struct FilamentsConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 220.0f;   // Lowest visible frequency in Hz (A3)
  int numOctaves = 5;         // Octave count (x12 = total filaments)
  float gain = 5.0f;          // FFT magnitude amplifier
  float curve = 1.5f;         // Contrast exponent on magnitude

  // Filament geometry
  float innerRadius = 0.05f;  // Start distance from center (0.01-0.3)
  float outerRadius = 0.8f;   // End distance from center (0.3-1.5)

  // Glow
  float glowIntensity = 0.008f;  // Peak filament brightness (0.001-0.05)
  float falloffExponent = 1.2f;  // Distance falloff sharpness (0.8-2.0)
  float baseBright = 0.05f;      // Dim ember level for quiet semitones (0.0-0.5)

  // Triangle-noise displacement
  float noiseStrength = 0.4f;    // Displacement magnitude (0.0-1.0)
  float noiseSpeed = 4.5f;       // Noise rotation rate (0.0-10.0)

  // Animation
  float rotationSpeed = 0.3f;    // Radial spin rate (rad/s)

  // Tonemap
  float brightness = 1.0f;       // HDR brightness before tanh rolloff (0.1-5.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**FilamentsEffect** (in `src/effects/filaments.h`):

```
typedef struct FilamentsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum;       // CPU-accumulated rotation angle
  float noiseTime;           // CPU-accumulated noise animation phase
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int innerRadiusLoc;
  int outerRadiusLoc;
  int glowIntensityLoc;
  int falloffExponentLoc;
  int baseBrightLoc;
  int noiseStrengthLoc;
  int noiseTimeLoc;
  int rotationAccumLoc;
  int brightnessLoc;
  int gradientLUTLoc;
} FilamentsEffect;
```

### Algorithm

The shader runs a single loop over `numOctaves * 12` filaments (max 96 at 8 octaves).

**Triangle-wave noise displacement** (applied once per pixel before the filament loop):
- `tri(x) = abs(fract(x) - 0.5)` — triangle wave, range [0, 0.5]
- `tri2(p) = vec2(tri(p.x + tri(p.y*2)), tri(p.y + tri(p.x*2)))` — 2D coupled triangle waves
- 4-octave fBM: each iteration rotates displacement by `noiseTime`, scales UV by ~1.2x via a fixed rotation matrix, accumulates `tri2(p) / z` where `z` grows by lacunarity ~1.7x
- Final displacement scaled by `noiseStrength`, offsets pixel coordinate before segment evaluation
- See research doc "Triangle-Wave Noise Displacement" section and nimitz reference for exact formulation

**Per-filament evaluation** (inside loop):
1. Compute angular position: `angle = (float(i) / float(totalFilaments)) * TAU`
2. Compute endpoints rotated by `rotationAccum`: inner endpoint at `innerRadius`, outer at `outerRadius`
3. IQ `sdSegment` distance: standard point-to-segment distance function (see research doc)
4. FFT semitone lookup: same pattern as spectral arcs — `freq = baseFreq * pow(2.0, float(i) / 12.0)`, normalize to bin, sample fftTexture, apply gain/curve
5. Glow accumulation: `glow = glowIntensity / (pow(dist, falloffExponent) + epsilon)`
6. Color from gradient LUT by pitch class: `pitchClass = fract(float(i) / 12.0)`
7. Accumulate: `result += filamentColor * glow * (baseBright + mag)`

**HDR rolloff**: `tanh(result * brightness)` — soft clamp preventing blowout from additive accumulation.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 20.0-880.0 | 220.0 | Yes | Base Freq (Hz) |
| numOctaves | int | 1-8 | 5 | No | Octaves |
| gain | float | 1.0-20.0 | 5.0 | Yes | Gain |
| curve | float | 0.5-4.0 | 1.5 | Yes | Contrast |
| innerRadius | float | 0.01-0.3 | 0.05 | Yes | Inner Radius |
| outerRadius | float | 0.3-1.5 | 0.8 | Yes | Outer Radius |
| glowIntensity | float | 0.001-0.05 | 0.008 | Yes | Glow Intensity |
| falloffExponent | float | 0.8-2.0 | 1.2 | Yes | Falloff |
| baseBright | float | 0.0-0.5 | 0.05 | Yes | Base Bright |
| noiseStrength | float | 0.0-1.0 | 0.4 | Yes | Noise Strength |
| noiseSpeed | float | 0.0-10.0 | 4.5 | Yes | Noise Speed |
| rotationSpeed | float | -3.14-3.14 | 0.3 | Yes | Rotation Speed |
| brightness | float | 0.1-5.0 | 1.0 | Yes | Brightness |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |
| blendMode | enum | — | SCREEN | No | Blend Mode |

### Constants

- Enum name: `TRANSFORM_FILAMENTS_BLEND`
- Display name: `"Filaments Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` (badge `"GEN"`, section 10)
- Config member name: `filaments`
- Effect member name: `filaments`
- Shader file: `shaders/filaments.fs`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Effect Module Header and Source

**Files**: `src/effects/filaments.h` (create), `src/effects/filaments.cpp` (create)

**Creates**: `FilamentsConfig` struct, `FilamentsEffect` struct, Init/Setup/Uninit/ConfigDefault/RegisterParams functions

**Do**:
- Header: Define `FilamentsConfig` and `FilamentsEffect` structs per Design section above. Include `raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`. Forward-declare `ColorLUT`. Declare 5 public functions: `FilamentsEffectInit`, `FilamentsEffectSetup`, `FilamentsEffectUninit`, `FilamentsConfigDefault`, `FilamentsRegisterParams`.
- `FilamentsEffectSetup` signature takes `(FilamentsEffect* e, const FilamentsConfig* cfg, float deltaTime, Texture2D fftTexture)` — same pattern as `SpectralArcsEffectSetup`.
- `FilamentsEffectInit` signature takes `(FilamentsEffect* e, const FilamentsConfig* cfg)` — same as spectral arcs (needs cfg for ColorLUT init).
- Source: Follow `spectral_arcs.cpp` structure exactly. Init loads shader, caches all uniform locations, creates ColorLUT. Setup accumulates `rotationAccum += rotationSpeed * deltaTime` and `noiseTime += noiseSpeed * deltaTime`, updates LUT, binds all uniforms. Uninit unloads shader and frees LUT.
- RegisterParams: Register all modulatable fields from parameter table. Use `ROTATION_SPEED_MAX` for `rotationSpeed` bounds.

**Verify**: `cmake.exe --build build` compiles (after Wave 2 adds to CMakeLists).

---

### Wave 2: Parallel Integration

All tasks in this wave modify different files — no file overlap.

#### Task 2.1: Fragment Shader

**Files**: `shaders/filaments.fs` (create)
**Depends on**: Wave 1 complete (needs uniform names from header)

**Do**:
- Create fragment shader implementing the full algorithm from Design section.
- Uniforms: `resolution`, `fftTexture`, `gradientLUT`, `sampleRate`, `baseFreq`, `numOctaves`, `gain`, `curve`, `innerRadius`, `outerRadius`, `glowIntensity`, `falloffExponent`, `baseBright`, `noiseStrength`, `noiseTime`, `rotationAccum`, `brightness`.
- Implement `tri()` and `tri2()` helper functions for triangle-wave noise.
- 4-octave triangle-noise fBM displaces UV before filament loop. Reference nimitz's Shadertoy and research doc for the exact rotation matrix and lacunarity constants.
- Filament loop: `for (int i = 0; i < numOctaves * 12; i++)`. Inside: compute angle, rotated endpoints, sdSegment distance, FFT lookup, glow accumulation, gradient LUT color by pitch class.
- Final: `tanh(result * brightness)` HDR rolloff, output with alpha 1.0.
- Use `0.0001` as epsilon in glow denominator.

**Verify**: Shader compiles at runtime (build succeeds, effect loads without error).

#### Task 2.2: Effect Registration (effect_config.h)

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/filaments.h"` in alphabetical position among effect includes.
- Add `TRANSFORM_FILAMENTS_BLEND` in `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`, after `TRANSFORM_MUONS_BLEND`.
- Add `"Filaments Blend"` in `TRANSFORM_EFFECT_NAMES` array at matching position.
- Add `FilamentsConfig filaments;` member to `EffectConfig` struct with comment `// Filaments (radial semitone burst with triangle-noise displacement)`.
- Add `IsTransformEnabled` case: `case TRANSFORM_FILAMENTS_BLEND: return e->filaments.enabled && e->filaments.blendIntensity > 0.0f;`

**Verify**: Compiles.

#### Task 2.3: PostEffect Integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `post_effect.h`: Add `#include "effects/filaments.h"`. Add `FilamentsEffect filaments;` member after `MuonsEffect muons;`. Add `bool filamentsBlendActive;` after `bool muonsBlendActive;`.
- `post_effect.cpp`: In `PostEffectInit`, add init call after muons init: `if (!FilamentsEffectInit(&pe->filaments, &pe->effects.filaments)) { ... }` with error log and cleanup. In `PostEffectUninit`, add `FilamentsEffectUninit(&pe->filaments);`. In `PostEffectRegisterParams`, add `FilamentsRegisterParams(&pe->effects.filaments);`.

**Verify**: Compiles.

#### Task 2.4: Shader Setup (Generators)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `shader_setup_generators.h`: Add `void SetupFilaments(PostEffect *pe);` and `void SetupFilamentsBlend(PostEffect *pe);` declarations.
- `shader_setup_generators.cpp`: Add `#include "effects/filaments.h"`. Implement `SetupFilaments` delegating to `FilamentsEffectSetup(&pe->filaments, &pe->effects.filaments, pe->currentDeltaTime, pe->fftTexture)`. Implement `SetupFilamentsBlend` calling `BlendCompositorApply` with `pe->effects.filaments.blendIntensity` and `pe->effects.filaments.blendMode`. Add `case TRANSFORM_FILAMENTS_BLEND:` in `GetGeneratorScratchPass` returning `{pe->filaments.shader, SetupFilaments}`.
- `shader_setup.cpp`: Add `case TRANSFORM_FILAMENTS_BLEND:` in `GetTransformEffect` returning `{&pe->blendCompositor->shader, SetupFilamentsBlend, &pe->filamentsBlendActive}`.

**Verify**: Compiles.

#### Task 2.5: Render Pipeline

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `pe->filamentsBlendActive = IsTransformEnabled(&pe->effects, TRANSFORM_FILAMENTS_BLEND);` in the active-flag computation block, after the muons line.
- Add `type == TRANSFORM_FILAMENTS_BLEND` to `IsGeneratorBlendEffect`.

**Verify**: Compiles.

#### Task 2.6: UI Panel

**Files**: `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/filaments.h"` to includes.
- Add `static bool sectionFilaments = false;` with other section states.
- Create `DrawGeneratorsFilaments` function. Follow `DrawGeneratorsSpectralArcs` as template:
  - Checkbox with MoveTransformToEnd on enable.
  - FFT section: `Octaves` (SliderInt), `Base Freq (Hz)`, `Gain`, `Contrast` (ModulatableSlider).
  - Geometry section: `Inner Radius`, `Outer Radius` (ModulatableSlider).
  - Glow section: `Glow Intensity`, `Falloff`, `Base Bright` (ModulatableSlider).
  - Noise section: `Noise Strength`, `Noise Speed` (ModulatableSlider).
  - Animation section: `Rotation Speed` (ModulatableSliderAngleDeg with `"%.1f /s"` format).
  - Tonemap section: `Brightness` (ModulatableSlider).
  - Color section: `ImGuiDrawColorMode(&cfg->gradient)`.
  - Output section: `Blend Intensity` (ModulatableSlider), `Blend Mode` (Combo).
- Add call in `DrawGeneratorsCategory`, after spectral arcs and before muons: `ImGui::Spacing(); DrawGeneratorsFilaments(e, modSources, Theme::GetSectionGlow(sectionIndex++));`

**Verify**: Compiles.

#### Task 2.7: Category Badge

**Files**: `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `case TRANSFORM_FILAMENTS_BLEND:` in `GetTransformCategory` under the Generators section (section 10), alongside the other generator blend effects.

**Verify**: Compiles.

#### Task 2.8: Preset Serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FilamentsConfig, enabled, baseFreq, numOctaves, gain, curve, innerRadius, outerRadius, glowIntensity, falloffExponent, baseBright, noiseStrength, noiseSpeed, rotationSpeed, brightness, gradient, blendMode, blendIntensity)` macro with other config macros.
- Add `to_json` entry: `if (e.filaments.enabled) { j["filaments"] = e.filaments; }`
- Add `from_json` entry: `e.filaments = j.value("filaments", e.filaments);`

**Verify**: Compiles.

#### Task 2.9: Build System

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `src/effects/filaments.cpp` to `EFFECTS_SOURCES` list, in alphabetical position.

**Verify**: `cmake.exe --build build` succeeds with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline as "Filaments Blend"
- [ ] Effect shows "GEN" category badge (not "???")
- [ ] Enabling effect adds it to pipeline list
- [ ] UI controls grouped into FFT / Geometry / Glow / Noise / Animation / Tonemap / Color / Output sections
- [ ] Filaments visually match research description: radial burst, organic writhing, dim embers, bright active notes
- [ ] Gradient LUT colors by pitch class (same semitone across octaves shares color)
- [ ] Rotation accumulates smoothly over time
- [ ] Triangle-noise displacement creates organic writhing motion
- [ ] tanh rolloff prevents HDR blowout
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
