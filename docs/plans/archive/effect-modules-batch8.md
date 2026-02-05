# Effect Modules Batch 8: Optical

Migrate 4 optical effects (Bloom, Bokeh, Heightfield Relief, Anamorphic Streak) from monolithic PostEffect into self-contained modules under `src/effects/`.

**Reference**: [docs/plans/effect-modules-migration.md](effect-modules-migration.md)

## Design

### Module Pattern

Each effect becomes a self-contained module with:
- **Header** (`src/effects/<name>.h`): Config struct (moved from `src/config/<name>_config.h`), Effect struct (shaders + uniform locations + render resources), lifecycle declarations
- **Implementation** (`src/effects/<name>.cpp`): Init (load shaders, cache locations, allocate resources), Setup (bind uniforms), Uninit (unload shaders, free resources), ConfigDefault (return `Config{}`)

### Multi-Pass Effects

Bloom and Anamorphic Streak own all their shaders and uniform locations inside the module struct. The multi-pass rendering functions (`ApplyBloomPasses`, `ApplyAnamorphicStreakPasses`) stay in `shader_setup.cpp` for now but access everything through `pe->bloom.*` and `pe->anamorphicStreak.*`. Same pattern as OilPaint: module owns all state, `Apply*` in `shader_setup.cpp` orchestrates the render passes.

### Effect Struct Layouts

**BloomEffect** (4 shaders, `BLOOM_MIP_COUNT` render textures, 6 uniform locations):
- `Shader prefilterShader, downsampleShader, upsampleShader, compositeShader`
- `RenderTexture2D mips[BLOOM_MIP_COUNT]`
- `int thresholdLoc, kneeLoc` (prefilter)
- `int downsampleHalfpixelLoc` (downsample)
- `int upsampleHalfpixelLoc` (upsample)
- `int intensityLoc, bloomTexLoc` (composite)

**BokehEffect** (4 uniform locations):
- `Shader shader`
- `int resolutionLoc, radiusLoc, iterationsLoc, brightnessPowerLoc`

**HeightfieldReliefEffect** (6 uniform locations):
- `Shader shader`
- `int resolutionLoc, intensityLoc, reliefScaleLoc, lightAngleLoc, lightHeightLoc, shininessLoc`

**AnamorphicStreakEffect** (3 shaders, 7 uniform locations):
- `Shader prefilterShader, blurShader, compositeShader`
- `int thresholdLoc, kneeLoc` (prefilter)
- `int resolutionLoc, offsetLoc, sharpnessLoc` (blur)
- `int intensityLoc, streakTexLoc` (composite)

### Setup Function Signatures

- `BloomEffectSetup(BloomEffect* e, const BloomConfig* cfg)` — binds composite uniforms only (intensity + bloomTexture)
- `BokehEffectSetup(BokehEffect* e, const BokehConfig* cfg)` — binds all uniforms, no time state
- `HeightfieldReliefEffectSetup(HeightfieldReliefEffect* e, const HeightfieldReliefConfig* cfg)` — binds all uniforms, no time state
- `AnamorphicStreakEffectSetup(AnamorphicStreakEffect* e, const AnamorphicStreakConfig* cfg)` — binds composite uniforms only (intensity + streakTexture)

Note: Bloom prefilter/downsample/upsample and anamorphic prefilter/blur uniforms are set by `ApplyBloomPasses` / `ApplyAnamorphicStreakPasses` in `shader_setup.cpp`, not by the module's Setup. This mirrors OilPaint where stroke uniforms are set by `ApplyHalfResOilPaint`.

### Bloom Resource Lifecycle

`BloomEffect` owns `mips[]`. Requires:
- `BloomEffectInit(BloomEffect* e, int width, int height)` — loads 4 shaders, caches locations, allocates mip chain
- `BloomEffectResize(BloomEffect* e, int width, int height)` — unloads and reallocates mips
- `BloomEffectUninit(BloomEffect* e)` — unloads 4 shaders and mip chain

The `InitBloomMips` / `UnloadBloomMips` helpers in `post_effect.cpp` move into `bloom.cpp` as static helpers.

### Resolution Handling

