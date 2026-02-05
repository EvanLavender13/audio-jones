# Effect Modules Batch 7: Retro

Migrate 5 retro effects (Pixelation, Glitch, ASCII Art, Matrix Rain, Synthwave) from monolithic PostEffect into self-contained modules under `src/effects/`.

**Reference**: [docs/plans/effect-modules-migration.md](effect-modules-migration.md)

## Design

### Module Pattern

Each effect becomes a self-contained module with:
- **Header** (`src/effects/<name>.h`): Config struct (moved from `src/config/<name>_config.h`), Effect struct (shader + uniform locations + time accumulators), lifecycle declarations
- **Implementation** (`src/effects/<name>.cpp`): Init (load shader, cache locations), Setup (bind uniforms, accumulate time), Uninit (unload shader), ConfigDefault (return `Config{}`)

### Effect Struct Layouts

**PixelationEffect** (4 uniform locations):
- `Shader shader`
- `int resolutionLoc, cellCountLoc, ditherScaleLoc, posterizeLevelsLoc`

**GlitchEffect** (49 uniform locations + 2 time accumulators):
- `Shader shader`
- `int resolutionLoc, timeLoc, frameLoc`
- `int crtEnabledLoc, curvatureLoc, vignetteEnabledLoc`
- `int analogIntensityLoc, aberrationLoc`
- `int blockThresholdLoc, blockOffsetLoc`
- `int vhsEnabledLoc, trackingBarIntensityLoc, scanlineNoiseIntensityLoc, colorDriftIntensityLoc`
- `int scanlineAmountLoc, noiseAmountLoc`
- `int datamoshEnabledLoc, datamoshIntensityLoc, datamoshMinLoc, datamoshMaxLoc, datamoshSpeedLoc, datamoshBandsLoc`
- `int rowSliceEnabledLoc, rowSliceIntensityLoc, rowSliceBurstFreqLoc, rowSliceBurstPowerLoc, rowSliceColumnsLoc`
- `int colSliceEnabledLoc, colSliceIntensityLoc, colSliceBurstFreqLoc, colSliceBurstPowerLoc, colSliceRowsLoc`
- `int diagonalBandsEnabledLoc, diagonalBandCountLoc, diagonalBandDisplaceLoc, diagonalBandSpeedLoc`
- `int blockMaskEnabledLoc, blockMaskIntensityLoc, blockMaskMinSizeLoc, blockMaskMaxSizeLoc, blockMaskTintLoc`
- `int temporalJitterEnabledLoc, temporalJitterAmountLoc, temporalJitterGateLoc`
- `int blockMultiplyEnabledLoc, blockMultiplySizeLoc, blockMultiplyControlLoc, blockMultiplyIterationsLoc, blockMultiplyIntensityLoc`
- `float time` (animation accumulator)
- `int frame` (frame counter)

**AsciiArtEffect** (6 uniform locations):
- `Shader shader`
- `int resolutionLoc, cellPixelsLoc, colorModeLoc, foregroundLoc, backgroundLoc, invertLoc`

**MatrixRainEffect** (9 uniform locations + 1 time accumulator):
- `Shader shader`
- `int resolutionLoc, cellSizeLoc, trailLengthLoc, fallerCountLoc, overlayIntensityLoc, refreshRateLoc, leadBrightnessLoc, timeLoc, sampleModeLoc`
- `float time` (rain animation accumulator, driven by `cfg->rainSpeed * deltaTime`)

**SynthwaveEffect** (18 uniform locations + 2 time accumulators):
- `Shader shader`
- `int resolutionLoc, horizonYLoc, colorMixLoc, palettePhaseLoc`
- `int gridSpacingLoc, gridThicknessLoc, gridOpacityLoc, gridGlowLoc, gridColorLoc`
- `int stripeCountLoc, stripeSoftnessLoc, stripeIntensityLoc, sunColorLoc`
- `int horizonIntensityLoc, horizonFalloffLoc, horizonColorLoc`
- `int gridTimeLoc, stripeTimeLoc`
- `float gridTime` (grid scroll accumulator, driven by `cfg->gridScrollSpeed * deltaTime`)
- `float stripeTime` (stripe scroll accumulator, driven by `cfg->stripeScrollSpeed * deltaTime`)

