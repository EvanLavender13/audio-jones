# Nebula

Parallax nebula generator with two kaliset fractal gas layers at different depths and per-semitone star twinkling. Front and back gas layers breathe independently with broad-band audio energy. Stars flare individually when their mapped semitone activates, creating a living star field where constellations trace the melody against slowly drifting cosmic gas. Blend-only generator — composites onto the scene via BlendCompositor.

**Research**: `docs/research/nebula.md`

## Design

### Types

**NebulaConfig** (in `src/effects/nebula.h`):

```
struct NebulaConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;       // Lowest mapped pitch in Hz (27.5-440.0)
  int numOctaves = 5;            // Semitone range for star mapping (2-8)
  float gain = 5.0f;             // FFT sensitivity (1.0-20.0)
  float curve = 1.5f;            // Contrast exponent on FFT magnitudes (0.5-3.0)
  float baseBright = 0.15f;      // Star glow when semitone is silent (0.0-1.0)

  // Nebula layers
  float driftSpeed = 0.06f;      // Sinusoidal wander rate (0.01-0.2)
  float frontScale = 3.0f;       // UV divisor for foreground layer (1.0-8.0)
  float backScale = 5.0f;        // UV divisor for background layer (2.0-12.0)
  int frontIter = 13;            // Fractal iterations front (6-20)
  int backIter = 10;             // Fractal iterations back (6-20)

  // Stars
  float starDensity = 400.0f;    // Grid resolution for star placement (100.0-800.0)
  float starSharpness = 35.0f;   // Hash power exponent (10.0-60.0)

  // Output
  float brightness = 1.0f;       // Overall multiplier (0.5-3.0)
  float vignetteStrength = 6.0f; // Edge fade steepness (0.0-10.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**NebulaEffect** (runtime state):

```
typedef struct NebulaEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;                    // Master time accumulator for drift
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int driftSpeedLoc;
  int frontScaleLoc;
  int backScaleLoc;
  int frontIterLoc;
  int backIterLoc;
  int starDensityLoc;
  int starSharpnessLoc;
  int brightnessLoc;
  int vignetteStrengthLoc;
  int gradientLUTLoc;
} NebulaEffect;
```

### Algorithm

The shader implements the full algorithm from the research doc. Key points for the implementer:

**Kaliset field function**: `abs(p) / dot(p,p) + FOLD_OFFSET` iteration with exponential weight accumulation. Constants `FOLD_OFFSET`, `STRENGTH`, `DECAY`, `POWER`, `SCALE`, `THRESHOLD` are `#define` constants in the shader (values in research doc). Include the time-varying `STRENGTH` perturbation (`+ 0.03 * log(fract(sin(time)*4373))`) for temporal shimmer.

**Two-layer parallax**: Foreground at z ≈ 0.0, background at z ≈ 3.5. Each layer gets different UV scale (divided by `frontScale`/`backScale`), different drift multiplier (foreground 0.45, background 0.16), and different iteration count.

**Audio-reactive gas**: Foreground sums lower-octave semitone magnitudes, background sums upper-octave. Standard semitone-to-bin lookup via `baseFreq * pow(2.0, s/12.0) / (sampleRate * 0.5)`. The band sum gates each layer's brightness.

**Per-semitone stars**: Grid-cell hash (`floor(layerUV * starDensity)`) with `nrand3` hash function. Each cell maps to a semitone via `floor(rnd.z * numOctaves * 12)`. Star brightness = `pow(rnd.y, starSharpness) * (baseBright + mag * gain)`. Color from gradient LUT by pitch class: `fract(semitone / 12.0)`.

**Color composition**: Gas colors from field value cubed/squared, tinted by LUT at offset coordinates (0.2 front, 0.8 back). Edge vignette: `(1 - exp((|x|-1) * vignetteStrength)) * (1 - exp((|y|-1) * vignetteStrength))`. Final: `mix(backLayer, 1.0, v) * frontColor + backColor + starColor`.

