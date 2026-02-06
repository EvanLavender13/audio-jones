# Pitch Spiral

Generates an Archimedean spiral where position encodes musical pitch via 12-TET mapping. Each full turn equals one octave — musically related notes (octaves, fifths) align geometrically. Samples the current FFT frame to display harmonic structure as radial spoke patterns.

**Research**: `docs/research/pitch-spiral.md`

## Design

### Types

**PitchSpiralConfig** (in `src/effects/pitch_spiral.h`):

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| enabled | bool | false | Effect toggle |
| baseFreq | float | 220.0f | Lowest visible frequency (Hz) — A3 |
| numTurns | int | 8 | Number of spiral rings (octaves visible) |
| spiralSpacing | float | 0.05f | Distance between adjacent rings |
| lineWidth | float | 0.02f | Spiral line thickness |
| blur | float | 0.02f | Anti-aliasing softness (smoothstep width) |
| curve | float | 2.0f | Magnitude contrast exponent — `pow(mag, curve)` |
| gradient | ColorConfig | {.mode = COLOR_MODE_GRADIENT} | Pitch-class coloring via LUT |
| blendMode | EffectBlendMode | EFFECT_BLEND_SCREEN | Blend compositing mode |
| blendIntensity | float | 1.0f | Blend strength |

**PitchSpiralEffect** (runtime state in `src/effects/pitch_spiral.h`):

| Field | Type | Purpose |
|-------|------|---------|
| shader | Shader | Fragment shader handle |
| gradientLUT | ColorLUT* | Pitch-class color lookup texture |
| resolutionLoc | int | `resolution` uniform location |
| fftTextureLoc | int | `fftTexture` uniform location |
| sampleRateLoc | int | `sampleRate` uniform location |
| baseFreqLoc | int | `baseFreq` uniform location |
| numTurnsLoc | int | `numTurns` uniform location |
| spiralSpacingLoc | int | `spiralSpacing` uniform location |
| lineWidthLoc | int | `lineWidth` uniform location |
| blurLoc | int | `blur` uniform location |
| curveLoc | int | `curve` uniform location |
| gradientLUTLoc | int | `gradientLUT` uniform location |

### Algorithm

The shader implements three steps from the research doc:

1. **Spiral geometry**: Convert UV to Archimedean spiral coordinates. Center at (0.5, 0.5), aspect-correct x. Compute angle via `atan()`, radius via `length()`. Derive `offset = radius + (angle/TAU) * spiralSpacing` and `whichTurn = floor(offset / spiralSpacing)`.

2. **Pitch mapping**: Convert spiral position to FFT bin. `cents = (whichTurn - angle/TAU) * 1200.0`. Frequency = `baseFreq * pow(TET_ROOT, cents/100.0)` where TET_ROOT = 2^(1/12) = 1.05946309436. Bin = `freq / (sampleRate * 0.5)`. Clamp: bins beyond 1.0 render black.

3. **Drawing**: Sample `fftTexture` at computed bin. Apply contrast via `pow(magnitude, curve)`. Compute spiral line mask using `smoothstep` on `mod(offset, spiralSpacing)`. Color via `gradientLUT` indexed by pitch class = `fract(cents / 1200.0)`. Final = `noteColor * magnitude * lineAlpha`.

See research doc for complete GLSL. The shader reads `fftTexture` (1025x1, normalized magnitudes) and `gradientLUT` (256x1, user gradient).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 55.0-440.0 | 220.0 | Yes | Base Freq (Hz) |
| numTurns | int | 4-12 | 8 | No | Octaves |
| spiralSpacing | float | 0.03-0.1 | 0.05 | Yes | Ring Spacing |
| lineWidth | float | 0.01-0.04 | 0.02 | Yes | Line Width |
| blur | float | 0.01-0.03 | 0.02 | Yes | AA Softness |
| curve | float | 0.5-4.0 | 2.0 | Yes | Contrast |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_PITCH_SPIRAL_BLEND`
- Display name: `"Pitch Spiral Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` (same as constellation, plasma, scan bars)
- Config member: `pitchSpiral` on EffectConfig
- Effect member: `pitchSpiral` on PostEffect
- Blend active flag: `pitchSpiralBlendActive` on PostEffect

---

## Tasks

### Wave 1: Effect Module

#### Task 1.1: Create pitch_spiral effect module

**Files**: `src/effects/pitch_spiral.h` (create), `src/effects/pitch_spiral.cpp` (create)

**Creates**: PitchSpiralConfig struct, PitchSpiralEffect struct, Init/Setup/Uninit/ConfigDefault/RegisterParams functions

**Do**:
- Follow `src/effects/scan_bars.h` / `src/effects/scan_bars.cpp` as the structural pattern
- Config and Effect structs match the Design section above
- `PitchSpiralEffectInit`: Load shader `shaders/pitch_spiral.fs`, get all uniform locations, init ColorLUT from `cfg->gradient`
- `PitchSpiralEffectSetup`: Takes `(PitchSpiralEffect*, const PitchSpiralConfig*, float deltaTime, Texture2D fftTexture)` — note the extra fftTexture param, same as `FftRadialWarpEffectSetup` pattern. Bind all uniforms including fftTexture via `SetShaderValueTexture`, update ColorLUT, pass `AUDIO_SAMPLE_RATE` as float uniform (include `audio/audio.h` for the define)
- `PitchSpiralEffectUninit`: Unload shader and ColorLUT
- `PitchSpiralRegisterParams`: Register all modulatable params per the Parameters table

**Verify**: `cmake.exe --build build` compiles (after Wave 2 adds to CMakeLists).

---

### Wave 2: All Integration (parallel — no file overlap)