### Setup Function Signatures

- `PixelationEffectSetup(PixelationEffect* e, const PixelationConfig* cfg)` — no time state
- `GlitchEffectSetup(GlitchEffect* e, const GlitchConfig* cfg, float deltaTime)` — manages time + frame
- `AsciiArtEffectSetup(AsciiArtEffect* e, const AsciiArtConfig* cfg)` — no time state
- `MatrixRainEffectSetup(MatrixRainEffect* e, const MatrixRainConfig* cfg, float deltaTime)` — manages time
- `SynthwaveEffectSetup(SynthwaveEffect* e, const SynthwaveConfig* cfg, float deltaTime)` — manages gridTime + stripeTime

### Time Accumulator Migration

| Accumulator | Current Location | Moves To |
|-------------|-----------------|----------|
| `glitchTime` | `post_effect.h:374`, init in `post_effect.cpp:574` | `GlitchEffect.time` |
| `glitchFrame` | `post_effect.h:375`, init in `post_effect.cpp:575` | `GlitchEffect.frame` |
| `matrixRainTime` | `post_effect.h:377` | `MatrixRainEffect.time` |
| `synthwaveGridTime` | `post_effect.h:310`, init in `post_effect.cpp:572`, updated in `render_pipeline.cpp:226` | `SynthwaveEffect.gridTime` |
| `synthwaveStripeTime` | `post_effect.h:311`, init in `post_effect.cpp:573`, updated in `render_pipeline.cpp:227` | `SynthwaveEffect.stripeTime` |

### Integration Changes

**`effect_config.h`**: Replace 5 config includes (`pixelation_config.h`, `glitch_config.h`, `ascii_art_config.h`, `matrix_rain_config.h`, `synthwave_config.h`) with 5 effect includes (`effects/pixelation.h`, etc.)

**`post_effect.h`**: Remove all retro shader/location/time fields. Add 5 effect struct members (`PixelationEffect pixelation`, etc.)

**`post_effect.cpp`**: Replace LoadShader/GetShaderLocation/UnloadShader calls with module Init/Uninit calls. Remove retro entries from `LoadPostEffectShaders`, `GetShaderUniformLocations`, `SetResolutionUniforms`, `PostEffectUninit`.

**`shader_setup_retro.cpp`**: Each Setup function becomes a one-line delegate to the module's EffectSetup.

**`shader_setup.cpp`**: Update 5 dispatch cases to use `pe-><effect>.shader` instead of `pe-><effect>Shader`.

**`render_pipeline.cpp`**: Remove synthwave time accumulator lines (226-228).

### Pitfall Reminders

From migration plan:
- `#include <stddef.h>` (not `<stdlib.h>`) in .cpp files
- ConfigDefault returns ONLY `return Config{};`
- No null-checks in Init/Setup/Uninit
- Use `shader.id == 0` pattern for shader validation
- Verify ONLY batch 7 shaders removed from `LoadPostEffectShaders` — don't touch neighbors
- None of these effects are half-res, so all set resolution in Setup

---

## Tasks

### Wave 1: Effect Modules

Create 5 module file pairs. No file overlap between tasks — all parallel.

#### Task 1.1: Pixelation Module

**Files**: `src/effects/pixelation.h` (create), `src/effects/pixelation.cpp` (create)

**Do**:
- Move `PixelationConfig` from `src/config/pixelation_config.h` into the header
- Define `PixelationEffect` struct per layout above
- Init: load `shaders/pixelation.fs`, cache 4 uniform locations (`resolution`, `cellCount`, `ditherScale`, `posterizeLevels`)
- Setup: set resolution via `GetScreenWidth()`/`GetScreenHeight()`, bind 3 config uniforms. No time state.
- Implement `PixelationConfigDefault` that returns only `return PixelationConfig{};`
- Follow `src/effects/toon.h`/`toon.cpp` as reference (no time accumulator pattern)

