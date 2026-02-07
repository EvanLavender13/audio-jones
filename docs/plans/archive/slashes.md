# Slashes

Chaotic thin rectangular bars scattered across the screen, one per semitone. Each tick, every bar re-rolls to a new random position and rotation via PCG3D hash. FFT magnitude gates visibility and drives width — loud notes snap open as bright slashes, quiet notes vanish. A snap-shut envelope gives each tick a punchy, staccato flash.

**Research**: `docs/research/slashes.md`

## Design

### Types

**SlashesConfig** (user-facing parameters):
```
bool enabled = false
float baseFreq = 55.0f          // Lowest mapped frequency (Hz)
int numOctaves = 4              // Octave count; total bars = octaves * 12
float gain = 5.0f               // FFT magnitude amplification
float curve = 1.0f              // Magnitude contrast shaping
float tickRate = 4.0f           // Re-roll rate (ticks/second)
float envelopeSharp = 4.0f      // Envelope decay sharpness
float maxBarWidth = 0.7f        // Maximum bar half-width at full magnitude
float barThickness = 0.005f     // Bar half-thickness baseline
float thicknessVariation = 0.5f // Random thickness spread per bar
float scatter = 0.5f            // Position offset range from center
float glowSoftness = 0.01f      // Edge anti-aliasing width
float baseBright = 0.05f        // Minimum brightness for visible notes
float rotationDepth = 0.0f      // 3D foreshortening (0=flat, 1=full 3D scatter)
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT}
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN
float blendIntensity = 1.0f
```

**SlashesEffect** (runtime state + shader handles):
```
Shader shader
ColorLUT* gradientLUT
float tickAccum              // CPU-accumulated tick counter
int resolutionLoc
int fftTextureLoc
int sampleRateLoc
int baseFreqLoc
int numOctavesLoc
int gainLoc
int curveLoc
int tickAccumLoc
int envelopeSharpLoc
int maxBarWidthLoc
int barThicknessLoc
int thicknessVariationLoc
int scatterLoc
int glowSoftnessLoc
int baseBrightLoc
int rotationDepthLoc
int gradientLUTLoc
```

### Algorithm

Single-pass fragment shader. Loops over `numOctaves * 12` semitones. For each bar:

1. **PCG3D hash** seeded with `(semitoneIndex, floor(tickAccum), floor(tickAccum))` produces deterministic random position/rotation per tick.
2. **FFT lookup** converts semitone index to frequency bin, samples fftTexture, applies `gain` and `curve` power.
3. **Envelope** `exp(-pow(fract(tickAccum), envelopeSharp))` peaks at tick boundary, decays within tick.
4. **Bar geometry**: UV rotated by hash-derived angle (2D when `rotationDepth=0`, or via `erot` Rodrigues rotation for 3D foreshortening), offset by hash-derived position * scatter. Width = `maxBarWidth * env * mag`. Thickness = `barThickness + hash.x * thicknessVariation * barThickness`.
5. **SDF**: `sdBox` signed distance to oriented rectangle.
6. **Glow**: `smoothstep(glowSoftness, 0.0, d)` soft-edged mask.
7. **Color**: gradient LUT sampled at `fract(i / 12.0)` (pitch class).
8. **Accumulate**: additive blend `result += color * glow * (baseBright + mag)`.

Final output through `tanh()` tone mapping.

`tickAccum` accumulates on CPU: `tickAccum += tickRate * deltaTime`. Passed as uniform. `floor(tickAccum)` gives discrete tick index for hash seeding. `fract(tickAccum)` drives envelope position.

Depth illusion via erot: when `rotationDepth > 0`, bar UV passes through 3D Rodrigues rotation with a random axis (from hash), then projects back to 2D. Bars tilted away appear foreshortened. Controlled by lerp between 2D rotation and erot result.