#### Task 2.1: Fragment shader

**Files**: `shaders/pitch_spiral.fs` (create)
**Depends on**: Wave 1 complete

**Do**:
- Implement the three-step algorithm from the Design section
- Uniforms: `resolution` (vec2), `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `numTurns` (int), `spiralSpacing` (float), `lineWidth` (float), `blur` (float), `curve` (float), `gradientLUT` (sampler2D)
- Define constants: `TAU = 6.28318530718`, `TET_ROOT = 1.05946309436` (2^(1/12))
- Clamp: if `bin > 1.0`, magnitude = 0 (renders black beyond Nyquist)
- Output to `finalColor` (no `texture0` input — this is a generator)

**Verify**: Shader syntax correct (compiles at runtime).

#### Task 2.2: Effect registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/pitch_spiral.h"` with other effect headers
- Add `TRANSFORM_PITCH_SPIRAL_BLEND` enum entry before `TRANSFORM_EFFECT_COUNT`
- Add name string `"Pitch Spiral Blend"` in `TransformEffectName` array
- Add `PitchSpiralConfig pitchSpiral;` in EffectConfig struct (after scanBars)
- Add `IsTransformEnabled` case: `return e->pitchSpiral.enabled && e->pitchSpiral.blendIntensity > 0.0f;`
- The order array auto-fills from the enum sequential initialization in `TransformOrderConfig()` constructor

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `post_effect.h`: Add `#include "effects/pitch_spiral.h"`, add `PitchSpiralEffect pitchSpiral;` member (after scanBars), add `bool pitchSpiralBlendActive;` flag (after scanBarsBlendActive)
- `post_effect.cpp` Init: Add `PitchSpiralEffectInit(&pe->pitchSpiral, &pe->effects.pitchSpiral)` with error handling (same pattern as scanBars)
- `post_effect.cpp` Uninit: Add `PitchSpiralEffectUninit(&pe->pitchSpiral)` (before generatorScratch unload)
- `post_effect.cpp` RegisterParams: Add `PitchSpiralRegisterParams(&pe->effects.pitchSpiral)` (after ScanBarsRegisterParams)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader setup (generators category)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `shader_setup_generators.h`: Declare `void SetupPitchSpiral(PostEffect *pe);` and `void SetupPitchSpiralBlend(PostEffect *pe);`
- `shader_setup_generators.cpp`: Implement `SetupPitchSpiral` — delegates to `PitchSpiralEffectSetup(&pe->pitchSpiral, &pe->effects.pitchSpiral, pe->currentDeltaTime, pe->fftTexture)`. Implement `SetupPitchSpiralBlend` — calls `BlendCompositorApply` with `pe->generatorScratch.texture`, same pattern as other generator blends
- `shader_setup.cpp`: Add `TRANSFORM_PITCH_SPIRAL_BLEND` case in `GetTransformEffect()` returning `{&pe->blendCompositor->shader, SetupPitchSpiralBlend, &pe->pitchSpiralBlendActive}`. Add `TRANSFORM_PITCH_SPIRAL_BLEND` case in `GetGeneratorScratchPass()` returning `{pe->pitchSpiral.shader, SetupPitchSpiral}`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `TRANSFORM_PITCH_SPIRAL_BLEND` to `IsGeneratorBlendEffect()` return expression (after `TRANSFORM_SCAN_BARS_BLEND`)
- Add `pe->pitchSpiralBlendActive = (pe->effects.pitchSpiral.enabled && pe->effects.pitchSpiral.blendIntensity > 0.0f);` in the generator blend active flags section (after scanBarsBlendActive)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `imgui_effects.cpp`: Add `case TRANSFORM_PITCH_SPIRAL_BLEND:` to `GetTransformCategory()` in the generators group (after `TRANSFORM_SCAN_BARS_BLEND`)
- `imgui_effects_generators.cpp`:
  - Add `#include "effects/pitch_spiral.h"` at top
  - Add `static bool sectionPitchSpiral = false;`
  - Add `DrawGeneratorsPitchSpiral` function following scan_bars pattern:
    - Section title: `"Pitch Spiral"`
    - Checkbox with MoveTransformToEnd for `TRANSFORM_PITCH_SPIRAL_BLEND`
    - `ModulatableSlider` for baseFreq, spiralSpacing, lineWidth, blur, curve
    - `ImGui::SliderInt` for numTurns (not modulatable)
    - `ImGuiDrawColorMode` for gradient
    - Output section: blend intensity + blend mode combo
  - Call `DrawGeneratorsPitchSpiral` from `DrawGeneratorsCategory` (after ScanBars, before SolidColor)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PitchSpiralConfig, enabled, baseFreq, numTurns, spiralSpacing, lineWidth, blur, curve, gradient, blendMode, blendIntensity)` with other config macros
- Add `if (e.pitchSpiral.enabled) { j["pitchSpiral"] = e.pitchSpiral; }` in `to_json`
- Add `e.pitchSpiral = j.value("pitchSpiral", e.pitchSpiral);` in `from_json`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.8: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `src/effects/pitch_spiral.cpp` to `EFFECTS_SOURCES` (after scan_bars.cpp)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline as "Pitch Spiral Blend"
- [ ] Effect shows generators category badge (not "???")
- [ ] Enabling effect renders spiral geometry to generatorScratch
- [ ] FFT data drives spiral brightness (play audio to verify)
- [ ] Gradient LUT colors spiral by pitch class
- [ ] Bins beyond Nyquist render black (outer rings with high numTurns)
- [ ] All modulatable params respond to LFO routing
- [ ] Preset save/load preserves all settings
- [ ] Blend mode and intensity work correctly