**Verify**: Files compile in isolation (header includes are self-contained).

#### Task 1.2: Glitch Module

**Files**: `src/effects/glitch.h` (create), `src/effects/glitch.cpp` (create)

**Do**:
- Move `GlitchConfig` from `src/config/glitch_config.h` into the header
- Define `GlitchEffect` struct with 49 uniform locations + `float time` + `int frame`
- Init: load `shaders/glitch.fs`, cache all 49 locations. Uniform names match shader exactly (e.g., `"time"`, `"frame"`, `"crtEnabled"`, etc.)
- Setup: accumulate `time += deltaTime` and `frame++`. Bind all uniforms. Convert bools to int (0/1) before SetShaderValue, same as current `SetupGlitch`. Pack vec3 fields into float[3] arrays (blockMaskTint).
- Follow current `shader_setup_retro.cpp:14-153` for exact uniform binding logic
- Verify all uniform names against `shaders/glitch.fs` before implementing Init
- Implement `GlitchConfigDefault` that returns only `return GlitchConfig{};`

**Verify**: Files compile in isolation.

#### Task 1.3: ASCII Art Module

**Files**: `src/effects/ascii_art.h` (create), `src/effects/ascii_art.cpp` (create)

**Do**:
- Move `AsciiArtConfig` from `src/config/ascii_art_config.h` into the header
- Define `AsciiArtEffect` struct with 6 uniform locations
- Init: load `shaders/ascii_art.fs`, cache locations (`resolution`, `cellPixels`, `colorMode`, `foreground`, `background`, `invert`)
- Setup: set resolution, cast `cellSize` to int for `cellPixels`, convert `invert` bool to int, pack foreground/background as float[3]. No time state.
- Follow current `shader_setup_retro.cpp:155-171` for binding logic
- Reference `src/effects/toon.h`/`toon.cpp` as structural pattern
- Implement `AsciiArtConfigDefault` that returns only `return AsciiArtConfig{};`

**Verify**: Files compile in isolation.

#### Task 1.4: Matrix Rain Module

**Files**: `src/effects/matrix_rain.h` (create), `src/effects/matrix_rain.cpp` (create)

**Do**:
- Move `MatrixRainConfig` from `src/config/matrix_rain_config.h` into the header
- Define `MatrixRainEffect` struct with 9 uniform locations + `float time`
- Init: load `shaders/matrix_rain.fs`, cache locations, init `time = 0.0f`
- Setup: accumulate `time += cfg->rainSpeed * deltaTime`. Bind all 8 config uniforms + time. Convert `sampleMode` bool to int.
- Follow current `shader_setup_retro.cpp:173-196` for binding logic
- Reference `src/effects/halftone.h`/`halftone.cpp` as structural pattern (has time accumulator)
- Implement `MatrixRainConfigDefault` that returns only `return MatrixRainConfig{};`

**Verify**: Files compile in isolation.

#### Task 1.5: Synthwave Module

**Files**: `src/effects/synthwave.h` (create), `src/effects/synthwave.cpp` (create)

**Do**:
- Move `SynthwaveConfig` from `src/config/synthwave_config.h` into the header
- Define `SynthwaveEffect` struct with 18 uniform locations + `float gridTime` + `float stripeTime`
- Init: load `shaders/synthwave.fs`, cache all 18 locations, init both times to 0.0f
- Setup: accumulate `gridTime += cfg->gridScrollSpeed * deltaTime` and `stripeTime += cfg->stripeScrollSpeed * deltaTime`. Bind all uniforms. Pack vec3 fields (palettePhase, gridColor, sunColor, horizonColor) into float[3] arrays.
- Follow current `shader_setup_retro.cpp:198-249` for binding logic, plus move time accumulation from `render_pipeline.cpp:226-228` into Setup
- Implement `SynthwaveConfigDefault` that returns only `return SynthwaveConfig{};`

