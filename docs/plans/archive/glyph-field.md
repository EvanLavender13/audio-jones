# Glyph Field

Layered planes of scrolling character grids where text dissolves into abstract visual texture. Multiple flat grids at different scales and speeds overlap with transparency, creating parallax depth. Characters lose legibility — becoming pattern, rhythm, and color fields that breathe, flicker, and break apart.

**Research**: `docs/research/glyph_field.md`
**Category**: Generators
**Pipeline**: Single fragment shader, standard generator blend pattern (scratch pass + BlendCompositor)

## Design

### Types

#### GlyphFieldConfig

```
enabled           bool              false
gridSize          float             24.0     // Character density — cells per screen height (8.0-64.0)
layerCount        int               2        // Overlapping grid planes (1-4)
layerScaleSpread  float             1.4      // Scale ratio between successive layers (0.5-2.0)
layerSpeedSpread  float             1.3      // Speed ratio between successive layers (0.5-2.0)
layerOpacity      float             0.6      // Opacity falloff per layer (0.1-1.0)
scrollDirection   int               0        // 0=Horizontal, 1=Vertical, 2=Radial
scrollSpeed       float             0.4      // Base scroll velocity (0.0-2.0)
rainAmount        float             1.0      // Row/column rain motion intensity (0.0-1.0)
flutterAmount     float             0.3      // Per-cell character cycling intensity (0.0-1.0)
flutterSpeed      float             2.0      // Character cycling rate (0.1-10.0)
waveAmplitude     float             0.05     // Sine distortion strength (0.0-0.5)
waveFreq          float             6.0      // Sine distortion spatial frequency (1.0-20.0)
waveSpeed         float             1.0      // Sine distortion animation speed (0.0-5.0)
driftAmount       float             0.0      // Per-cell position wander magnitude (0.0-0.5)
driftSpeed        float             0.5      // Position wander rate (0.1-5.0)
bandDistortion    float             0.3      // Step-based row height variation (0.0-1.0)
inversionRate     float             0.1      // Fraction of cells with inverted glyphs (0.0-1.0)
inversionSpeed    float             0.1      // Inversion state rotation speed (0.0-2.0)
lcdMode           bool              false    // LCD sub-pixel RGB stripe overlay
lcdFreq           float             800.0    // LCD stripe spatial frequency (100.0-2000.0)
gradient          ColorConfig       {mode=COLOR_MODE_GRADIENT}
blendMode         EffectBlendMode   EFFECT_BLEND_SCREEN
blendIntensity    float             1.0
```

#### GlyphFieldEffect

```
shader            Shader
fontAtlas         Texture2D         // Loaded from fonts/font_atlas.png
gradientLUT       ColorLUT*
time              float             // Animation accumulator
resolutionLoc     int
timeLoc           int
gridSizeLoc       int
layerCountLoc     int
layerScaleSpreadLoc  int
layerSpeedSpreadLoc  int
layerOpacityLoc   int
scrollDirectionLoc   int
scrollSpeedLoc    int
rainAmountLoc     int
flutterAmountLoc  int
flutterSpeedLoc   int
waveAmplitudeLoc  int
waveFreqLoc       int
waveSpeedLoc      int
driftAmountLoc    int
driftSpeedLoc     int
bandDistortionLoc int
inversionRateLoc  int
inversionSpeedLoc int
lcdModeLoc        int
lcdFreqLoc        int
fontAtlasLoc      int
gradientLUTLoc    int
```

### Algorithm

All logic lives in `shaders/glyph_field.fs`. The C++ side only accumulates time and binds uniforms.

**Font atlas sampling**: The existing `fonts/font_atlas.png` is 1024x1024 with a 16x16 grid (256 glyphs, 64px per cell). Sample by character index using normalized UV math from the research doc — `cellOrigin = vec2(idx % 16, idx / 16) / 16.0; charUV = cellOrigin + localUV / 16.0`.

**PCG3D hash**: Stateless 3D integer hash from research doc. Three multiply-XOR rounds produce per-cell random vectors for character index, color, speed offset, flutter phase, and inversion state.

**Grid layout**: For each layer, `cellCoord = floor(uv * gridSize)`, `localUV = fract(uv * gridSize)`. Each layer scales gridSize by `pow(layerScaleSpread, layerIndex)`.