**Star density normalization**: Divide star grid UV by `max(resolution.x, 600.0)` for consistent density across resolutions.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| numOctaves | int | 2-8 | 5 | No | Octaves |
| gain | float | 1.0-20.0 | 5.0 | Yes | Gain |
| curve | float | 0.5-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| driftSpeed | float | 0.01-0.2 | 0.06 | Yes | Drift Speed |
| frontScale | float | 1.0-8.0 | 3.0 | Yes | Front Scale |
| backScale | float | 2.0-12.0 | 5.0 | Yes | Back Scale |
| frontIter | int | 6-20 | 13 | No | Front Iterations |
| backIter | int | 6-20 | 10 | No | Back Iterations |
| starDensity | float | 100.0-800.0 | 400.0 | Yes | Star Density |
| starSharpness | float | 10.0-60.0 | 35.0 | Yes | Star Sharpness |
| brightness | float | 0.5-3.0 | 1.0 | Yes | Brightness |
| vignetteStrength | float | 0.0-10.0 | 6.0 | Yes | Vignette |
| gradient | ColorConfig | — | gradient mode | No | (color widget) |
| blendMode | EffectBlendMode | enum | EFFECT_BLEND_SCREEN | No | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | No | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_NEBULA_BLEND`
- Display name: `"Nebula Blend"`
- Category badge: `"GEN"`, section index 10
- Effect config member: `nebula`
- Blend active flag: `nebulaBlendActive`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create nebula effect header and source

**Files**: `src/effects/nebula.h` (create), `src/effects/nebula.cpp` (create)
**Creates**: `NebulaConfig` struct, `NebulaEffect` struct, lifecycle function declarations

**Do**: Follow `spectral_arcs.h` structural pattern. Header declares `NebulaConfig` and `NebulaEffect` as specified in Design section. Include `render/color_config.h`, `render/blend_mode.h`, `raylib.h`. Source implements `NebulaEffectInit` (load shader, cache all uniform locations, init ColorLUT from `cfg->gradient`), `NebulaEffectSetup` (accumulate `time += driftSpeed * deltaTime`, update ColorLUT, set all uniforms including `fftTexture` and `sampleRate`), `NebulaEffectUninit`, `NebulaConfigDefault`, `NebulaRegisterParams`. Setup signature takes `Texture2D fftTexture` like spectral_arcs. Register all 11 modulatable params listed in the parameters table (baseFreq, gain, curve, baseBright, driftSpeed, frontScale, backScale, starDensity, starSharpness, brightness, vignetteStrength).

**Verify**: `cmake.exe --build build` compiles (after Wave 2 adds CMakeLists entry).

---

### Wave 2: All Integration (parallel tasks, no file overlap)

#### Task 2.1: Effect registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/nebula.h"`. Add `TRANSFORM_NEBULA_BLEND` enum entry before `TRANSFORM_CRT`. Add name string `"Nebula Blend"` in the names array at matching position. Add `NebulaConfig nebula;` member to `EffectConfig`. Add `IsTransformEnabled` case: `return e->nebula.enabled && e->nebula.blendIntensity > 0.0f;`. Add `TRANSFORM_NEBULA_BLEND` to `TransformOrderConfig::order` array.

**Verify**: Compiles.

#### Task 2.2: Fragment shader

**Files**: `shaders/nebula.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the full kaliset nebula shader as described in the Algorithm section. Uniforms match every `*Loc` field in `NebulaEffect`. Use `#define` for kaliset constants (`FOLD_OFFSET`, `STRENGTH`, `DECAY`, `POWER`, `SCALE`, `THRESHOLD`). Two-layer parallax with separate drift rates. Audio-reactive gas brightness via semitone band sums from `fftTexture`. Per-semitone star overlay with grid-cell hash. Color from `gradientLUT` at offset coordinates. Edge vignette. All formulas from the research doc.