**Verify**: Files compile in isolation.

---

### Wave 2: Integration

Modify existing files to use modules. No file overlap between tasks — all parallel.

#### Task 2.1: Effect Config Integration

**Files**: `src/config/effect_config.h` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Replace 5 config includes with effect includes:
  - `#include "pixelation_config.h"` → `#include "effects/pixelation.h"`
  - `#include "glitch_config.h"` → `#include "effects/glitch.h"`
  - `#include "ascii_art_config.h"` → `#include "effects/ascii_art.h"`
  - `#include "matrix_rain_config.h"` → `#include "effects/matrix_rain.h"`
  - `#include "synthwave_config.h"` → `#include "effects/synthwave.h"`
- Everything else unchanged (enum, name, order array, EffectConfig members, IsTransformEnabled)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: PostEffect Integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)

**Depends on**: Wave 1 complete

**Do** in `post_effect.h`:
- Add 5 includes: `effects/pixelation.h`, `effects/glitch.h`, `effects/ascii_art.h`, `effects/matrix_rain.h`, `effects/synthwave.h`
- Remove bare shader fields: `pixelationShader` (line 72), `glitchShader` (line 74), `asciiArtShader` (line 78), `matrixRainShader` (line 89), `synthwaveShader` (line 90)
- Remove all uniform location fields: `pixelationResolutionLoc` through `pixelationPosterizeLevelsLoc` (lines 139-142), `glitchResolutionLoc` through `glitchBlockMultiplyIntensityLoc` (lines 158-206), `asciiArtResolutionLoc` through `asciiArtInvertLoc` (lines 236-241), `matrixRainResolutionLoc` through `matrixRainSampleModeLoc` (lines 264-272), `synthwaveResolutionLoc` through `synthwaveStripeTimeLoc` (lines 273-290)
- Remove time accumulators: `synthwaveGridTime` (line 310), `synthwaveStripeTime` (line 311), `glitchTime` (line 374), `glitchFrame` (line 375), `matrixRainTime` (line 377)
- Add 5 effect struct members after existing effect members (near line 359): `PixelationEffect pixelation;`, `GlitchEffect glitch;`, `AsciiArtEffect asciiArt;`, `MatrixRainEffect matrixRain;`, `SynthwaveEffect synthwave;`

**Do** in `post_effect.cpp`:
- In `LoadPostEffectShaders`: remove 5 LoadShader calls (pixelation line 97, glitch line 98, asciiArt line 102, matrixRain line 119, synthwave line 123). Remove their validation checks from the success block.
- In `GetShaderUniformLocations`: remove all GetShaderLocation calls for pixelation (lines 213-220), glitch (lines 221-307), asciiArt (lines 336-345), matrixRain (lines 386-402), synthwave (lines 403-433)
- In `SetResolutionUniforms`: remove entries for pixelation (lines 530-531), glitch (lines 532-533), asciiArt (lines 537-538), matrixRain (lines 543-544), synthwave (lines 545-546)
- In `PostEffectInit`: add 5 Init calls after existing effect inits, each with error handling:
  ```cpp
  if (!PixelationEffectInit(&pe->pixelation)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize pixelation");
    free(pe);
    return NULL;
  }
  ```
  Repeat pattern for all 5 effects.
- Remove time accumulator inits: `synthwaveGridTime` (line 572), `synthwaveStripeTime` (line 573), `glitchTime` (line 574), `glitchFrame` (line 575)
- In `PostEffectUninit`: replace 5 UnloadShader calls with Uninit calls (`PixelationEffectUninit(&pe->pixelation)`, etc.)
- **CRITICAL**: Diff `LoadPostEffectShaders` removal carefully. Only remove the 5 retro shaders. The plasma, heightfieldRelief, colorGrade, constellation, falseColor, paletteQuantization, bokeh, bloom, anamorphicStreak, interference shaders stay.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Shader Setup Retro