- **Bokeh**: Sets resolution in Setup (not in HALF_RES_EFFECTS, not half-res)
- **Heightfield Relief**: Sets resolution in Setup (not half-res)
- **Bloom**: No resolution uniform on composite shader; prefilter/downsample/upsample use per-mip halfpixel values set by `ApplyBloomPasses`
- **Anamorphic Streak**: No resolution uniform on composite shader; blur shader resolution set by `ApplyAnamorphicStreakPasses`

### Integration Changes

**`effect_config.h`**: Replace 4 config includes (`anamorphic_streak_config.h`, `bloom_config.h`, `bokeh_config.h`, `heightfield_relief_config.h`) with 4 effect includes (`effects/bloom.h`, `effects/bokeh.h`, `effects/heightfield_relief.h`, `effects/anamorphic_streak.h`)

**`post_effect.h`**: Remove all optical shader/location fields. Add 4 effect struct members (`BloomEffect bloom`, `BokehEffect bokeh`, `HeightfieldReliefEffect heightfieldRelief`, `AnamorphicStreakEffect anamorphicStreak`)

**`post_effect.cpp`**: Replace LoadShader/GetShaderLocation/UnloadShader calls with module Init/Uninit calls. Remove optical entries from `LoadPostEffectShaders`, `GetShaderUniformLocations`, `SetResolutionUniforms`, `PostEffectUninit`. Move `InitBloomMips`/`UnloadBloomMips` into bloom module. Remove bloom mip init/uninit from `PostEffectInit`/`PostEffectUninit`. Update `PostEffectResize` to call `BloomEffectResize` instead of `UnloadBloomMips`/`InitBloomMips`.

**`shader_setup_optical.cpp`**: Each Setup function becomes a one-line delegate to the module's EffectSetup.

**`shader_setup.cpp`**: Update 4 dispatch cases to use module shader fields. Update `ApplyBloomPasses` to access `pe->bloom.*` instead of `pe->bloom*`. Update `ApplyAnamorphicStreakPasses` to access `pe->anamorphicStreak.*`. Remove `#include "config/anamorphic_streak_config.h"`.

### Pitfall Reminders

From migration plan:
- `#include <stddef.h>` (not `<stdlib.h>`) in .cpp files
- ConfigDefault returns ONLY `return Config{};`
- No null-checks in Init/Setup/Uninit
- Use `shader.id == 0` pattern for shader validation
- Verify ONLY batch 8 shaders removed from `LoadPostEffectShaders` — don't touch neighbors
- None of these effects are in HALF_RES_EFFECTS (no changes to that array)

---

## Tasks

### Wave 1: Effect Modules

Create 4 module file pairs. No file overlap between tasks — all parallel.

#### Task 1.1: Bloom Module

**Files**: `src/effects/bloom.h` (create), `src/effects/bloom.cpp` (create)

**Do**:
- Move `BloomConfig` from `src/config/bloom_config.h` into the header. Keep `BLOOM_MIP_COUNT` as a constant in the header (other code references it).
- Define `BloomEffect` struct per layout above (4 shaders, `mips[BLOOM_MIP_COUNT]`, 6 uniform locations)
- Init signature: `bool BloomEffectInit(BloomEffect* e, int width, int height)`. Loads 4 shaders (`bloom_prefilter.fs`, `bloom_downsample.fs`, `bloom_upsample.fs`, `bloom_composite.fs`). Caches 6 uniform locations:
  - prefilter: `"threshold"`, `"knee"`
  - downsample: `"halfpixel"`
  - upsample: `"halfpixel"`
  - composite: `"intensity"`, `"bloomTexture"`
- Init also allocates mip chain: same logic as current `InitBloomMips` in `post_effect.cpp` (halving width/height per level, clamping to 1). Use `RenderUtilsInitTextureHDR` — include `"render_utils.h"`.
- Setup: binds composite uniforms only — intensity float + bloomTexture via `SetShaderValueTexture(e->compositeShader, e->bloomTexLoc, e->mips[0].texture)`. Same as current `SetupBloom` in `shader_setup_optical.cpp`.
- Resize: `void BloomEffectResize(BloomEffect* e, int width, int height)` — unloads mips, reallocates
- Uninit: unloads 4 shaders + mip chain
- ConfigDefault: `return BloomConfig{};`
- Follow `src/effects/oil_paint.h`/`oil_paint.cpp` as reference (multi-shader + resource ownership + Resize pattern)