Reference: `docs/research/slashes.md` — PCG3D hash (Jarzynski & Olano), sdBox (IQ), erot (Suricrasia Online).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 20-2000 | 55.0 | yes | Base Freq (Hz) |
| numOctaves | int | 1-8 | 4 | no | Octaves |
| gain | float | 0.1-20.0 | 5.0 | yes | Gain |
| curve | float | 0.1-5.0 | 1.0 | yes | Contrast |
| tickRate | float | 0.5-20.0 | 4.0 | yes | Tick Rate |
| envelopeSharp | float | 1.0-10.0 | 4.0 | yes | Envelope Sharp |
| maxBarWidth | float | 0.1-1.5 | 0.7 | yes | Bar Width |
| barThickness | float | 0.001-0.05 | 0.005 | yes | Bar Thickness |
| thicknessVariation | float | 0.0-1.0 | 0.5 | yes | Thickness Var |
| scatter | float | 0.0-1.0 | 0.5 | yes | Scatter |
| glowSoftness | float | 0.001-0.05 | 0.01 | yes | Glow Softness |
| baseBright | float | 0.0-0.5 | 0.05 | yes | Base Bright |
| rotationDepth | float | 0.0-1.0 | 0.0 | yes | Rotation Depth |
| gradient | ColorConfig | — | gradient | no | (color widget) |
| blendMode | EffectBlendMode | — | SCREEN | no | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum: `TRANSFORM_SLASHES_BLEND`
- Display name: `"Slashes Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` → `{"GEN", 10}`
- Config member: `slashes`
- Effect member: `slashes`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create SlashesConfig header

**Files**: `src/effects/slashes.h` (create)
**Creates**: Config struct and effect struct that all other tasks `#include`

**Do**: Follow `src/effects/filaments.h` as pattern. Define `SlashesConfig` with all parameters from the Parameters table above. Define `SlashesEffect` with shader, gradientLUT pointer, tickAccum float, and all uniform location ints. Declare `SlashesEffectInit`, `SlashesEffectSetup` (takes fftTexture like filaments), `SlashesEffectUninit`, `SlashesConfigDefault`, `SlashesRegisterParams`. Include `render/blend_mode.h` and `render/color_config.h`.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker errors expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Implement slashes.cpp

**Files**: `src/effects/slashes.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `src/effects/filaments.cpp` as pattern. Load `shaders/slashes.fs`. Cache all uniform locations. Init/uninit ColorLUT. In Setup, accumulate `tickAccum += cfg->tickRate * deltaTime`, call `ColorLUTUpdate`, bind all uniforms including resolution, fftTexture, sampleRate, gradientLUT. Register all modulatable params per Parameters table (tickRate, envelopeSharp, maxBarWidth, barThickness, thicknessVariation, scatter, glowSoftness, baseBright, rotationDepth, gain, curve, baseFreq, blendIntensity).

**Verify**: Compiles.

---

#### Task 2.2: Create slashes.fs shader

**Files**: `shaders/slashes.fs` (create)
**Depends on**: Wave 1 complete (uniform names)

**Do**: Implement the Algorithm section. Uniforms match SlashesEffect location names. Include `pcg3d` hash function, `sdBox` SDF, `erot` Rodrigues rotation (all from research doc). Loop `numOctaves * 12`, apply envelope + FFT gating + SDF glow + gradient LUT color. Additive accumulation, `tanh()` tone mapping. When `rotationDepth > 0`, apply erot 3D rotation; otherwise pure 2D `mat2` rotation.

**Verify**: File exists, valid GLSL 330 syntax.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/slashes.h"` with other effect includes (alphabetical).
2. Add `TRANSFORM_SLASHES_BLEND` enum entry before `TRANSFORM_EFFECT_COUNT`, after `TRANSFORM_FILAMENTS_BLEND`.
3. Add `"Slashes Blend"` to `TRANSFORM_EFFECT_NAMES` array at matching position.
4. Add `SlashesConfig slashes;` member to `EffectConfig` struct after filaments, with comment `// Slashes (chaotic per-semitone rectangular bar scatter)`.
5. Add `IsTransformEnabled` case: `case TRANSFORM_SLASHES_BLEND: return e->slashes.enabled && e->slashes.blendIntensity > 0.0f;`

**Verify**: Compiles.

---

#### Task 2.4: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- **post_effect.h**: Add `#include "effects/slashes.h"`. Add `SlashesEffect slashes;` member after `filaments`. Add `bool slashesBlendActive;` after `filamentsBlendActive`.
- **post_effect.cpp**: Add `SlashesEffectInit(&pe->slashes, &pe->effects.slashes)` call in `PostEffectInit` after filaments init, with error logging on failure. Add `SlashesEffectUninit(&pe->slashes)` in `PostEffectUninit` after filaments uninit. Add `SlashesRegisterParams(&pe->effects.slashes)` in `PostEffectRegisterParams` after filaments register.