**Files**: `src/render/shader_setup_retro.cpp` (modify), `src/render/shader_setup_retro.h` (modify)

**Depends on**: Wave 1 complete

**Do** in `shader_setup_retro.h`:
- Add includes for all 5 effect headers
- Declarations stay the same (5 `void Setup*(PostEffect* pe)` functions)

**Do** in `shader_setup_retro.cpp`:
- Add includes for all 5 effect headers
- Replace each Setup function body with a one-line delegate:
  - `SetupPixelation`: `PixelationEffectSetup(&pe->pixelation, &pe->effects.pixelation);`
  - `SetupGlitch`: `GlitchEffectSetup(&pe->glitch, &pe->effects.glitch, pe->currentDeltaTime);`
  - `SetupAsciiArt`: `AsciiArtEffectSetup(&pe->asciiArt, &pe->effects.asciiArt);`
  - `SetupMatrixRain`: `MatrixRainEffectSetup(&pe->matrixRain, &pe->effects.matrixRain, pe->currentDeltaTime);`
  - `SetupSynthwave`: `SynthwaveEffectSetup(&pe->synthwave, &pe->effects.synthwave, pe->currentDeltaTime);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader Setup Dispatch

**Files**: `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Update 5 dispatch cases in `GetTransformEffect()`:
- `TRANSFORM_PIXELATION` (line 47): `&pe->pixelationShader` → `&pe->pixelation.shader`
- `TRANSFORM_GLITCH` (line 50): `&pe->glitchShader` → `&pe->glitch.shader`
- `TRANSFORM_ASCII_ART` (line 74): `&pe->asciiArtShader` → `&pe->asciiArt.shader`
- `TRANSFORM_MATRIX_RAIN` (line 135): `&pe->matrixRainShader` → `&pe->matrixRain.shader`
- `TRANSFORM_SYNTHWAVE` (line 166): `&pe->synthwaveShader` → `&pe->synthwave.shader`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Render Pipeline Cleanup

**Files**: `src/render/render_pipeline.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Remove synthwave time accumulator lines (around lines 226-228):
```
pe->synthwaveGridTime += deltaTime * pe->effects.synthwave.gridScrollSpeed;
pe->synthwaveStripeTime += deltaTime * pe->effects.synthwave.stripeScrollSpeed;
```
These now live inside `SynthwaveEffectSetup`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Build System

**Files**: `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**: Add 5 source files to `EFFECTS_SOURCES`:
```cmake
src/effects/pixelation.cpp
src/effects/glitch.cpp
src/effects/ascii_art.cpp
src/effects/matrix_rain.cpp
src/effects/synthwave.cpp
```

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 3: Cleanup

#### Task 3.1: Delete Old Config Headers

**Files**: Delete `src/config/pixelation_config.h`, `src/config/glitch_config.h`, `src/config/ascii_art_config.h`, `src/config/matrix_rain_config.h`, `src/config/synthwave_config.h`

**Depends on**: Wave 2 complete

**Do**:
- Delete the 5 config headers
- Grep for any stale includes of these files across the codebase (`pixelation_config.h`, `glitch_config.h`, `ascii_art_config.h`, `matrix_rain_config.h`, `synthwave_config.h`). Remove any found.
- Expected stale include locations: `preset.cpp`, `imgui_effects_retro.cpp` (if they include config headers directly)

**Verify**: `cmake.exe --build build` compiles with no warnings. No stale includes remain.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 5 effects appear in transform order pipeline
- [ ] All 5 effects show correct "Retro" category badge
- [ ] Enabling each effect adds it to the pipeline and renders correctly
- [ ] UI controls modify each effect in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to all registered parameters
- [ ] No stale config includes remain
- [ ] `LoadPostEffectShaders` diff shows ONLY retro shaders removed
- [ ] `render_pipeline.cpp` diff shows ONLY synthwave time lines removed