**Verify**: Files compile in isolation.

#### Task 1.2: Bokeh Module

**Files**: `src/effects/bokeh.h` (create), `src/effects/bokeh.cpp` (create)

**Do**:
- Move `BokehConfig` from `src/config/bokeh_config.h` into the header
- Define `BokehEffect` struct with 4 uniform locations
- Init: load `shaders/bokeh.fs`, cache locations (`resolution`, `radius`, `iterations`, `brightnessPower`)
- Setup: set resolution via `GetScreenWidth()`/`GetScreenHeight()`, bind 3 config uniforms. No time state.
- ConfigDefault: `return BokehConfig{};`
- Follow `src/effects/toon.h`/`toon.cpp` as reference (simple single-shader, no time)

**Verify**: Files compile in isolation.

#### Task 1.3: Heightfield Relief Module

**Files**: `src/effects/heightfield_relief.h` (create), `src/effects/heightfield_relief.cpp` (create)

**Do**:
- Move `HeightfieldReliefConfig` from `src/config/heightfield_relief_config.h` into the header
- Define `HeightfieldReliefEffect` struct with 6 uniform locations
- Init: load `shaders/heightfield_relief.fs`, cache locations (`resolution`, `intensity`, `reliefScale`, `lightAngle`, `lightHeight`, `shininess`)
- Setup: set resolution via `GetScreenWidth()`/`GetScreenHeight()`, bind 5 config uniforms. No time state.
- ConfigDefault: `return HeightfieldReliefConfig{};`
- Follow `src/effects/toon.h`/`toon.cpp` as reference

**Verify**: Files compile in isolation.

#### Task 1.4: Anamorphic Streak Module

**Files**: `src/effects/anamorphic_streak.h` (create), `src/effects/anamorphic_streak.cpp` (create)

**Do**:
- Move `AnamorphicStreakConfig` from `src/config/anamorphic_streak_config.h` into the header
- Define `AnamorphicStreakEffect` struct with 3 shaders and 7 uniform locations
- Init: load 3 shaders (`anamorphic_streak_prefilter.fs`, `anamorphic_streak_blur.fs`, `anamorphic_streak_composite.fs`). Cache 7 uniform locations:
  - prefilter: `"threshold"`, `"knee"`
  - blur: `"resolution"`, `"offset"`, `"sharpness"`
  - composite: `"intensity"`, `"streakTexture"`
- Setup: binds composite uniforms only — intensity float + streakTexture via `SetShaderValueTexture`. The `halfResA.texture` must be passed as a parameter since the module doesn't own it. Signature: `void AnamorphicStreakEffectSetup(AnamorphicStreakEffect* e, const AnamorphicStreakConfig* cfg, Texture2D streakTex)`.
- Uninit: unloads 3 shaders
- ConfigDefault: `return AnamorphicStreakConfig{};`
- Follow `src/effects/oil_paint.h`/`oil_paint.cpp` as reference (multi-shader pattern)

**Verify**: Files compile in isolation.

---

### Wave 2: Integration

Modify existing files to use modules. No file overlap between tasks — all parallel.

#### Task 2.1: Effect Config Integration

**Files**: `src/config/effect_config.h` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Replace 4 config includes with effect includes:
  - `#include "anamorphic_streak_config.h"` → `#include "effects/anamorphic_streak.h"`
  - `#include "bloom_config.h"` → `#include "effects/bloom.h"`
  - `#include "bokeh_config.h"` → `#include "effects/bokeh.h"`
  - `#include "heightfield_relief_config.h"` → `#include "effects/heightfield_relief.h"`
- Everything else unchanged (enum, name, order array, EffectConfig members, IsTransformEnabled)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: PostEffect Integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)

**Depends on**: Wave 1 complete