**Verify**: Shader syntax valid (checked at runtime on init).

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `post_effect.h`: add `#include "effects/nebula.h"`, add `NebulaEffect nebula;` member, add `bool nebulaBlendActive;` flag. In `post_effect.cpp`: add `NebulaEffectInit(&pe->nebula, &pe->effects.nebula)` in init section, add `NebulaEffectUninit(&pe->nebula)` in uninit, add `NebulaRegisterParams(&pe->effects.nebula)` in register params.

**Verify**: Compiles.

#### Task 2.4: Shader setup (generators)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `shader_setup_generators.h`: declare `SetupNebula(PostEffect *pe)` and `SetupNebulaBlend(PostEffect *pe)`. In `shader_setup_generators.cpp`: add `#include "effects/nebula.h"`, implement `SetupNebula` (delegates to `NebulaEffectSetup` with `pe->fftTexture`), implement `SetupNebulaBlend` (calls `BlendCompositorApply` with `pe->generatorScratch.texture`, nebula blend intensity, nebula blend mode). Add `TRANSFORM_NEBULA_BLEND` case to `GetGeneratorScratchPass` returning `{pe->nebula.shader, SetupNebula}`. In `shader_setup.cpp`: add `TRANSFORM_NEBULA_BLEND` dispatch case in `GetTransformEffect` returning `{&pe->blendCompositor->shader, SetupNebulaBlend, &pe->effects.nebula.enabled}`. Note: `enabled` pointer here works because `IsTransformEnabled` handles the `&& blendIntensity > 0` check separately.

**Verify**: Compiles.

#### Task 2.5: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `type == TRANSFORM_NEBULA_BLEND` to `IsGeneratorBlendEffect()` return expression. Add `pe->nebulaBlendActive = IsTransformEnabled(&pe->effects, TRANSFORM_NEBULA_BLEND);` in `RenderPipelineApplyOutput()` with the other blend active flag assignments.

**Verify**: Compiles.

#### Task 2.6: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/nebula.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` succeeds.

#### Task 2.7: UI panel

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: In `imgui_effects.cpp`: add `TRANSFORM_NEBULA_BLEND` case to `GetTransformCategory()` returning `{"GEN", 10}`. In `imgui_effects_generators.cpp`: add `static bool sectionNebula = false;`. Create `DrawGeneratorsNebula` helper following the spectral_arcs UI pattern: `DrawSectionBegin("Nebula", ...)`, checkbox enables `TRANSFORM_NEBULA_BLEND`, group params by section (`SeparatorText`):
- **FFT**: `SliderInt` for Octaves, `ModulatableSlider` for Base Freq, Gain, Contrast, Base Bright
- **Layers**: `ModulatableSlider` for Front Scale, Back Scale, `SliderInt` for Front Iterations, Back Iterations
- **Stars**: `ModulatableSlider` for Star Density, Star Sharpness
- **Animation**: `ModulatableSlider` for Drift Speed
- **Output**: `ModulatableSlider` for Brightness, Vignette; `ImGuiDrawColorMode` for gradient; `ModulatableSlider` for Blend Intensity; `Combo` for Blend Mode

Add `DrawGeneratorsNebula` call in `DrawGeneratorsCategory` before `DrawGeneratorsSolidColor`.

**Verify**: Compiles.

#### Task 2.8: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NebulaConfig, enabled, baseFreq, numOctaves, gain, curve, baseBright, driftSpeed, frontScale, backScale, frontIter, backIter, starDensity, starSharpness, brightness, vignetteStrength, gradient, blendMode, blendIntensity)`. Add `to_json` entry: `if (e.nebula.enabled) { j["nebula"] = e.nebula; }`. Add `from_json` entry: `e.nebula = j.value("nebula", e.nebula);`.

**Verify**: Compiles.

---

## Implementation Notes

Major deviations from the original plan, discovered during iterative tuning.

### Three-layer parallax (was two)

Added a mid layer between front and back for richer depth. New config fields `midScale` (default 3.0, range 1.0-10.0) and `midIter` (default 20, range 6-40). Layer positions: front z=0, mid z=2.5, back z=5. Each layer uses a different drift multiplier (front 0.3x, mid 0.2x, back 0.12x) and spatial offset to prevent layer overlap.