**Scroll directions**: Three modes selected by `scrollDirection` uniform:
- Horizontal: per-row hash offsets `uv.x` by `time * (hash - 0.5) * scrollSpeed`
- Vertical: per-column hash offsets `uv.y`
- Radial: convert to polar `(theta, r)`, per-ring angular scroll, `fract()` wrapping at theta=0

**Motion modes** (all mixable via amplitude params):
- Rain: base scroll from scroll direction. `rainAmount` scales offset.
- Flutter: per-cell character index cycling via `floor(time * flutterSpeed + hash * 100)`.
- Wave: sine-based UV displacement before grid quantization.
- Drift: per-cell position offset from time-seeded hash.

**Band distortion**: Step-based UV scaling from research doc — rows near center render at normal size, rows toward edges compress.

**Inversion flicker**: Animated fraction of cells swap foreground/background via `fract(hash + time * inversionSpeed) < inversionRate`.

**Layer compositing**: Loop `layerCount` layers, additive blend with per-layer opacity falloff.

**Coloring**: Per-cell hash maps to gradient LUT position. Standard `ColorConfig` / `ColorLUT` integration.

**LCD sub-pixel mode**: `max(sin(pixelCoord.x * lcdFreq + vec3(0, 2, 4)) * sin(pixelCoord.y * lcdFreq), 0.0)` multiplied onto final color when `lcdMode` enabled.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| gridSize | float | 8.0-64.0 | 24.0 | yes | Grid Size |
| layerCount | int | 1-4 | 2 | no | Layers |
| layerScaleSpread | float | 0.5-2.0 | 1.4 | yes | Layer Scale |
| layerSpeedSpread | float | 0.5-2.0 | 1.3 | yes | Layer Speed |
| layerOpacity | float | 0.1-1.0 | 0.6 | yes | Layer Opacity |
| scrollDirection | int | 0-2 | 0 | no | Scroll Dir |
| scrollSpeed | float | 0.0-2.0 | 0.4 | yes | Scroll Speed |
| rainAmount | float | 0.0-1.0 | 1.0 | yes | Rain |
| flutterAmount | float | 0.0-1.0 | 0.3 | yes | Flutter |
| flutterSpeed | float | 0.1-10.0 | 2.0 | yes | Flutter Speed |
| waveAmplitude | float | 0.0-0.5 | 0.05 | yes | Wave Amp |
| waveFreq | float | 1.0-20.0 | 6.0 | yes | Wave Freq |
| waveSpeed | float | 0.0-5.0 | 1.0 | yes | Wave Speed |
| driftAmount | float | 0.0-0.5 | 0.0 | yes | Drift |
| driftSpeed | float | 0.1-5.0 | 0.5 | yes | Drift Speed |
| bandDistortion | float | 0.0-1.0 | 0.3 | yes | Band Distort |
| inversionRate | float | 0.0-1.0 | 0.1 | yes | Inversion |
| inversionSpeed | float | 0.0-2.0 | 0.1 | yes | Inversion Speed |
| lcdMode | bool | on/off | off | no | LCD Mode |
| lcdFreq | float | 100.0-2000.0 | 800.0 | yes | LCD Freq |
| gradient | ColorConfig | — | cyan→magenta | no | — |
| blendMode | EffectBlendMode | — | SCREEN | no | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_GLYPH_FIELD_BLEND`
- Display name: `"Glyph Field Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` → badge `"GEN"`, section 10
- Config member: `glyphField` in `EffectConfig`
- Effect member: `glyphField` in `PostEffect`
- Blend active: `glyphFieldBlendActive` in `PostEffect`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create GlyphFieldConfig and GlyphFieldEffect types

**Files**: `src/effects/glyph_field.h` (create)
**Creates**: Config struct and effect struct that all other files include

**Do**: Follow `plasma.h` pattern exactly. Define `GlyphFieldConfig` with all 23 fields from the parameter table above. Define `GlyphFieldEffect` with shader, fontAtlas (Texture2D), gradientLUT (ColorLUT*), time accumulator, and one `int` per uniform location (see Design section). Declare standard lifecycle functions: `GlyphFieldEffectInit`, `GlyphFieldEffectSetup`, `GlyphFieldEffectUninit`, `GlyphFieldConfigDefault`, `GlyphFieldRegisterParams`. Include `render/color_config.h` and `render/blend_mode.h`.