**Do** in `post_effect.h`:
- Add 4 includes: `effects/bloom.h`, `effects/bokeh.h`, `effects/heightfield_relief.h`, `effects/anamorphic_streak.h`
- Remove bare shader fields: `heightfieldReliefShader`, `bokehShader`, `bloomPrefilterShader`, `bloomDownsampleShader`, `bloomUpsampleShader`, `bloomCompositeShader`, `anamorphicStreakPrefilterShader`, `anamorphicStreakBlurShader`, `anamorphicStreakCompositeShader`
- Remove `bloomMips[BLOOM_MIP_COUNT]` render texture array
- Remove all uniform location fields: `heightfieldReliefResolutionLoc` through `heightfieldReliefShininessLoc`, `bokehResolutionLoc` through `bokehBrightnessPowerLoc`, `bloomThresholdLoc` through `bloomBloomTexLoc`, `anamorphicStreakThresholdLoc` through `anamorphicStreakStreakTexLoc`
- Add 4 effect struct members: `BloomEffect bloom;`, `BokehEffect bokeh;`, `HeightfieldReliefEffect heightfieldRelief;`, `AnamorphicStreakEffect anamorphicStreak;`

**Do** in `post_effect.cpp`:
- Remove `InitBloomMips` and `UnloadBloomMips` static helper functions (logic moved to bloom module)
- In `LoadPostEffectShaders`: remove LoadShader calls for `heightfieldReliefShader`, `bokehShader`, `bloomPrefilterShader`, `bloomDownsampleShader`, `bloomUpsampleShader`, `bloomCompositeShader`, `anamorphicStreakPrefilterShader`, `anamorphicStreakBlurShader`, `anamorphicStreakCompositeShader`. Remove their `.id != 0` checks from the validation return.
- In `GetShaderUniformLocations`: remove all GetShaderLocation calls for heightfield relief, bokeh, bloom, and anamorphic streak uniform locations.
- In `SetResolutionUniforms`: remove entries for `heightfieldReliefShader` and `bokehShader`.
- In `PostEffectInit`: add 4 Init calls after existing effect inits:
  - `BloomEffectInit(&pe->bloom, screenWidth, screenHeight)` — with error handling (`free(pe); return NULL;`)
  - `BokehEffectInit(&pe->bokeh)` — with error handling
  - `HeightfieldReliefEffectInit(&pe->heightfieldRelief)` — with error handling
  - `AnamorphicStreakEffectInit(&pe->anamorphicStreak)` — with error handling
- Remove bloom mip allocation code and TraceLog from PostEffectInit (now handled by BloomEffectInit)
- In `PostEffectResize`: replace `UnloadBloomMips(pe); InitBloomMips(pe, width, height);` with `BloomEffectResize(&pe->bloom, width, height);`
- In `PostEffectUninit`: replace UnloadShader calls for optical shaders with module Uninit calls (`BloomEffectUninit`, `BokehEffectUninit`, `HeightfieldReliefEffectUninit`, `AnamorphicStreakEffectUninit`). Remove `UnloadBloomMips(pe)`.
- **CRITICAL**: Diff `LoadPostEffectShaders` removal carefully. Only remove the 9 optical shader loads. The plasma, colorGrade, constellation, falseColor, paletteQuantization, interference shaders stay. The ToonEffectInit, NeonGlowEffectInit, HalftoneEffectInit, KuwaharaEffectInit, DiscoBallEffectInit, LegoBricksEffectInit calls stay.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Shader Setup Optical

**Files**: `src/render/shader_setup_optical.cpp` (modify), `src/render/shader_setup_optical.h` (modify)

**Depends on**: Wave 1 complete

**Do** in `shader_setup_optical.h`:
- Declarations stay the same (4 `void Setup*(PostEffect* pe)` functions)

**Do** in `shader_setup_optical.cpp`:
- Remove `#include "config/anamorphic_streak_config.h"` (no longer needed, types come through `post_effect.h`)
- Replace each Setup function body with a one-line delegate:
  - `SetupBloom`: `BloomEffectSetup(&pe->bloom, &pe->effects.bloom);`
  - `SetupBokeh`: `BokehEffectSetup(&pe->bokeh, &pe->effects.bokeh);`
  - `SetupHeightfieldRelief`: `HeightfieldReliefEffectSetup(&pe->heightfieldRelief, &pe->effects.heightfieldRelief);`
  - `SetupAnamorphicStreak`: `AnamorphicStreakEffectSetup(&pe->anamorphicStreak, &pe->effects.anamorphicStreak, pe->halfResA.texture);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader Setup Dispatch + Multi-Pass

**Files**: `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Remove `#include "config/anamorphic_streak_config.h"`
- Update 4 dispatch cases in `GetTransformEffect()`:
  - `TRANSFORM_HEIGHTFIELD_RELIEF`: `&pe->heightfieldReliefShader` → `&pe->heightfieldRelief.shader`
  - `TRANSFORM_BOKEH`: `&pe->bokehShader` → `&pe->bokeh.shader`
  - `TRANSFORM_BLOOM`: `&pe->bloomCompositeShader` → `&pe->bloom.compositeShader`
  - `TRANSFORM_ANAMORPHIC_STREAK`: `&pe->anamorphicStreakCompositeShader` → `&pe->anamorphicStreak.compositeShader`