**Verify**: Compiles.

---

#### Task 2.5: Shader setup generators

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- **Header**: Declare `void SetupSlashes(PostEffect *pe);` and `void SetupSlashesBlend(PostEffect *pe);` after filaments declarations.
- **Source**: Add `#include "effects/slashes.h"`. Implement `SetupSlashes` — delegates to `SlashesEffectSetup(&pe->slashes, &pe->effects.slashes, pe->currentDeltaTime, pe->fftTexture)`. Implement `SetupSlashesBlend` — calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.slashes.blendIntensity, pe->effects.slashes.blendMode)`. Add `case TRANSFORM_SLASHES_BLEND: return {pe->slashes.shader, SetupSlashes};` to `GetGeneratorScratchPass` switch.

**Verify**: Compiles.

---

#### Task 2.6: Shader setup dispatcher

**Files**: `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add dispatch case in `GetTransformEffect` after `TRANSFORM_FILAMENTS_BLEND`:
```
case TRANSFORM_SLASHES_BLEND:
    return {&pe->blendCompositor->shader, SetupSlashesBlend, &pe->slashesBlendActive};
```

**Verify**: Compiles.

---

#### Task 2.7: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `|| type == TRANSFORM_SLASHES_BLEND` to `IsGeneratorBlendEffect()`.
2. Add `pe->slashesBlendActive = IsTransformEnabled(&pe->effects, TRANSFORM_SLASHES_BLEND);` after the filaments blend active line in the render function.

**Verify**: Compiles.

---

#### Task 2.8: UI panel

**Files**: `src/ui/imgui_effects.cpp` (modify), `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- **imgui_effects.cpp**: Add `case TRANSFORM_SLASHES_BLEND:` to `GetTransformCategory` under the generators section (returns `{"GEN", 10}`).
- **imgui_effects_generators.cpp**: Add `#include "effects/slashes.h"`. Add `static bool sectionSlashes = false;`. Create `DrawSlashesParams` and `DrawSlashesOutput` helper functions following filaments pattern. `DrawSlashesParams` sections: FFT (Octaves SliderInt, Base Freq, Gain, Contrast modulatable), Timing (Tick Rate, Envelope Sharp modulatable), Geometry (Bar Width, Bar Thickness, Thickness Var, Scatter, Rotation Depth — all modulatable), Glow (Glow Softness, Base Bright modulatable). `DrawSlashesOutput`: ColorConfig widget, Output section (Blend Intensity, Blend Mode). Create `DrawGeneratorsSlashes` orchestrator following filaments pattern. Add call to `DrawGeneratorsSlashes` in `DrawGeneratorsCategory` after filaments, before muons, with `ImGui::Spacing()`.

**Verify**: Compiles.

---

#### Task 2.9: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SlashesConfig, enabled, baseFreq, numOctaves, gain, curve, tickRate, envelopeSharp, maxBarWidth, barThickness, thicknessVariation, scatter, glowSoftness, baseBright, rotationDepth, gradient, blendMode, blendIntensity)` macro after FilamentsConfig macro.
2. Add `if (e.slashes.enabled) { j["slashes"] = e.slashes; }` in `to_json` after filaments.
3. Add `e.slashes = j.value("slashes", e.slashes);` in `from_json` after filaments.

**Verify**: Compiles.

---

#### Task 2.10: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/slashes.cpp` to `EFFECTS_SOURCES` list (alphabetical placement).

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline with "GEN" badge
- [ ] Enabling effect creates bars that scatter and re-roll each tick
- [ ] FFT reactivity: loud notes produce bright wide bars, quiet notes vanish
- [ ] Envelope produces punchy snap-shut flash at each tick boundary
- [ ] rotationDepth > 0 produces foreshortened 3D-looking bars
- [ ] Gradient LUT colors bars by pitch class
- [ ] All modulatable params respond to LFO routing
- [ ] Preset save/load round-trips all settings
- [ ] Blend mode and intensity work correctly