**Verify**: `cmake.exe --build build` compiles (header-only, no .cpp yet — other files don't include it yet).

---

### Wave 2: Parallel Implementation

All tasks in this wave touch different files with no overlap.

#### Task 2.1: Effect module implementation

**Files**: `src/effects/glyph_field.cpp` (create)
**Depends on**: Wave 1

**Do**: Follow `plasma.cpp` pattern.
- `GlyphFieldEffectInit`: Load shader from `shaders/glyph_field.fs`. Load font atlas via `LoadTexture("fonts/font_atlas.png")` — check `.id != 0`, cleanup shader on failure. Set `TEXTURE_FILTER_BILINEAR` and `TEXTURE_WRAP_REPEAT` on the atlas. Init `ColorLUT` from config gradient. Cache all uniform locations. Init `time = 0`.
- `GlyphFieldEffectSetup`: Accumulate `time += deltaTime` (raw time, no speed multiply — shader uses `time * scrollSpeed` etc.). Update `ColorLUT`. Bind all uniforms. Bind font atlas texture via `SetShaderValueTexture`. Bind gradient LUT texture.
- `GlyphFieldEffectUninit`: Unload shader, unload font atlas texture, uninit ColorLUT.
- `GlyphFieldConfigDefault`: Return default-initialized `GlyphFieldConfig{}`.
- `GlyphFieldRegisterParams`: Register all 18 modulatable float params (see parameter table) with `ModEngineRegisterParam`.

**Verify**: Compiles.

#### Task 2.2: Fragment shader

**Files**: `shaders/glyph_field.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the full algorithm from the research doc and Design section. Key points:
- `#version 330`. Inputs: `fragTexCoord`, `resolution`. Output: `finalColor`.
- Uniforms: `fontAtlas` (sampler2D), `gradientLUT` (sampler2D), `resolution`, `time`, plus all config params.
- Implement `pcg3d` and `pcg3df` hash functions from the research doc.
- Main loop: iterate `layerCount` layers. Per layer: scale UV, apply scroll direction mode, apply motion modes (rain, flutter, wave, drift), apply band distortion, compute cell coordinates, sample font atlas, apply inversion flicker, sample gradient LUT for color, accumulate additively with layer opacity falloff.
- Radial mode: Cartesian-to-polar, per-ring angular scroll, `fract()` seam wrapping.
- LCD sub-pixel: conditional multiply when `lcdMode > 0`.
- Aspect ratio: use `resolution` to correct UV aspect before grid computation.

**Verify**: Shader compiles (checked at runtime when effect initializes).

#### Task 2.3: Effect registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/glyph_field.h"` with other effect includes.
2. Add `TRANSFORM_GLYPH_FIELD_BLEND` to `TransformEffectType` enum — insert before `TRANSFORM_CRT` (after `TRANSFORM_SLASHES_BLEND`).
3. Add `"Glyph Field Blend"` to `TRANSFORM_EFFECT_NAMES` array at the matching position.
4. Add `GlyphFieldConfig glyphField;` to `EffectConfig` struct, near other generator configs.
5. Add `case TRANSFORM_GLYPH_FIELD_BLEND: return e->glyphField.enabled && e->glyphField.blendIntensity > 0.0f;` to `IsTransformEnabled`.

**Verify**: Compiles.

#### Task 2.4: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1

**Do**:
- `post_effect.h`: Add `#include "effects/glyph_field.h"`. Add `GlyphFieldEffect glyphField;` member. Add `bool glyphFieldBlendActive;` member.
- `post_effect.cpp`: Add `#include "effects/glyph_field.h"`.
  - In `PostEffectInit`: Call `GlyphFieldEffectInit(&pe->glyphField, &pe->effects.glyphField)` with failure handling (same pattern as Constellation).
  - In `PostEffectUninit`: Call `GlyphFieldEffectUninit(&pe->glyphField)`.
  - In `PostEffectRegisterParams`: Call `GlyphFieldRegisterParams(&pe->effects.glyphField)`.
  - In `RenderPipelineUpdate` (blend active flags section): Set `pe->glyphFieldBlendActive = IsTransformEnabled(&pe->effects, TRANSFORM_GLYPH_FIELD_BLEND)`.

**Verify**: Compiles.

#### Task 2.5: Shader setup (generators)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify)
**Depends on**: Wave 1

**Do**:
- `shader_setup_generators.h`: Declare `SetupGlyphField(PostEffect *pe)` and `SetupGlyphFieldBlend(PostEffect *pe)`.
- `shader_setup_generators.cpp`: Add `#include "effects/glyph_field.h"`.
  - `SetupGlyphField`: delegates to `GlyphFieldEffectSetup(&pe->glyphField, &pe->effects.glyphField, pe->currentDeltaTime)`.
  - `SetupGlyphFieldBlend`: delegates to `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.glyphField.blendIntensity, pe->effects.glyphField.blendMode)`.
  - Add `case TRANSFORM_GLYPH_FIELD_BLEND` in `GetGeneratorScratchPass`: return `{pe->glyphField.shader, SetupGlyphField}`.

**Verify**: Compiles.

#### Task 2.6: Shader setup dispatch

**Files**: `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `case TRANSFORM_GLYPH_FIELD_BLEND` in `GetTransformEffect()`:
```
return {&pe->blendCompositor->shader, SetupGlyphFieldBlend, &pe->glyphFieldBlendActive};
```
(Header `shader_setup_generators.h` is already included.)

**Verify**: Compiles.

#### Task 2.7: Render pipeline

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `type == TRANSFORM_GLYPH_FIELD_BLEND` to the `IsGeneratorBlendEffect()` function's return expression. Also add `pe->glyphFieldBlendActive = IsTransformEnabled(...)` line in the blend active flags section if not handled in Task 2.4.

**Verify**: Compiles.

#### Task 2.8: UI panel

**Files**: `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1

**Do**: Follow the `DrawGeneratorsConstellation` pattern.
1. Add `#include "effects/glyph_field.h"` (if not already pulled in transitively).
2. Add `static bool sectionGlyphField = false;` with other section bools.
3. Create `DrawGeneratorsGlyphField(EffectConfig *e, const ModSources *modSources, const ImU32 categoryGlow)`:
   - `DrawSectionBegin("Glyph Field", ...)`, checkbox, `MoveTransformToEnd` on enable.
   - Grid section: `ModulatableSlider` for gridSize, `SliderInt` for layerCount, `ModulatableSlider` for layerScaleSpread, layerSpeedSpread, layerOpacity.
   - Scroll section: `Combo` for scrollDirection ("Horizontal\0Vertical\0Radial\0"), `ModulatableSlider` for scrollSpeed.
   - Motion section: `ModulatableSlider` for rainAmount, flutterAmount, flutterSpeed, waveAmplitude, waveFreq, waveSpeed, driftAmount, driftSpeed.
   - Distortion section: `ModulatableSlider` for bandDistortion, inversionRate, inversionSpeed.
   - LCD section: `Checkbox` for lcdMode, conditional `ModulatableSlider` for lcdFreq.
   - Color: `ImGuiDrawColorMode(&c->gradient)`.
   - Output: `ModulatableSlider` for blendIntensity, `Combo` for blendMode.
4. Add call in `DrawGeneratorsCategory` with spacing, before the SolidColor entry (last generator).

**Verify**: Compiles.

#### Task 2.9: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlyphFieldConfig, enabled, gridSize, layerCount, layerScaleSpread, layerSpeedSpread, layerOpacity, scrollDirection, scrollSpeed, rainAmount, flutterAmount, flutterSpeed, waveAmplitude, waveFreq, waveSpeed, driftAmount, driftSpeed, bandDistortion, inversionRate, inversionSpeed, lcdMode, lcdFreq, gradient, blendMode, blendIntensity)` near other generator config macros.
2. Add `if (e.glyphField.enabled) { j["glyphField"] = e.glyphField; }` in `to_json`.
3. Add `e.glyphField = j.value("glyphField", e.glyphField);` in `from_json`.

**Verify**: Compiles.

#### Task 2.10: Category mapping

**Files**: `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Wave 1

**Do**: Add `case TRANSFORM_GLYPH_FIELD_BLEND:` to the generators group in `GetTransformCategory()`, returning `{"GEN", 10}`.

**Verify**: Compiles.

#### Task 2.11: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/glyph_field.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline as "Glyph Field Blend"
- [ ] Effect shows "GEN" category badge
- [ ] Enabling effect renders scrolling character grids
- [ ] Layer count, scroll direction, and motion modes respond to UI controls
- [ ] LCD sub-pixel mode toggles correctly
- [ ] Radial scroll mode renders concentric ring scrolling
- [ ] Gradient LUT colors characters per-cell
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