- Update `BloomRenderPass` helper: no changes needed (takes shader as parameter)
- Update `ApplyBloomPasses`:
  - Access config: `&pe->effects.bloom` (unchanged)
  - Prefilter shader/locs: `pe->bloomPrefilterShader` → `pe->bloom.prefilterShader`, `pe->bloomThresholdLoc` → `pe->bloom.thresholdLoc`, `pe->bloomKneeLoc` → `pe->bloom.kneeLoc`
  - Downsample shader/locs: `pe->bloomDownsampleShader` → `pe->bloom.downsampleShader`, `pe->bloomDownsampleHalfpixelLoc` → `pe->bloom.downsampleHalfpixelLoc`
  - Upsample shader/locs: `pe->bloomUpsampleShader` → `pe->bloom.upsampleShader`, `pe->bloomUpsampleHalfpixelLoc` → `pe->bloom.upsampleHalfpixelLoc`
  - Mips: `pe->bloomMips` → `pe->bloom.mips`
- Update `ApplyAnamorphicStreakPasses`:
  - Access config: `&pe->effects.anamorphicStreak` (unchanged)
  - Prefilter shader/locs: `pe->anamorphicStreakPrefilterShader` → `pe->anamorphicStreak.prefilterShader`, `pe->anamorphicStreakThresholdLoc` → `pe->anamorphicStreak.thresholdLoc`, `pe->anamorphicStreakKneeLoc` → `pe->anamorphicStreak.kneeLoc`
  - Blur shader/locs: `pe->anamorphicStreakBlurShader` → `pe->anamorphicStreak.blurShader`, `pe->anamorphicStreakResolutionLoc` → `pe->anamorphicStreak.resolutionLoc`, `pe->anamorphicStreakOffsetLoc` → `pe->anamorphicStreak.offsetLoc`, `pe->anamorphicStreakSharpnessLoc` → `pe->anamorphicStreak.sharpnessLoc`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Build System

**Files**: `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**: Add 4 source files to `EFFECTS_SOURCES`:
```cmake
src/effects/bloom.cpp
src/effects/bokeh.cpp
src/effects/heightfield_relief.cpp
src/effects/anamorphic_streak.cpp
```

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 3: Cleanup

#### Task 3.1: Delete Old Config Headers + Stale Includes

**Files**: Delete `src/config/bloom_config.h`, `src/config/bokeh_config.h`, `src/config/heightfield_relief_config.h`, `src/config/anamorphic_streak_config.h`

**Depends on**: Wave 2 complete

**Do**:
- Delete the 4 config headers
- Grep for any stale includes of these files across the codebase. Expected stale include locations:
  - `src/config/preset.cpp`: `#include "config/anamorphic_streak_config.h"` — remove
  - `src/ui/imgui_effects_optical.cpp`: `#include "config/anamorphic_streak_config.h"`, `#include "config/bloom_config.h"`, `#include "config/bokeh_config.h"` — remove all three (types now come through `config/effect_config.h`)
- HeightfieldRelief config is NOT directly included by preset.cpp or imgui_effects_optical.cpp (it comes through effect_config.h), so no stale include to clean.

**Verify**: `cmake.exe --build build` compiles with no warnings. No stale includes remain.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 4 effects appear in transform order pipeline
- [ ] Enabling each effect renders correctly
- [ ] Bloom multi-pass (prefilter → downsample → upsample → composite) works
- [ ] Anamorphic streak multi-pass (prefilter → blur iterations → composite) works
- [ ] UI controls modify each effect in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to all registered parameters
- [ ] No stale config includes remain
- [ ] `LoadPostEffectShaders` diff shows ONLY optical shaders removed
- [ ] `PostEffectResize` calls `BloomEffectResize` instead of manual mip management
- [ ] `HALF_RES_EFFECTS` array unchanged