### No audio-reactive gas

The plan called for gas layers that "breathe" with broad-band FFT energy. In practice, any audio multiplier on the fractal gas blows out into solid color — the kaliset field values are already near saturation. Audio drives stars only (per-semitone magnitude gates individual star brightness). Gas renders as stable fractal structure colored by gradient LUT.

### Gas coloring: cubic/quadratic power curves

Front layer uses cubic falloff (`0.3 * t³ * LUT`) for sharp filament contrast. Mid and back layers use quadratic (`0.4 * tm² * LUT`, `0.5 * t2² * LUT`) for softer fill. The scalar multipliers (0.3/0.4/0.5) prevent front-layer blowout while keeping background layers visible.

### CBS field constants hardcoded

The kaliset is extremely sensitive to constant values — small deviations produce flat/invisible output. Only `FOLD_OFFSET` remains a `#define`. All other CBS constants are inline: `strength=7.0`, exponential decay weight `exp(-i/7.0)`, power `2.2`, scale `5.0`, threshold `-0.7`. The time-varying strength perturbation (`+ 0.03 * log(1.e-6 + fract(sin(time)*4373.11))`) uses the `1.e-6` guard against `log(0)` at `time=0`.

### Grid-cell star glow

Stars use a 3×3 neighbor grid-cell approach with gaussian glow falloff `exp(-d² * 10.0)` instead of the per-pixel CBS pattern (which produced invisible sub-pixel dots). Each grid cell hashes to a semitone via `floor(rnd.z * totalSemitones)`. Star brightness = `(baseBright + sMag * gain) * glow`. Stars render on all three layers using each layer's world-space position for parallax.

### CPU-accumulated drift time

`driftSpeed` controls how fast time accumulates on the CPU (`time += driftSpeed * deltaTime`). The shader receives `time` directly and never multiplies by `driftSpeed`. This prevents position jumps when the user adjusts the drift slider — changing the slider only affects the accumulation rate going forward, not the current position.

### Updated defaults

| Parameter | Plan | Implemented | Reason |
|-----------|------|-------------|--------|
| baseBright | 0.15 | 0.6 | Stars invisible at low values |
| driftSpeed | 0.06 | 1.0 | Wall-clock accumulation rate, not raw shader multiplier |
| frontScale | 3.0 | 4.0 | Zoomed in too much, lost filament detail |
| backScale | 5.0 | 4.0 | Balanced against new mid layer |
| frontIter | 13 | 26 | CBS uses 26; low values produce blobby output |
| backIter | 10 | 18 | CBS uses 18; matches filament sharpness |
| baseBright range | 0.0-1.0 | 0.0-2.0 | Wider range for star visibility tuning |
| driftSpeed range | 0.01-0.2 | 0.01-5.0 | CPU accumulation needs wider range |
| iteration max | 20 | 40 | Higher iterations yield finer fractal detail |

### Vignette removed

The vignette uniform still exists in the shader interface but the shader does not apply it. The edge fade obscured stars near screen edges and added no visual value to a blend-only generator.

### sampleRate sourced internally

`NebulaEffectSetup` uses `(float)AUDIO_SAMPLE_RATE` from `audio/audio.h` instead of receiving `sampleRate` as a parameter. `PostEffect` has no `sampleRate` member — this matches the `spectral_arcs` pattern.

### 12 modulatable params (was 11)

Added `midScale` to the modulatable param list. `midIter` is not modulatable (integer, changed rarely).

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in pipeline list with "GEN" badge when enabled
- [x] Effect can be reordered via drag-drop
- [x] UI shows all parameter sections (FFT, Layers, Stars, Animation, Output)
- [x] Enabling effect moves it to end of transform order
- [ ] Preset save/load preserves all nebula settings
- [x] Modulation routes to all 12 registered parameters
- [x] Kaliset fractal renders three distinct parallax layers
- [x] Stars twinkle per-semitone when audio plays
- [x] Gas layers render as stable fractal structure (no audio blowout)
